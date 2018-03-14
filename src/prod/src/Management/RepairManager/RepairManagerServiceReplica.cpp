// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("Replica");
StringLiteral const TimerTag_Recovery("RMRecovery");
StringLiteral const TimerTag_Background("RMBackground");

class RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation : public AsyncOperation
{
public:
    FinishAcceptRequestAsyncOperation(
        __in RepairManagerServiceReplica &,
        ErrorCode &&,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    FinishAcceptRequestAsyncOperation(
        __in RepairManagerServiceReplica &, 
        StoreTransaction &&,
        shared_ptr<RepairTaskContext> &&,
        TimeSpan const,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    static ErrorCode End(AsyncOperationSPtr const &, int64 &);

    void OnStart(AsyncOperationSPtr const &);

private:
    
    void OnCommitComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

    RepairManagerServiceReplica & owner_;
    StoreTransaction storeTx_;
    shared_ptr<RepairTaskContext> context_;
    int64 commitVersion_;
    ErrorCode error_;
    ReplicaActivityId activityId_;
};

RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation::FinishAcceptRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , owner_(owner)
    , storeTx_(StoreTransaction::CreateInvalid(owner.ReplicatedStore, owner.PartitionedReplicaId))
    , context_()
    , commitVersion_(0)
    , error_(move(error))
    , activityId_(ReplicaActivityId(owner.PartitionedReplicaId, ActivityId()))
{
}

RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation::FinishAcceptRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    StoreTransaction && storeTx,
    shared_ptr<RepairTaskContext> && context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , owner_(owner)
    , storeTx_(move(storeTx))
    , context_(move(context))
    , commitVersion_(0)
    , error_(ErrorCode::Success())
    , activityId_(storeTx_.ReplicaActivityId)
{
    UNREFERENCED_PARAMETER(timeout);
}

ErrorCode RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation::End(
    AsyncOperationSPtr const & operation,
    __out int64 & commitVersion)
{
    commitVersion = 0;
    auto casted = AsyncOperation::End<FinishAcceptRequestAsyncOperation>(operation);
    
    if (casted->Error.IsSuccess())
    {
        commitVersion = casted->commitVersion_;
    }

    return casted->Error;
}

void RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (context_)
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx_),
            *context_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error_);
    }
}

void RepairManagerServiceReplica::FinishAcceptRequestAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode commitError = StoreTransaction::EndCommit(operation);

    if (commitError.IsSuccess())
    {
        // Store the version before the context becomes inaccessible
        commitVersion_ = context_->Task.Version;

        owner_.TryEnqueueTask(move(context_), activityId_);
    }
    else
    {
        error_ = commitError;
    }

    this->TryComplete(operation->Parent, error_);
}

//////////////////////////////////////////////////////////////////////////////////////

class RepairManagerServiceReplica::ChargeTask
{
    DENY_COPY(ChargeTask);

public:

    ChargeTask(RepairManagerServiceReplica & owner, ReplicaActivityId const & activityId)
        : owner_(owner)
        , chargePending_(owner.TryChargeNewTask(activityId))
        , activityId_(activityId)
    {
    }

    ~ChargeTask()
    {
        if (chargePending_)
        {
            owner_.CreditTask(activityId_, true, true);
        }
    }

    bool ChargeSucceeded()
    {
        return chargePending_;
    }

    void Commit()
    {
        chargePending_ = false;
    }

private:
    RepairManagerServiceReplica & owner_;
    bool chargePending_;
    ReplicaActivityId activityId_;
};

//////////////////////////////////////////////////////////////////////////////////////

RepairManagerServiceReplica::RepairManagerServiceReplica(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const & serviceName,
    __in ServiceRoutingAgentProxy & routingAgentProxy,
    Api::IClientFactoryPtr const & clientFactory,
    Federation::NodeInstance const & nodeInstance,
    RepairManagerServiceFactoryHolder const & factoryHolder)
    : KeyValueStoreReplica(partitionId, replicaId)
    , serviceName_(serviceName)
    , nodeInstance_(nodeInstance)
    , serviceLocation_()
    , clientFactory_(clientFactory)
    , queryClient_()
    , clusterMgmtClient_()
    , routingAgentProxy_(routingAgentProxy)
    , factoryHolder_(factoryHolder)
    , queryMessageHandler_()
    , lock_()
    , state_(RepairManagerState::Stopped)
    , outstandingOperations_(0)
    , recoveryTimer_()
    , knownFabricNodes_()
    , activeRepairTaskCount_(0)
    , totalRepairTaskCount_(0)
    , busyTasks_()
    , backgroundProcessingTimer_()
    , healthClient_()
    , healthCheckTimer_()
    , healthLastQueriedAt_(DateTime::Zero)
{
    this->SetTraceId(this->PartitionedReplicaId.TraceId);

    WriteInfo(
        TraceComponent,
        "{0} ctor: node = {1}, this = {2}",
        this->TraceId,
        this->NodeInstance,
        TraceThis);
}

RepairManagerServiceReplica::~RepairManagerServiceReplica()
{
    WriteInfo(
        TraceComponent,
        "{0} ~dtor: node = {1}, this = {2}",
        this->TraceId,
        this->NodeInstance,
        TraceThis);
}

// *******************
// StatefulServiceBase
// *******************

ErrorCode RepairManagerServiceReplica::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    UNREFERENCED_PARAMETER(servicePartition);

    // Cache service location since it will not change after open
    serviceLocation_ = SystemServiceLocation(
        this->NodeInstance,
        this->PartitionId,
        this->ReplicaId,
        DateTime::Now().Ticks);

    RMEvents::Trace->ReplicaOpen(
        this->PartitionedReplicaId,
        this->NodeInstance,
        serviceLocation_);

    auto error = clientFactory_->CreateQueryClient(queryClient_);
    if (!error.IsSuccess())
    {
        WriteError(TraceComponent, "{0} CreateQueryClient failed, error = {1}", this->TraceId, error);
        return error;
    }

    error = clientFactory_->CreateClusterManagementClient(clusterMgmtClient_);
    if (!error.IsSuccess())
    {
        WriteError(TraceComponent, "{0} CreateClusterManagementClient failed, error = {1}", this->TraceId, error);
        return error;
    }

    error = clientFactory_->CreateHealthClient(healthClient_);
    if (!error.IsSuccess())
    {
        WriteError(TraceComponent, "{0} CreateHealthClient failed, error = {1}", this->TraceId, error);
        return error;
    }

    return ErrorCode::Success();
}

ErrorCode RepairManagerServiceReplica::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
{
    RMEvents::Trace->ReplicaChangeRole(
        this->PartitionedReplicaId,
        ToWString(newRole));

    WriteInfo(
        TraceComponent,
        "{0} OnChangeRole started: node = {1}, this = {2}, newRole = {3}",
        this->TraceId,
        this->NodeInstance,
        TraceThis,
        ToWString(newRole)); 

    ErrorCode error(ErrorCodeValue::Success);
    if (newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {   
        RegisterMessageHandler();

        serviceLocation = this->serviceLocation_.Location;

        this->Start();
    }
    else
    {
        UnregisterMessageHandler();

        this->Stop();
    }
    
    WriteTrace(
        error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
        TraceComponent,
        "{0} OnChangeRole completed: node = {1}, this = {2}, error = {3}",
        this->TraceId,
        this->NodeInstance,
        TraceThis,
        error);

    return error;
}

ErrorCode RepairManagerServiceReplica::OnClose()
{
    RMEvents::Trace->ReplicaClose(this->PartitionedReplicaId);

    this->Stop();

    auto callback = this->OnCloseReplicaCallback;
    if (callback)
    {
        callback(this->PartitionId);
    }

    return ErrorCode::Success();
}

void RepairManagerServiceReplica::OnAbort()
{
    RMEvents::Trace->ReplicaAbort(this->PartitionedReplicaId);

    OnClose();
}

wstring RepairManagerServiceReplica::ToWString(FABRIC_REPLICA_ROLE role)
{
    switch (role)
    {
        case FABRIC_REPLICA_ROLE_NONE:
            return L"None";
        case FABRIC_REPLICA_ROLE_PRIMARY:
            return L"Primary";
        case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            return L"Idle Secondary";
        case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            return L"Active Secondary";
        default:
            return L"Unknown";
    }
}

void RepairManagerServiceReplica::RegisterMessageHandler()
{
    auto selfRoot = this->CreateComponentRoot();

    routingAgentProxy_.RegisterMessageHandler(
        serviceLocation_,
        [this, selfRoot](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->RequestMessageHandler(move(message), move(receiverContext));
        });

    // Create and register the query handlers
    queryMessageHandler_ = make_unique<QueryMessageHandler>(*selfRoot, QueryAddresses::RMAddressSegment);
    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        {
            return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
        },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
        {
            return this->EndProcessQuery(operation, reply);
        });

    auto error = queryMessageHandler_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} QueryMessageHandler failed to open with error {1}",
            this->TraceId,
            error);
    }
}

void RepairManagerServiceReplica::UnregisterMessageHandler()
{
    routingAgentProxy_.UnregisterMessageHandler(serviceLocation_);

    if (queryMessageHandler_)
    {
        queryMessageHandler_->Close();
    }
}

TimeSpan RepairManagerServiceReplica::GetRandomizedOperationRetryDelay(ErrorCode const & error) const
{
    return StoreTransaction::GetRandomizedOperationRetryDelay(
        error,
        RepairManagerConfig::GetConfig().MaxOperationRetryDelay);
}

GlobalWString RepairManagerServiceReplica::RejectReasonMissingActivityHeader = make_global<wstring>(L"Missing Activity Header");
GlobalWString RepairManagerServiceReplica::RejectReasonMissingTimeoutHeader = make_global<wstring>(L"Missing Timeout Header");
GlobalWString RepairManagerServiceReplica::RejectReasonIncorrectActor = make_global<wstring>(L"Incorrect Actor");
GlobalWString RepairManagerServiceReplica::RejectReasonInvalidAction = make_global<wstring>(L"Invalid Action");

bool RepairManagerServiceReplica::ValidateClientMessage(__in Transport::MessageUPtr & message, __out wstring & rejectReason)
{
    bool success = true;

    if (success)
    {
        FabricActivityHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *RejectReasonMissingActivityHeader;
        }
    }

    if (success)
    {
        TimeoutHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *RejectReasonMissingTimeoutHeader;
        }
    }

    if (success)
    {
        success = (message->Actor == Transport::Actor::RM);

        if (!success)
        {
            StringWriter(rejectReason).Write("{0}: {1}", RejectReasonIncorrectActor, message->Actor);
        }
    }

    return success;
}

void RepairManagerServiceReplica::RequestMessageHandler(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext)
{
    wstring rejectReason;
    if (!ValidateClientMessage(request, rejectReason))
    {
        RMEvents::Trace->RequestValidationFailed(
            this->PartitionedReplicaId,
            request->Action,
            request->MessageId,
            rejectReason);

        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        return;
    }

    auto timeout = TimeoutHeader::FromMessage(*request).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;
    auto messageId = request->MessageId;

    RMEvents::Trace->RequestReceived(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        request->Action,
        request->MessageId,
        timeout);

    BeginProcessRequest(
        move(request),
        move(receiverContext),
        activityId,
        timeout,
        [this, activityId, messageId](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessRequestComplete(activityId, messageId, asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginProcessRequest(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (request->Action == QueryTcpMessage::QueryAction)
    {
        return AsyncOperation::CreateAndStart<RepairManagerQueryMessageHandler>(
            *this,
            *queryMessageHandler_,
            move(request),
            move(receiverContext),
            timeout,
            callback,
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::CreateRepairRequestAction)
    {
        return AsyncOperation::CreateAndStart<CreateRepairRequestAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::CancelRepairRequestAction)
    {
        return AsyncOperation::CreateAndStart<CancelRepairRequestAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::ForceApproveRepairAction)
    {
        return AsyncOperation::CreateAndStart<ForceApproveRepairAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::DeleteRepairRequestAction)
    {
        return AsyncOperation::CreateAndStart<DeleteRepairRequestAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::UpdateRepairExecutionStateAction)
    {
        return AsyncOperation::CreateAndStart<UpdateRepairExecutionStateAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
    else if (request->Action == RepairManagerTcpMessage::UpdateRepairTaskHealthPolicyAction)
    {
        return AsyncOperation::CreateAndStart<UpdateRepairTaskHealthPolicyAsyncOperation>(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            callback,
            parent);
    }    
    else
    {
        // Unrecognized action; message is invalid

        RMEvents::Trace->RequestValidationFailed2(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            request->Action,
            request->MessageId,
            RejectReasonInvalidAction);

        return AsyncOperation::CreateAndStart<ClientRequestAsyncOperation>(
            *this,
            ErrorCodeValue::InvalidMessage,
            move(request),
            move(receiverContext),
            timeout,
            callback, 
            parent);
    }
}

ErrorCode RepairManagerServiceReplica::EndProcessRequest(
    AsyncOperationSPtr const & asyncOperation,
    MessageUPtr & reply,
    IpcReceiverContextUPtr & receiverContext)
{
    return ClientRequestAsyncOperation::End(asyncOperation, reply, receiverContext);
}

void RepairManagerServiceReplica::OnProcessRequestComplete(
    ActivityId const & activityId,
    MessageId const & messageId,
    AsyncOperationSPtr const & asyncOperation)
{
    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    auto error = EndProcessRequest(asyncOperation, reply, receiverContext);

    if (error.IsSuccess())
    {
        RMEvents::Trace->RequestSucceeded(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            messageId);

        reply->Headers.Replace(FabricActivityHeader(activityId));
        routingAgentProxy_.SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        RMEvents::Trace->RequestFailed(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            messageId,
            error);

        routingAgentProxy_.OnIpcFailure(error, *receiverContext, activityId);
    }
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginProcessQuery(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode RepairManagerServiceReplica::EndProcessQuery(
    AsyncOperationSPtr const & operation,
    MessageUPtr & reply)
{
    return ProcessQueryAsyncOperation::End(operation, reply);
}

ErrorCode RepairManagerServiceReplica::EndAcceptRequest(
    AsyncOperationSPtr const & asyncOperation)
{
    int64 unused;
    return EndAcceptRequest(asyncOperation, unused);
}

ErrorCode RepairManagerServiceReplica::EndAcceptRequest(
    AsyncOperationSPtr const & asyncOperation,
    int64 & commitVersion)
{
    return FinishAcceptRequestAsyncOperation::End(asyncOperation, commitVersion);
}

AsyncOperationSPtr RepairManagerServiceReplica::RejectRequest(
    ClientRequestSPtr && clientRequest,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (!error.IsSuccess())
    {
        RMEvents::Trace->RequestRejected(
            clientRequest->ReplicaActivityId,
            clientRequest->RequestMessageId,
            error);
    }

    return AsyncOperation::CreateAndStart<FinishAcceptRequestAsyncOperation>(
        *this,
        move(error),
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::FinishAcceptRequest(
    StoreTransaction && storeTx,
    shared_ptr<RepairTaskContext> && context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<FinishAcceptRequestAsyncOperation>(
        *this,
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::RejectInvalidMessage(
    ClientRequestSPtr && clientRequest,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->RejectRequest(move(clientRequest), ErrorCodeValue::InvalidMessage, callback, parent);
}

ErrorCode RepairManagerServiceReplica::ValidateImpact(RepairTask const & task) const
{    
    // impact validation isn't relevant in states other than Preparing 
    if (task.State != RepairTaskState::Preparing)
    {
        return ErrorCodeValue::Success;
    }

    if (task.Impact)
    {
        switch (task.Impact->Kind)
        {
        case RepairImpactKind::Node:
        {
            // SECURITY CRITICAL - Scope check
            if (task.Scope->Kind != RepairScopeIdentifierKind::Cluster)
            {
                return ErrorCodeValue::AccessDenied;
            }

            if (RepairManagerConfig::GetConfig().ValidateImpactedNodeNames ||
                task.Action == L"System.Manual")
            {
                auto nodeImpact = dynamic_pointer_cast<NodeRepairImpactDescription const>(task.Impact);
                vector<NodeImpact> const & impactList = nodeImpact->NodeImpactList;

                AcquireReadLock lock(lock_);
                for (auto it = impactList.begin(); it != impactList.end(); ++it)
                {
                    auto findResult = knownFabricNodes_.find(it->NodeName);
                    if (findResult == knownFabricNodes_.end())
                    {
                        return ErrorCodeValue::NodeNotFound;
                    }
                }
            }
        }
        break;

        default:
            // Unknown impact kind
            return ErrorCodeValue::InvalidArgument;
        }
    }

    return ErrorCode::Success();
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptCreateRepairRequest(
    RepairTask && task,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (!RepairManagerConfig::GetConfig().AllowNewTasks)
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::NotReady, callback, parent);
    }

    auto error = ValidateRepairTaskHealthCheckFlags(task.PerformPreparingHealthCheck, task.PerformRestoringHealthCheck);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} health check flags validation failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    // Validate the incoming task and prepare it for insertion
    error = task.TryPrepareForCreate();
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} TryPrepareForCreate failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    // If impact is specified, it requires additional validation
    error = ValidateImpact(task);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ValidateImpact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    ChargeTask charge(*this, clientRequest->ReplicaActivityId);
    if (!charge.ChargeSucceeded())
    {
        WriteInfo(TraceComponent, "{0} Service is too busy to handle this request", clientRequest->ReplicaActivityId);
        return RejectRequest(move(clientRequest), ErrorCodeValue::ServiceTooBusy, callback, parent);
    }

    auto context = make_unique<RepairTaskContext>(move(task));

    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    // Insert, or fail if a task with the same ID already exists
    bool inserted(false);
    error = storeTx.InsertIfNotFound(*context, inserted);

    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} InsertIfNotFound failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    if (!inserted)
    {
        WriteInfo(TraceComponent, "{0} Repair task already exists: id = {1}", clientRequest->ReplicaActivityId, context->TaskId);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskAlreadyExists, callback, parent);
    }

    // Commit the charges to the active/total task counts
    charge.Commit();

    // TODO replace with a structured event
    WriteInfo(TraceComponent,
        "{0} CreateRepairRequest: {1}",
        clientRequest->ReplicaActivityId,
        context->Task);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptCancelRepairRequest(
    RepairScopeIdentifierBase const & operationScope,
    wstring const & taskId,
    int64 const expectedVersion,
    bool const requestAbort,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    auto context = make_unique<RepairTaskContext>(RepairTask(taskId));

    // Read repair task context from the store
    auto error = storeTx.ReadExact(*context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    RepairTask & task = context->Task;
    auto oldState = task.State;

    // SECURITY CRITICAL - Scope check
    if (!task.IsVisibleTo(operationScope))
    {
        WriteInfo(TraceComponent,
            "{0} Task {1} (scope={2}) is not visible to scope {3}",
            clientRequest->ReplicaActivityId,
            task.TaskId,
            task.Scope,
            operationScope);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }

    // Optimistic concurrency check
    if (!task.CheckExpectedVersion(expectedVersion))
    {
        WriteInfo(TraceComponent,
            "{0} CheckExpectedVersion failed: expected = {1}, actual = {2}",
            clientRequest->ReplicaActivityId,
            expectedVersion,
            task.Version);

        return RejectRequest(move(clientRequest), ErrorCodeValue::StaleRequest, callback, parent);
    }

    // Validate and perform state transition
    error = task.TryPrepareForCancel(requestAbort);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} TryPrepareForCancel failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    error = storeTx.Update(*context);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} Update failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    if (task.IsCompleted)
    {
        CreditCompletedTask(storeTx.ReplicaActivityId);
    }

    // TODO replace with a structured event
    WriteInfo(TraceComponent,
        "{0} CancelRepairRequest: id = {1}, old state = {2}, new state = {3}",
        clientRequest->ReplicaActivityId,
        context->TaskId,
        oldState,
        context->TaskState);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptForceApproveRepair(
    RepairScopeIdentifierBase const & operationScope,
    wstring const & taskId,
    int64 const expectedVersion,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    auto context = make_unique<RepairTaskContext>(RepairTask(taskId));

    // Read repair task context from the store
    auto error = storeTx.ReadExact(*context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    RepairTask & task = context->Task;
    auto oldState = task.State;

    // SECURITY CRITICAL - Scope check
    if (!task.IsVisibleTo(operationScope))
    {
        WriteInfo(TraceComponent,
            "{0} Task {1} (scope={2}) is not visible to scope {3}",
            clientRequest->ReplicaActivityId,
            task.TaskId,
            task.Scope,
            operationScope);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }

    // Optimistic concurrency check
    if (!task.CheckExpectedVersion(expectedVersion))
    {
        WriteInfo(TraceComponent,
            "{0} CheckExpectedVersion failed: expected = {1}, actual = {2}",
            clientRequest->ReplicaActivityId,
            expectedVersion,
            task.Version);

        return RejectRequest(move(clientRequest), ErrorCodeValue::StaleRequest, callback, parent);
    }

    // Validate and perform state transition
    error = task.TryPrepareForForceApprove();
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} TryPrepareForForceApprove failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    error = storeTx.Update(*context);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} Update failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    // No charge/credit needed, task is still active

    // TODO replace with a structured event
    WriteInfo(TraceComponent,
        "{0} ForceApproveRepair: id = {1}, old state = {2}, new state = {3}",
        clientRequest->ReplicaActivityId,
        context->TaskId,
        oldState,
        context->TaskState);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptDeleteRepairRequest(
    RepairScopeIdentifierBase const & operationScope,
    wstring const & taskId,
    int64 const expectedVersion,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    auto context = make_unique<RepairTaskContext>(RepairTask(taskId));

    // Read repair task context from the store
    auto error = storeTx.ReadExact(*context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    RepairTask & task = context->Task;

    // SECURITY CRITICAL - Scope check
    if (!task.IsVisibleTo(operationScope))
    {
        WriteInfo(TraceComponent,
            "{0} Task {1} (scope={2}) is not visible to scope {3}",
            clientRequest->ReplicaActivityId,
            task.TaskId,
            task.Scope,
            operationScope);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }

    // Optimistic concurrency check
    if (!task.CheckExpectedVersion(expectedVersion))
    {
        WriteInfo(TraceComponent,
            "{0} CheckExpectedVersion failed: expected = {1}, actual = {2}",
            clientRequest->ReplicaActivityId,
            expectedVersion,
            task.Version);

        return RejectRequest(move(clientRequest), ErrorCodeValue::StaleRequest, callback, parent);
    }

    if (!task.IsCompleted)
    {
        WriteInfo(TraceComponent, "{0} Repair task is still active: id = {1}", clientRequest->ReplicaActivityId, task.TaskId);
        return RejectRequest(move(clientRequest), ErrorCodeValue::InvalidState, callback, parent);
    }

    error = storeTx.Delete(*context);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} Delete failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    CreditDeletedTask(storeTx.ReplicaActivityId);

    // TODO replace with a structured event
    WriteInfo(TraceComponent,
        "{0} DeleteRepairRequest: {1}",
        clientRequest->ReplicaActivityId,
        context->Task);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptUpdateRepairExecutionState(
    RepairTask const & updatedTask,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    auto context = make_unique<RepairTaskContext>(RepairTask(updatedTask.TaskId));

    // Read repair task context from the store
    auto error = storeTx.ReadExact(*context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    RepairTask & currentTask = context->Task;
    RepairTaskState::Enum currentTaskOriginalState = currentTask.State;

    // SECURITY CRITICAL - Scope check
    if (!updatedTask.Scope)
    {
        return RejectInvalidMessage(move(clientRequest), callback, parent);
    }
    else if (!currentTask.IsVisibleTo(*updatedTask.Scope))
    {
        WriteInfo(TraceComponent,
            "{0} Task {1} (scope={2}) is not visible to scope {3}",
            clientRequest->ReplicaActivityId,
            currentTask.TaskId,
            currentTask.Scope,
            updatedTask.Scope);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }

    // Optimistic concurrency check
    if (!currentTask.CheckExpectedVersion(updatedTask.Version))
    {
        WriteInfo(TraceComponent,
            "{0} CheckExpectedVersion failed: expected = {1}, actual = {2}",
            clientRequest->ReplicaActivityId,
            updatedTask.Version,
            currentTask.Version);

        return RejectRequest(move(clientRequest), ErrorCodeValue::StaleRequest, callback, parent);
    }

    // Validate and perform state transition
    error = currentTask.TryPrepareForUpdate(updatedTask);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} TryPrepareForUpdate failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    // If impact is specified, it requires additional validation
    error = ValidateImpact(currentTask);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ValidateImpact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    error = storeTx.Update(*context);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} Update failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    // check original state to not double-credit the task
    if (currentTask.IsCompleted && currentTaskOriginalState != RepairTaskState::Completed)
    {
        CreditCompletedTask(storeTx.ReplicaActivityId);
    }

    // TODO replace with a structured event
    WriteInfo(TraceComponent,
        "{0} UpdateRepairExecutionState: {1}",
        clientRequest->ReplicaActivityId,
        context->Task);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RepairManagerServiceReplica::BeginAcceptUpdateRepairTaskHealthPolicy(
    RepairScopeIdentifierBase const & operationScope,
    wstring const & taskId,
    int64 const expectedVersion,    
    std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
    std::shared_ptr<bool> const & performRestoringHealthCheckSPtr,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(
        TraceComponent, 
        "{0} BeginAcceptUpdateRepairTaskHealthPolicy begin, id = {1}, expectedVersion: {2}, performPreparingHealthCheckSPtr: {3}, performRestoringHealthCheckSPtr: {4}",
        clientRequest->ReplicaActivityId, 
        taskId, 
        expectedVersion, 
        performPreparingHealthCheckSPtr ? wformatString("{0}", *performPreparingHealthCheckSPtr) : L"null", 
        performRestoringHealthCheckSPtr ? wformatString("{0}", *performRestoringHealthCheckSPtr) : L"null");
    
    auto storeTx = CreateTransaction(clientRequest->ActivityId);

    auto context = make_unique<RepairTaskContext>(RepairTask(taskId));

    // Read repair task context from the store
    auto error = storeTx.ReadExact(*context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} ReadExact failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    error = ValidateRepairTaskHealthCheckFlags(
        performPreparingHealthCheckSPtr ? *performPreparingHealthCheckSPtr : false, 
        performRestoringHealthCheckSPtr ? *performRestoringHealthCheckSPtr : false);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} health check flags validation failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    RepairTask & task = context->Task;

    // SECURITY CRITICAL - Scope check
    if (!task.IsVisibleTo(operationScope))
    {
        WriteInfo(TraceComponent,
            "{0} Task {1} (scope={2}) is not visible to scope {3}",
            clientRequest->ReplicaActivityId,
            task.TaskId,
            task.Scope,
            operationScope);
        return RejectRequest(move(clientRequest), ErrorCodeValue::RepairTaskNotFound, callback, parent);
    }

    // Optimistic concurrency check
    if (!task.CheckExpectedVersion(expectedVersion))
    {
        WriteInfo(TraceComponent,
            "{0} CheckExpectedVersion failed: expected = {1}, actual = {2}",
            clientRequest->ReplicaActivityId,
            expectedVersion,
            task.Version);

        return RejectRequest(move(clientRequest), ErrorCodeValue::StaleRequest, callback, parent);
    }
    
    // Validate and perform state transition
    error = task.TryPrepareForUpdateHealthPolicy(performPreparingHealthCheckSPtr, performRestoringHealthCheckSPtr);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} TryPrepareForUpdateHealthPolicy failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    error = storeTx.Update(*context);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} Update failed with {1}", clientRequest->ReplicaActivityId, error);
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        timeout,
        callback,
        parent);
}

ErrorCode RepairManagerServiceReplica::ProcessQuery(
    QueryNames::Enum queryName, 
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    __out MessageUPtr & reply)
{
    ReplicaActivityId replicaActivityId(this->PartitionedReplicaId, activityId);

    std::unique_ptr<QueryResult> queryResult;
    switch (queryName) 
    {
    case QueryNames::GetRepairList:
        queryResult = Common::make_unique<QueryResult>(std::move(GetRepairList(queryArgs, replicaActivityId)));
        break;

    default:
        reply = nullptr;
        return ErrorCodeValue::InvalidConfiguration;
    }

    ErrorCode result = queryResult->QueryProcessingError;
    reply = move(RepairManagerTcpMessage::GetClientOperationSuccess(std::move(queryResult))->GetTcpMessage());
    return result;
}

QueryResult RepairManagerServiceReplica::GetRepairList(
    QueryArgumentMap const & queryArgs,
    ReplicaActivityId const & replicaActivityId)
{
    wstring scopeString;
    wstring taskIdPrefix;
    wstring stateMaskString;
    wstring executorName;

    if (!queryArgs.TryGetValue(QueryResourceProperties::Repair::Scope, scopeString) || 
        !queryArgs.TryGetValue(QueryResourceProperties::Repair::TaskIdPrefix, taskIdPrefix) ||
        !queryArgs.TryGetValue(QueryResourceProperties::Repair::StateMask, stateMaskString) ||
        !queryArgs.TryGetValue(QueryResourceProperties::Repair::ExecutorName, executorName))
    {
        // Query gateway should have already enforced these as required parameters
        Assert::TestAssert("Missing a query parameter that should have been marked as required");
        return QueryResult(ErrorCodeValue::InvalidArgument);
    }

    RepairTaskState::Enum stateMask = RepairTaskState::Invalid;
    
    auto error = RepairTaskState::ParseStateMask(stateMaskString, stateMask);
    if (!error.IsSuccess())
    {
        return QueryResult(error);
    }

    if (stateMask == RepairTaskState::Invalid)
    {
        // By default, include all states
        stateMask = static_cast<RepairTaskState::Enum>(FABRIC_REPAIR_TASK_STATE_FILTER_ALL);
    }

    RepairScopeIdentifierBaseSPtr scope;
    error = JsonHelper::Deserialize(scope, scopeString);
    if (!error.IsSuccess())
    {
        return QueryResult(error);
    }
    else if (!scope)
    {
        return QueryResult(ErrorCodeValue::InvalidArgument);
    }

    vector<RepairTask> resultList;
    error = GetRepairList(*scope, resultList, taskIdPrefix, stateMask, executorName, replicaActivityId);
    if (!error.IsSuccess())
    {
        return QueryResult(error);
    }

    return QueryResult(move(resultList));
}

ErrorCode RepairManagerServiceReplica::GetRepairList(
    RepairScopeIdentifierBase const & operationScope,
    vector<RepairTask> & resultList,
    wstring const & taskIdPrefix,
    RepairTaskState::Enum const stateMask,
    wstring const & executorName,
    ReplicaActivityId const & replicaActivityId)
{
    bool filterByState = (stateMask != RepairTaskState::Invalid);
    bool filterByExecutor = !executorName.empty();

    auto storeTx = CreateTransaction(replicaActivityId.ActivityId);

    // Read from the store, filtering by task ID prefix.
    vector<RepairTaskContext> contexts;
    auto error = storeTx.ReadPrefix<RepairTaskContext>(
        Constants::StoreType_RepairTaskContext,
        taskIdPrefix,
        contexts);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        storeTx.Rollback();
        return error;
    }

    // Extract the RepairTask objects from the returned contexts,
    // and filter by state and executor.
    for (auto it = contexts.begin(); it != contexts.end(); ++it)
    {
        RepairTask & task = it->Task;

        // SECURITY CRITICAL - Scope check
        if (!task.IsVisibleTo(operationScope))
        {
            continue;
        }

        if (filterByState && ((task.State & stateMask) == 0))
        {
            continue;
        }

        if (filterByExecutor && (task.Executor != executorName))
        {
            continue;
        }

        resultList.push_back(move(task));
    }

    return ErrorCode::Success();
}

StoreTransaction RepairManagerServiceReplica::CreateTransaction(ActivityId const & activityId) const
{
    return StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, activityId);
}

bool RepairManagerServiceReplica::TryChargeNewTask(ReplicaActivityId const & activityId)
{
    RepairManagerConfig & config = RepairManagerConfig::GetConfig();

    uint maxActive = static_cast<uint>(max(0, config.MaxActiveTasks));
    uint maxTotal = static_cast<uint>(max(0, config.MaxTotalTasks));

    AcquireWriteLock lock(lock_);

    if (activeRepairTaskCount_ >= maxActive)
    {
        return false;
    }

    if (totalRepairTaskCount_ >= maxTotal)
    {
        return false;
    }

    ++activeRepairTaskCount_;
    ++totalRepairTaskCount_;

    WriteInfo(
        TraceComponent,
        "{0} TryChargeNewTask: active = {1}, total = {2}",
        activityId,
        activeRepairTaskCount_,
        totalRepairTaskCount_);

    return true;
}

void RepairManagerServiceReplica::CreditTask(ReplicaActivityId const & activityId, bool creditActive, bool creditTotal)
{
    AcquireWriteLock lock(lock_);

    if (creditActive)
    {
        ASSERT_IFNOT(activeRepairTaskCount_ > 0, "CreditTask: activeRepairTaskCount_ = {0}", activeRepairTaskCount_);
        --activeRepairTaskCount_;
    }

    ASSERT_IFNOT(totalRepairTaskCount_ > 0, "CreditTask: totalRepairTaskCount_ = {0}", totalRepairTaskCount_);
    if (creditTotal)
    {
        --totalRepairTaskCount_;
    }

    WriteInfo(
        TraceComponent,
        "{0} CreditTask: active = {1}, total = {2}",
        activityId,
        activeRepairTaskCount_,
        totalRepairTaskCount_);
}

void RepairManagerServiceReplica::CreditCompletedTask(ReplicaActivityId const & activityId)
{
    CreditTask(activityId, true, false);
}

void RepairManagerServiceReplica::CreditDeletedTask(ReplicaActivityId const & activityId)
{
    CreditTask(activityId, false, true);
}

void RepairManagerServiceReplica::Start()
{
    AcquireWriteLock lock(lock_);

    WriteInfo(TraceComponent, "{0} Start(): current state = {1}", TraceId, state_);

    switch (state_)
    {
        case RepairManagerState::Stopped:
            this->StartRecovery();
            break;

        case RepairManagerState::Stopping:
            this->SetState(RepairManagerState::StoppingPendingRestart);
            break;

        default:
            Assert::TestAssert("{0} invalid state in Start(): {1}", this->TraceId, state_);
            this->SetState(RepairManagerState::Closed);
            break;
    }   // end switch (state)
}

void RepairManagerServiceReplica::Stop()
{
    AcquireWriteLock lock(lock_);

    WriteInfo(TraceComponent, "{0} Stop(): current state = {1}", TraceId, state_);

    switch (state_)
    {
        case RepairManagerState::Starting:
        case RepairManagerState::Started:
        case RepairManagerState::StoppingPendingRestart:

            if (outstandingOperations_ > 0)
            {
                this->SetState(RepairManagerState::Stopping);
            }
            else
            {
                this->SetState(RepairManagerState::Stopped);
            }
            break;

        case RepairManagerState::Stopped:
        case RepairManagerState::Stopping:
        case RepairManagerState::Closed:
            // nothing to do
            break;

        default:
            Assert::TestAssert("{0} invalid state in Stop(): {1}", this->TraceId, state_);
            this->SetState(RepairManagerState::Closed);
            break;
    }   // end switch (state)
}

// expects caller to hold lock
void RepairManagerServiceReplica::SetState(RepairManagerState::Enum const newState)
{
    WriteInfo(TraceComponent, "{0} SetState(): {1} -> {2}", TraceId, state_, newState);
    state_ = newState;
}

RepairManagerState::Enum RepairManagerServiceReplica::get_State() const
{
    AcquireReadLock lock(lock_);
    return state_;
}

ErrorCode RepairManagerServiceReplica::BeginOperation(ReplicaActivityId const & activityId)
{
    AcquireWriteLock lock(lock_);

    if (state_ != RepairManagerState::Started)
    {
        return ErrorCodeValue::NotPrimary;
    }

    this->BeginOperationInternal(activityId);

    return ErrorCode::Success();
}

void RepairManagerServiceReplica::EndOperation(ReplicaActivityId const & activityId)
{
    AcquireWriteLock lock(lock_);
    this->EndOperationInternal(activityId);
}

// expects caller to hold lock
void RepairManagerServiceReplica::BeginOperationInternal(ReplicaActivityId const & activityId)
{
    ASSERT_IFNOT(
        state_ == RepairManagerState::Starting || state_ == RepairManagerState::Started,
        "BeginOperation: cannot start operations in state {0}",
        state_);

    ++outstandingOperations_;

    WriteNoise(TraceComponent,
        "{0} BeginOperation: outstanding operation count = {1}",
        activityId,
        outstandingOperations_);
}

// expects caller to hold lock
void RepairManagerServiceReplica::EndOperationInternal(ReplicaActivityId const & activityId)
{
    ASSERT_IFNOT(outstandingOperations_ > 0, "EndOperation: outstandingOperations_ = {0}", outstandingOperations_);

    --outstandingOperations_;

    WriteNoise(TraceComponent,
        "{0} EndOperation: outstanding operation count = {1}",
        activityId,
        outstandingOperations_);

    if (outstandingOperations_ == 0)
    {
        if (state_ == RepairManagerState::Stopping)
        {
            this->SetState(RepairManagerState::Stopped);
        }
        else if (state_ == RepairManagerState::StoppingPendingRestart)
        {
            this->StartRecovery();
        }
    }
}

// expects caller to hold lock
void RepairManagerServiceReplica::StartRecovery()
{
    this->SetState(RepairManagerState::Starting);

    // Start immediately, and generate a new activity ID for each entry into Starting
    this->ScheduleRecovery(
        TimeSpan::Zero,
        ReplicaActivityId(this->PartitionedReplicaId, ActivityId()));
}

// expects caller to hold lock
void RepairManagerServiceReplica::ScheduleRecovery(TimeSpan const delay, ReplicaActivityId const & activityId)
{
    ASSERT_IFNOT(state_ == RepairManagerState::Starting, "ScheduleRecovery: unexpected state {0}", state_);

    WriteInfo(TraceComponent, "{0} ScheduleRecovery: delay = {1}", activityId, delay);
    this->BeginOperationInternal(activityId);

    auto root = this->CreateAsyncOperationRoot();

    recoveryTimer_ = Timer::Create(
        TimerTag_Recovery,
        [this, root, activityId](TimerSPtr const & timer)
        {
            timer->Cancel();
            this->RecoverNodeList(activityId);
        },
        true); // allow concurrency

    recoveryTimer_->Change(delay);
}

void RepairManagerServiceReplica::RecoverNodeList(ReplicaActivityId const & activityId)
{
    // TODO continuation token
    BeginGetNodeList(
        [this, activityId](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnRecoverNodeListComplete(activityId, asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

void RepairManagerServiceReplica::OnRecoverNodeListComplete(
    ReplicaActivityId const & activityId,
    AsyncOperationSPtr const & operation)
{
    auto error = OnGetNodeListComplete(activityId, operation);

    if (error.IsSuccess())
    {
        this->RecoverState(activityId);
    }
    else
    {
        this->FinishRecovery(activityId, error);
    }
}

void RepairManagerServiceReplica::BeginGetNodeList(
    Common::AsyncCallback const& callback,
    Common::AsyncOperationSPtr const& parent)
{
    queryClient_->BeginGetNodeList(
        L"",
        L"",
        RepairManagerConfig::GetConfig().MaxCommunicationTimeout,
        callback,
        parent);
}

ErrorCode RepairManagerServiceReplica::OnGetNodeListComplete(
    ReplicaActivityId const & activityId,
    AsyncOperationSPtr const & operation)
{
    vector<NodeQueryResult> nodeQueryResultList;
    PagingStatusUPtr pagingStatus;
    auto error = queryClient_->EndGetNodeList(operation, nodeQueryResultList, pagingStatus);

    // TODO: if paging status not null, should ask for next page

    if (error.IsSuccess())
    {
        set<wstring> validNodes;
        for (auto it = nodeQueryResultList.begin(); it != nodeQueryResultList.end(); ++it)
        {
            validNodes.insert(it->NodeName);
        }

        WriteInfo(TraceComponent, "{0} OnGetNodeListComplete: node count = {1}", activityId, validNodes.size());

        for (auto it = validNodes.begin(); it != validNodes.end(); ++it)
        {
            WriteNoise(TraceComponent, "{0} OnGetNodeListComplete: node name = {1}", activityId, *it);
        }

        {
            AcquireWriteLock lock(lock_);
            knownFabricNodes_.swap(validNodes);
        }
    }
    else
    {
        WriteWarning(TraceComponent, "{0} OnGetNodeListComplete: GetNodeList failed: error = {1}", activityId, error);
    }

    return error;
}

void RepairManagerServiceReplica::RecoverState(ReplicaActivityId const & activityId)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, activityId.ActivityId);

    WriteInfo(TraceComponent, "{0} RecoverState: recovery started", activityId);

    // Read all contexts
    vector<RepairTaskContext> contexts;
    auto error = storeTx.ReadPrefix<RepairTaskContext>(
        Constants::StoreType_RepairTaskContext,
        L"",
        contexts);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();

        busyTasks_.clear();

        activeRepairTaskCount_ = 0;
        totalRepairTaskCount_ = static_cast<uint>(contexts.size());

        for (auto it = contexts.begin(); it != contexts.end(); ++it)
        {
            if (!it->Task.IsCompleted)
            {
                ++activeRepairTaskCount_;
            }

            if (this->TaskNeedsProcessing(*it))
            {
                auto contextSPtr = make_shared<RepairTaskContext>(move(*it));
                this->EnqueueTask(move(contextSPtr), activityId);
            }
        }

        WriteInfo(TraceComponent,
            "{0} RecoverState: recovery completed (active = {1}, total = {2})",
            activityId,
            activeRepairTaskCount_,
            totalRepairTaskCount_);
    }
    else
    {
        storeTx.Rollback();
        WriteWarning(TraceComponent, "{0} RecoverState: ReadPrefix failed: error = {1}", activityId, error);
    }

    this->FinishRecovery(activityId, error);
}

void RepairManagerServiceReplica::FinishRecovery(ReplicaActivityId const & activityId, ErrorCode const & error)
{
    AcquireWriteLock lock(lock_);

    this->EndOperationInternal(activityId);

    if (state_ == RepairManagerState::Starting)
    {
        if (error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} FinishRecovery: recovery succeeded", activityId);
            this->SetState(RepairManagerState::Started);
            this->ScheduleBackgroundProcessing(TimeSpan::Zero, activityId);
            this->ScheduleHealthCheck(RepairManagerConfig::GetConfig().HealthCheckInterval, activityId);
            this->ScheduleCleanup(RepairManagerConfig::GetConfig().CleanupInterval, activityId);
        }
        else
        {
            WriteWarning(TraceComponent, "{0} FinishRecovery: recovery failed (error = {1}); retrying", activityId, error);
            this->ScheduleRecovery(this->GetRandomizedOperationRetryDelay(error), activityId);
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0} FinishRecovery: recovery cancelled, state = {1}", activityId, state_);
    }
}

bool RepairManagerServiceReplica::TaskNeedsProcessing(RepairTaskContext const & context) const
{
    auto state = context.TaskState;
    return (state == RepairTaskState::Preparing || state == RepairTaskState::Restoring);
}

void RepairManagerServiceReplica::TryEnqueueTask(shared_ptr<RepairTaskContext> && context, ReplicaActivityId const & activityId)
{
    if (!this->TaskNeedsProcessing(*context))
    {
        // Not a state that the RM needs to process; ignore this update
        return;
    }

    AcquireWriteLock lock(lock_);

    if (state_ != RepairManagerState::Started)
    {
        // RM is stopping; do not enqueue more work
        return;
    }

    this->EnqueueTask(move(context), activityId);
    this->ScheduleBackgroundProcessing(TimeSpan::Zero, activityId);    
}

// caller holds lock
void RepairManagerServiceReplica::EnqueueTask(shared_ptr<RepairTaskContext> && context, ReplicaActivityId const & activityId)
{
    auto iter = busyTasks_.find(context->TaskId);
    if (iter == busyTasks_.end())
    {
        WriteInfo(TraceComponent, "{0} EnqueueTask: added new task: id = {1}, state = {2}",
            activityId,
            context->TaskId,
            context->TaskState);

        busyTasks_[context->TaskId] = move(context);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} EnqueueTask: refreshing task: id = {1}, state = {2}",
            activityId,
            context->TaskId,
            context->TaskState);

        iter->second->NeedsRefresh = true;
    }
}

void RepairManagerServiceReplica::TryRemoveTask(
    RepairTaskContext const & context,
    ReplicaActivityId const & activityId)
{
    AcquireWriteLock lock(lock_);

    if (context.NeedsRefresh)
    {
        // Do not remove from map, task has already been updated
        WriteInfo(TraceComponent, "{0} TryRemoveTask: task requires refresh: id = {1}, state = {2}",
            activityId,
            context.TaskId,
            context.TaskState);
    }
    else
    {
        auto iter = busyTasks_.find(context.TaskId);
        if (iter == busyTasks_.end())
        {
            WriteWarning(TraceComponent, "{0} TryRemoveTask: task not found: id = {1}, state = {2}",
                activityId,
                context.TaskId,
                context.TaskState);
        }
        else
        {
            busyTasks_.erase(iter);

            WriteInfo(TraceComponent, "{0} TryRemoveTask: removed task: id = {1}, state = {2}",
                activityId,
                context.TaskId,
                context.TaskState);
        }
    }
}

void RepairManagerServiceReplica::SnapshotTasks(vector<shared_ptr<RepairTaskContext>> & tasks)
{
    AcquireReadLock lock(lock_);

    vector<shared_ptr<RepairTaskContext>> result;
    result.reserve(busyTasks_.size());

    for (auto it = busyTasks_.begin(); it != busyTasks_.end(); ++it)
    {
        result.push_back(it->second);
    }

    tasks = move(result);
}

// caller holds lock
void RepairManagerServiceReplica::ScheduleCleanup(TimeSpan const delay, ReplicaActivityId const & parentActivityId)
{
    if (!cleanupTimer_)
    {
        ReplicaActivityId newActivityId(this->PartitionedReplicaId, ActivityId());

        WriteInfo(TraceComponent, "{0} ScheduleCleanup: delay = {1}, new activity = {2}",
            parentActivityId,
            delay,
            newActivityId.ActivityId);

        auto root = this->CreateAsyncOperationRoot();

        cleanupTimer_ = Timer::Create(
            TimerTag_Background,
            [this, root, newActivityId](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->RunCleanup(newActivityId);
            },
            true); // allow concurrency

        cleanupTimer_->Change(delay);
    }
}

void RepairManagerServiceReplica::RunCleanup(ReplicaActivityId const & activityId)
{
    AsyncOperation::CreateAndStart<CleanupAsyncOperation>(
        *this,
        activityId,
        [this, activityId](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnCleanupCompleted(activityId, asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

void RepairManagerServiceReplica::OnCleanupCompleted(
    ReplicaActivityId const & activityId,
    AsyncOperationSPtr const & operation)
{
    auto error = AsyncOperation::End<CleanupAsyncOperation>(operation)->Error;

    AcquireWriteLock lock(lock_);

    WriteInfo(TraceComponent,
        "{0} OnCleanupCompleted: background processing completed: error = {1}, state = {2}",
        activityId,
        error,
        state_);

    cleanupTimer_ = nullptr;

    if (state_ == RepairManagerState::Started)
    {
        auto delay = RepairManagerConfig::GetConfig().CleanupInterval;
        this->ScheduleCleanup(delay, activityId);
    }
}

// caller holds lock
void RepairManagerServiceReplica::ScheduleBackgroundProcessing(TimeSpan const delay, ReplicaActivityId const & parentActivityId)
{
    if (!backgroundProcessingTimer_)
    {
        ReplicaActivityId newActivityId(this->PartitionedReplicaId, ActivityId());

        WriteInfo(TraceComponent, "{0} ScheduleBackgroundProcessing: delay = {1}, new activity = {2}",
            parentActivityId,
            delay,
            newActivityId.ActivityId);

        auto root = this->CreateAsyncOperationRoot();

        backgroundProcessingTimer_ = Timer::Create(
            TimerTag_Background,
            [this, root, newActivityId](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->ProcessTasks(newActivityId);
            },
            true); // allow concurrency

        backgroundProcessingTimer_->Change(delay);
    }
}

void RepairManagerServiceReplica::ProcessTasks(ReplicaActivityId const & activityId)
{
    vector<shared_ptr<RepairTaskContext>> snapshot;
    this->SnapshotTasks(snapshot);

    AsyncOperation::CreateAndStart<ProcessRepairTaskContextsAsyncOperation>(
        *this,
        move(snapshot),
        activityId,
        [this, activityId](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessTasksCompleted(activityId, asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

void RepairManagerServiceReplica::OnProcessTasksCompleted(
    ReplicaActivityId const & activityId,
    AsyncOperationSPtr const & operation)
{
    auto error = AsyncOperation::End<ProcessRepairTaskContextsAsyncOperation>(operation)->Error;

    AcquireWriteLock lock(lock_);

    WriteInfo(TraceComponent,
        "{0} OnProcessTasksCompleted: background processing completed: error = {1}, state = {2}, task count = {3}",
        activityId,
        error,
        state_,
        busyTasks_.size());

    backgroundProcessingTimer_ = nullptr;

    if (state_ == RepairManagerState::Started && !busyTasks_.empty())
    {
        auto delay = RepairManagerConfig::GetConfig().RepairTaskProcessingInterval;
        this->ScheduleBackgroundProcessing(delay, activityId);
    }
}

void RepairManagerServiceReplica::ScheduleHealthCheck(TimeSpan const delay, ReplicaActivityId const & parentActivityId)
{
    if (!healthCheckTimer_)
    {
        ReplicaActivityId newActivityId(this->PartitionedReplicaId, ActivityId());

        WriteInfo(TraceComponent, "{0} ScheduleHealthCheck: start, delay = {1}, new activity = {2}",
            parentActivityId,
            delay,
            newActivityId.ActivityId);

        auto root = this->CreateAsyncOperationRoot();

        healthCheckTimer_ = Timer::Create(
            TimerTag_Background,
            [this, root, newActivityId](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->ProcessHealthCheck(newActivityId);
            },
            true); // allow concurrency

        healthCheckTimer_->Change(delay);
    }
}

void RepairManagerServiceReplica::UpdateHealthLastQueriedTime()
{
    AcquireWriteLock lock(lock_);
    healthLastQueriedAt_ = DateTime::Now();
}

DateTime RepairManagerServiceReplica::GetHealthLastQueriedTime() const
{
    AcquireReadLock lock(lock_);
    return healthLastQueriedAt_;
}

void RepairManagerServiceReplica::ProcessHealthCheck(ReplicaActivityId const & activityId)
{
    AsyncOperation::CreateAndStart<HealthCheckAsyncOperation>(
        *this,        
        activityId,
        [this, activityId](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessHealthCheckCompleted(activityId, asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

void RepairManagerServiceReplica::OnProcessHealthCheckCompleted(
    ReplicaActivityId const & activityId,
    AsyncOperationSPtr const & operation)
{
    auto error = AsyncOperation::End<HealthCheckAsyncOperation>(operation)->Error;

    AcquireWriteLock lock(lock_);

    WriteInfo(TraceComponent,
        "{0} OnProcessHealthCheckCompleted: health check completed: error = {1}",
        activityId,
        error);

    healthCheckTimer_ = nullptr;

    if (state_ == RepairManagerState::Started)
    {
        auto delay = RepairManagerConfig::GetConfig().HealthCheckInterval;
        this->ScheduleHealthCheck(delay, activityId);
    }
}

/// <remarks>
/// This overload is used by the repair task (e.g. RepairPreparationAsyncOperation and RepairRestorationAsyncOperation).
/// When they call this method, the healthLastQueriedAt_ is updated. This decides if health checks are needed in the 
/// replica.
/// </remarks>
ErrorCode RepairManagerServiceReplica::GetHealthCheckStoreData(
    __in ReplicaActivityId const & replicaActivityId,
    __out HealthCheckStoreData & healthCheckStoreData)
{
    auto storeTx = CreateTransaction(replicaActivityId.ActivityId);

    UpdateHealthLastQueriedTime();

    auto error = GetHealthCheckStoreData(storeTx, replicaActivityId, healthCheckStoreData);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        storeTx.Rollback();
    }

    return error;
}

/// <remarks>
/// This overload doesn't update the last queried time. This is invoked by the replica when it is doing
/// health checks to first read the existing store data before modifying and dumping it back to the persisted store.
/// </remarks>
ErrorCode RepairManagerServiceReplica::GetHealthCheckStoreData(
    __in StoreTransaction const & storeTx,
    __in ReplicaActivityId const & replicaActivityId, 
    __out HealthCheckStoreData & healthCheckStoreData)
{
    auto error = storeTx.ReadExact(healthCheckStoreData);

    if (error.IsSuccess() || error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} {1}: LastErrorAt: {2}, LastHealthyAt: {3}",
            replicaActivityId,
            error.IsError(ErrorCodeValue::NotFound) ? L"Initialized default health check store data" : L"Obtained health check data from store",
            healthCheckStoreData.LastErrorAt,
            healthCheckStoreData.LastHealthyAt);

        return ErrorCode::Success();
    }

    WriteWarning(TraceComponent, "{0} ReadExact failed getting health check store data: error = {1}", replicaActivityId, error);
    return error;
}

ErrorCode RepairManagerServiceReplica::ValidateRepairTaskHealthCheckFlags(
    __in bool performPreparingHealthCheck,
    __in bool performRestoringHealthCheck)
{
    // any repair task health check flag setting is okay if RM wide health checks are enabled
    if (RepairManagerConfig::GetConfig().EnableHealthChecks)
    {
        return ErrorCode::Success();
    }

    // if RM wide health checks are disabled, then health check flags cannot be set on the repair task
    return (performPreparingHealthCheck || performRestoringHealthCheck) ? ErrorCodeValue::InvalidState : ErrorCode::Success();
}
