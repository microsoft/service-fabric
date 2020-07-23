// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Federation;
using namespace Hosting2;
using namespace Query;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;
using namespace Management;
using namespace Naming;
using namespace Management::BackupRestoreService;
using namespace Management::BackupRestoreAgentComponent;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("BackupRestoreServiceAgent");

class BackupRestoreServiceAgent::RequestReplyAsyncOperationBase
    : public Common::TimedAsyncOperation
    , public Common::TextTraceComponent<Common::TraceTaskCodes::BackupRestoreService>
{
public:
    RequestReplyAsyncOperationBase(
        FABRIC_BACKUP_PARTITION_INFO const & partitionInfo,
        IpcClient& ipcClient,
        wstring const & traceMethod,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , partitionInfo_(partitionInfo)
        , ipcClient_(ipcClient)
        , traceMethod_(traceMethod)
    {
    }

    ~RequestReplyAsyncOperationBase() { }

    static Common::ErrorCode End(
        Common::AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<RequestReplyAsyncOperationBase>(asyncOperation);
        return casted->Error;
    }

protected:
    
    virtual MessageUPtr GetRequestMessage() = 0;

    __declspec(property(get = get_NameUri)) NamingUri & NameUri;
    NamingUri& get_NameUri() { return namingUri_; }

    __declspec(property(get = get_PartitionInfo)) FABRIC_BACKUP_PARTITION_INFO const& PartitionInfo;
    FABRIC_BACKUP_PARTITION_INFO const& get_PartitionInfo() { return partitionInfo_; }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        // Construct message here and send over IPC
        if (!Common::NamingUri::TryParse(partitionInfo_.ServiceName, namingUri_))
        {
            this->FinishComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
        }

        requestMessage_ = GetRequestMessage();

        Trace.WriteInfo(TraceComponent, "{0} Sending message for partition {1} with activity id {2}",
            traceMethod_,
            Common::Guid(partitionInfo_.PartitionId),
            FabricActivityHeader::FromMessage(*requestMessage_).ActivityId);

        BAMessage::WrapForBA(*requestMessage_);

        requestMessage_->Headers.Replace(TimeoutHeader(this->get_RemainingTime()));

        auto operation = ipcClient_.BeginRequest(
            move(requestMessage_),
            this->get_RemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnIpcRequestComplete(operation, false);
            },
            thisSPtr);

        this->OnIpcRequestComplete(operation, true);
    }

private:

    void OnIpcRequestComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        MessageUPtr reply;
        ErrorCode errorCode = ipcClient_.EndRequest(operation, reply);

        if (errorCode.IsSuccess())
        {
            errorCode = BAMessage::ValidateIpcReply(*reply);
            if (!errorCode.IsSuccess())
            {
                Common::ActivityId activityId = FabricActivityHeader::FromMessage(*reply).ActivityId;
                Trace.WriteWarning(TraceComponent, "{0} Request with activity id {1} failed with error {2}", traceMethod_, activityId, errorCode);
            }
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Request failed with error {1}", traceMethod_, errorCode);
        }

        this->TryComplete(operation->Parent, errorCode);
    }

    Common::NamingUri namingUri_;
    Transport::MessageUPtr requestMessage_;
    FABRIC_BACKUP_PARTITION_INFO const & partitionInfo_;
    IpcClient& ipcClient_;
    wstring traceMethod_;
};

class BackupRestoreServiceAgent::UpdateBackupSchedulePolicyAsyncOperation : public RequestReplyAsyncOperationBase
{
public:
    UpdateBackupSchedulePolicyAsyncOperation(
        FABRIC_BACKUP_PARTITION_INFO const & partitionInfo,
        FABRIC_BACKUP_POLICY const * policy,
        IpcClient & ipcClient,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestReplyAsyncOperationBase(partitionInfo, ipcClient, L"UpdateBackupSchedulePolicy", timeout, callback, parent)
        , policy_(policy)
    {
    }

protected:

    MessageUPtr GetRequestMessage()
    {
        MessageUPtr message;

        if (policy_ != NULL)
        {
            BackupPolicy backupPolicy;
            backupPolicy.FromPublicApi(*policy_);
            message = BAMessage::CreateUpdatePolicyMessage(backupPolicy, Common::Guid(this->PartitionInfo.PartitionId), this->NameUri);
        }
        else
        {
            // Policy can be NULL in case of disable protections
            message = BAMessage::CreateUpdatePolicyMessage(Common::Guid(this->PartitionInfo.PartitionId), this->NameUri);
        }

        return message;
    }

private:
    FABRIC_BACKUP_POLICY const * policy_;
};

class BackupRestoreServiceAgent::PartitionBackupAsyncOperation : public RequestReplyAsyncOperationBase
{
public:
    PartitionBackupAsyncOperation(
        FABRIC_BACKUP_PARTITION_INFO const & partitionInfo,
        FABRIC_BACKUP_OPERATION_ID operationId,
        FABRIC_BACKUP_CONFIGURATION const* backupConfiguration,
        IpcClient & ipcClient,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestReplyAsyncOperationBase(partitionInfo, ipcClient, L"BackupPartition", timeout, callback, parent)
        , operationId_(operationId)
        , backupConfig_(backupConfiguration)
    {
    }

protected:

    MessageUPtr GetRequestMessage()
    {
        MessageUPtr message;
        message = BAMessage::CreateBackupNowMessage(operationId_, *backupConfig_, Common::Guid(this->PartitionInfo.PartitionId), this->NameUri);
        return message;
    }

private:
    FABRIC_BACKUP_OPERATION_ID operationId_;
    FABRIC_BACKUP_CONFIGURATION const * backupConfig_;
};

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class BackupRestoreServiceAgent::DispatchMessageAsyncOperationBase
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::BackupRestoreService>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IBackupRestoreService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , NodeActivityTraceComponent(nodeInstance, FabricActivityHeader::FromMessage(*message).ActivityId)
        , service_(service)
        , request_(move(message))
        , receiverContext_(move(receiverContext))
    {
    }

    virtual ~DispatchMessageAsyncOperationBase() { }

    __declspec(property(get = get_Service)) IBackupRestoreService & Service;
    IBackupRestoreService & get_Service() { return service_; }

    __declspec(property(get = get_Request)) Message & Request;
    Message & get_Request() { return *request_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out MessageUPtr & reply, __out IpcReceiverContextUPtr & receiverContext)
    {
        auto casted = AsyncOperation::End<DispatchMessageAsyncOperationBase>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        // Always return receiver context for caller to send IPC failure
        receiverContext = move(casted->receiverContext_);

        return casted->Error;
    }

protected:

    virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply) = 0;

    void OnDispatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->EndDispatch(operation, reply_);

        this->TryComplete(operation->Parent, error);
    }

private:

    IBackupRestoreService & service_;
    MessageUPtr request_;
    IpcReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;
};

class BackupRestoreServiceAgent::GetBackupSchedulingPolicyAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetBackupSchedulingPolicyAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IBackupRestoreService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetBackupSchedulingPolicyAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        ScopedHeap heap;
        FABRIC_BACKUP_PARTITION_INFO partitionInfo = {};

        PartitionInfoMessageBody body;
        if (this->Request.GetBody<PartitionInfoMessageBody>(body))
        {
            partitionInfo.PartitionId = body.PartitionId.AsGUID();
            partitionInfo.ServiceName = heap.AddString(body.ServiceName);

            auto operation = this->Service.BeginGetBackupSchedulingPolicy(
                &partitionInfo,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Failed to get PartitionInfo message body", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndGetBackupSchedulingPolicy(operation, &policyResult_);

        if (error.IsSuccess())
        {
            auto policy = policyResult_->get_BackupSchedulePolicy();
            BackupPolicy backupPolicy;
            error = backupPolicy.FromPublicApi(*policy);
            if (!error.IsSuccess()) { return error; }
            
            GetPolicyReplyMessageBody getPolicyReplyBody(backupPolicy);
            reply = BAMessage::GetUserServiceOperationSuccess<GetPolicyReplyMessageBody>(getPolicyReplyBody);
        }

        return error;
    }

    IFabricGetBackupSchedulePolicyResult* policyResult_;
};

class BackupRestoreServiceAgent::GetRestorePointAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetRestorePointAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IBackupRestoreService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetRestorePointAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        ScopedHeap heap;
        FABRIC_BACKUP_PARTITION_INFO partitionInfo = { 0 };

        PartitionInfoMessageBody body;
        if (this->Request.GetBody<PartitionInfoMessageBody>(body))
        {
            partitionInfo.PartitionId = body.PartitionId.AsGUID();
            partitionInfo.ServiceName = heap.AddString(body.ServiceName);

            auto operation = this->Service.BeginGetRestorePointDetails(
                &partitionInfo,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Failed to get Restore point request message body.", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndGetGetRestorePointDetails(operation, &restorePoint_);

        if (error.IsSuccess())
        {
            auto restorePoint = restorePoint_->get_RestorePointDetails();
            RestorePointDetails restorePointDetails;
            restorePointDetails.FromPublicApi(*restorePoint);

            GetRestorePointReplyMessageBody getRestorePointReplyBody(restorePointDetails);
            reply = BAMessage::GetUserServiceOperationSuccess<GetRestorePointReplyMessageBody>(getRestorePointReplyBody);
        }

        return error;
    }

    IFabricGetRestorePointDetailsResult* restorePoint_;
};

class BackupRestoreServiceAgent::ReportBackupCompletionOperation : public DispatchMessageAsyncOperationBase
{
public:
    ReportBackupCompletionOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IBackupRestoreService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~ReportBackupCompletionOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        ScopedHeap heap;
        FABRIC_BACKUP_OPERATION_RESULT operationResult = { 0 };

        BackupOperationResultMessageBody body;
        if (this->Request.GetBody<BackupOperationResultMessageBody>(body))
        {
            operationResult.PartitionId = body.PartitionId.AsGUID();
            operationResult.ServiceName = heap.AddString(body.ServiceName);
            operationResult.OperationId = body.OperationId.AsGUID();
            operationResult.ErrorCode = body.ErrorCode;
            operationResult.Message = heap.AddString(body.Message);
            operationResult.TimeStampUtc = body.OperationTimestampUtc.AsFileTime;

            if (body.ErrorCode == ERROR_SUCCESS)
            {
                operationResult.EpochOfLastBackupRecord = body.Epoch;
                operationResult.LsnOfLastBackupRecord = body.Lsn;
                operationResult.BackupId = body.BackupId.AsGUID();
                operationResult.BackupLocation = heap.AddString(body.BackupLocation);
            }

            operationResult.Reserved = NULL;

            auto operation = this->Service.BeginReportBackupOperationResult(
                &operationResult,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Failed to get Backup Operation Result message body", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndReportBackupOperationResult(operation);

        if (error.IsSuccess())
        {
            reply = BAMessage::CreateSuccessReply(this->ActivityId);
        }

        return error;
    }
};

class BackupRestoreServiceAgent::ReportRestoreCompletionOperation : public DispatchMessageAsyncOperationBase
{
public:
    ReportRestoreCompletionOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IBackupRestoreService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~ReportRestoreCompletionOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        ScopedHeap heap;
        FABRIC_RESTORE_OPERATION_RESULT operationResult = { 0 };

        RestoreOperationResultMessageBody body;
        if (this->Request.GetBody<RestoreOperationResultMessageBody>(body))
        {
            operationResult.PartitionId = body.PartitionId.AsGUID();
            operationResult.OperationId = body.OperationId.AsGUID();
            operationResult.ServiceName = heap.AddString(body.ServiceName);
            operationResult.ErrorCode = body.ErrorCode;
            operationResult.Message = heap.AddString(body.Message);
            operationResult.TimeStampUtc = body.OperationTimestampUtc.AsFileTime;
            operationResult.Reserved = NULL;

            auto operation = this->Service.BeginReportRestoreOperationResult(
                &operationResult,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Failed to get Restore Operation Result message body", this->ActivityId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndReportRestoreOperationResult(operation);

        if (error.IsSuccess())
        {
            reply = BAMessage::CreateSuccessReply(this->ActivityId);
        }

        return error;
    }
};

//------------------------------------------------------------------------------
// BackupRestoreServiceAgent
//

BackupRestoreServiceAgent::BackupRestoreServiceAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : IBackupRestoreServiceAgent()
    , ComponentRoot()
    , NodeTraceComponent(nodeInstance)
    , runtimeCPtr_(runtimeCPtr)
    , servicePtr_()
    , ipcClient_(ipcClient)
{
    // Nothing
}

BackupRestoreServiceAgent::~BackupRestoreServiceAgent()
{
    // Nothing
}

ErrorCode BackupRestoreServiceAgent::Create(__out shared_ptr<BackupRestoreServiceAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    HRESULT hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    ComFabricRuntime * castedRuntimePtr;
    try
    {
        castedRuntimePtr = dynamic_cast<ComFabricRuntime*>(runtimeCPtr.GetRawPointer());
    }
    catch (...)
    {
        Assert::TestAssert("BRAgent unable to get ComfabricRuntime");
        return ErrorCodeValue::OperationFailed;
    }

    FabricNodeContextSPtr fabricNodeContext;
    auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    NodeId nodeId;
    if (!NodeId::TryParse(fabricNodeContext->NodeId, nodeId))
    {
        Assert::TestAssert("Could not parse NodeId {0}", fabricNodeContext->NodeId);
        return ErrorCodeValue::OperationFailed;
    }

    uint64 nodeInstanceId = fabricNodeContext->NodeInstanceId;
    Federation::NodeInstance nodeInstance(nodeId, nodeInstanceId);

    IpcClient & ipcClient = castedRuntimePtr->Runtime->Host.Client;
    shared_ptr<BackupRestoreServiceAgent> agentSPtr(new BackupRestoreServiceAgent(nodeInstance, ipcClient, runtimeCPtr));

    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "Successfully created BackupRestoreServiceAgent");
        result.swap(agentSPtr);
    }

    return error;
}

ErrorCode BackupRestoreServiceAgent::Initialize()
{
    // return success for now
    return ErrorCode::Success();
}

void BackupRestoreServiceAgent::Release()
{
    ipcClient_.UnregisterMessageHandler(Actor::BRS);
}

void BackupRestoreServiceAgent::RegisterBackupRestoreService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    IBackupRestoreServicePtr const & rootedService,
    __out wstring & serviceAddress)
{
    SystemServiceLocation serviceLocation(
        this->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);
    
    auto selfRoot = this->CreateComponentRoot();

    servicePtr_ = rootedService;

    ipcClient_.RegisterMessageHandler(
        Actor::BRS,
        [this, rootedService](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->ProcessTransportIpcRequest(rootedService, message, move(receiverContext));
        },
        false /*dispatchOnTransportThread*/);

    serviceAddress = wformatString("{0}", serviceLocation);
}

void BackupRestoreServiceAgent::UnregisterBackupRestoreService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId)
{
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(replicaId);

    ipcClient_.UnregisterMessageHandler(Actor::BRS);
}

AsyncOperationSPtr BackupRestoreServiceAgent::BeginUpdateBackupSchedulePolicy(
    FABRIC_BACKUP_PARTITION_INFO * info,
    FABRIC_BACKUP_POLICY * policy,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<UpdateBackupSchedulePolicyAsyncOperation>(
        *info,
        policy,
        ipcClient_,
        timeout,
        callback,
        parent);
}

ErrorCode BackupRestoreServiceAgent::EndUpdateBackupSchedulePolicy(AsyncOperationSPtr const &operation)
{
    auto op = AsyncOperation::End<UpdateBackupSchedulePolicyAsyncOperation>(operation);
    return op->Error;
}

AsyncOperationSPtr BackupRestoreServiceAgent::BeginPartitionBackupOperation(
    FABRIC_BACKUP_PARTITION_INFO * info,
    FABRIC_BACKUP_OPERATION_ID operationId,
    FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<PartitionBackupAsyncOperation>(
        *info,
        operationId,
        backupConfiguration,
        ipcClient_,
        timeout,
        callback,
        parent);
}

ErrorCode BackupRestoreServiceAgent::EndPartitionBackupOperation(
    AsyncOperationSPtr const &operation)
{
    auto op = AsyncOperation::End<PartitionBackupAsyncOperation>(operation);
    return op->Error;
}

void BackupRestoreServiceAgent::ProcessTransportIpcRequest(
    IBackupRestoreServicePtr const & servicePtr, 
    MessageUPtr & message, 
    IpcReceiverContextUPtr && receiverContext)
{
    auto activityId = FabricActivityHeader::FromMessage(*message).ActivityId;

    Trace.WriteInfo(TraceComponent, "ProcessTransportIpcRequest: Request received with action {0} and activityid {1}, message id {2}", 
        message->Action, 
        activityId,
        message->MessageId);
    
    if (message->Actor == Actor::BRS)
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            Trace.WriteError(TraceComponent, "{0} missing TimeoutHeader", activityId);
            this->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext, activityId);
            return;
        }

        vector<ComponentRootSPtr> roots;
        roots.push_back(servicePtr.get_Root());
        roots.push_back(this->CreateComponentRoot());

        if (message->Action == BAMessage::GetBackupPolicyAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetBackupSchedulingPolicyAsyncOperation>(
                this->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this, activityId](AsyncOperationSPtr const & operation) { return this->OnProcessTransportIpcRequestComplete(operation, activityId, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnProcessTransportIpcRequestComplete(operation, activityId, true);
        }
        else if (message->Action == BAMessage::GetRestorePointAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetRestorePointAsyncOperation>(
                this->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this, activityId](AsyncOperationSPtr const & operation) { return this->OnProcessTransportIpcRequestComplete(operation, activityId, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnProcessTransportIpcRequestComplete(operation, activityId, true);
        }
        else if (message->Action == BAMessage::BackupCompleteAction)
        {
            auto operation = AsyncOperation::CreateAndStart<ReportBackupCompletionOperation>(
                this->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this, activityId](AsyncOperationSPtr const & operation) { return this->OnProcessTransportIpcRequestComplete(operation, activityId, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnProcessTransportIpcRequestComplete(operation, activityId, true);
        }
        else if (message->Action == BAMessage::RestoreCompleteAction)
        {
            auto operation = AsyncOperation::CreateAndStart<ReportRestoreCompletionOperation>(
                this->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this, activityId](AsyncOperationSPtr const & operation) { return this->OnProcessTransportIpcRequestComplete(operation, activityId, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnProcessTransportIpcRequestComplete(operation, activityId, true);
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "{0} Invalid Action {1}", activityId, message->Action);
            this->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext, activityId);
        }
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "{0} Invalid Actor {1}", activityId, message->Actor);
        this->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext, activityId);
    }
}

void BackupRestoreServiceAgent::SendIpcReply(MessageUPtr&& reply, IpcReceiverContext const & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void BackupRestoreServiceAgent::OnIpcFailure(ErrorCode const& error, IpcReceiverContext const & receiverContext, ActivityId const& activityId)
{
    auto reply = BAMessage::CreateIpcFailureMessage(IpcFailureBody(error), activityId);
    this->SendIpcReply(move(reply), receiverContext);
}

void BackupRestoreServiceAgent::OnProcessTransportIpcRequestComplete(
    AsyncOperationSPtr const & operation,
    ActivityId const& activityId,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode error = DispatchMessageAsyncOperationBase::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "{0} Replying back to IPC request with action {1}", activityId, reply->Action);
        this->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "{0} Replying back to IPC request with error {1}", activityId, error);
        this->OnIpcFailure(error, *receiverContext, activityId);
    }
}
