// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Query;
using namespace Reliability;
using namespace std;
using namespace ServiceModel;
using namespace Management::NetworkInventoryManager;

// *** Helper Functions
//

template<typename T>
ErrorCode TryGetMessageBody(Transport::Message & message, T & body)
{
    if (!message.GetBody(body))
    {
        ReliabilitySubsystem::WriteWarning(Reliability::Constants::AdminApiSource, "Message with id {0} has invalid body {1:x}", message.MessageId, message.Status);
        return ErrorCode(ErrorCodeValue::InvalidMessage);
    }

    return ErrorCode::Success();
}

ErrorCode TryExtractErrorCodeFromBasicFailoverReplyMessage(Message & message)
{
    ErrorCode error;

    BasicFailoverReplyMessageBody body;
    error = TryGetMessageBody(message, body);
    if (!error.IsSuccess())
    {
        return error;
    }

    return body.ErrorCodeValue;
}

// *** Helper Classes
//

// *** Base async operation for DeactivateNodes, RemoveNodeDeactivations, and GetNodeDeactivationStatus

class ServiceAdminClient::NodeDeactivationBaseAsyncOperation : public AsyncOperation
{
    DENY_COPY(NodeDeactivationBaseAsyncOperation)

public:

    NodeDeactivationBaseAsyncOperation(
        __in ServiceAdminClient & serviceAdminClient,
        FabricActivityHeader const& activityHeader,
        TimeSpan const timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent),
        serviceAdminClient_(serviceAdminClient),
        activityHeader_(activityHeader),
        timeoutHelper_(timeout),
        pendingRequests_(0),
        error_(),
        lock_()
    {
    }

    virtual ~NodeDeactivationBaseAsyncOperation() { }

    __declspec (property(get = get_ActivityId)) Common::ActivityId const& ActivityId;
    Common::ActivityId const& get_ActivityId() const { return activityHeader_.ActivityId; }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        return AsyncOperation::End<NodeDeactivationBaseAsyncOperation>(operation)->Error;
    }

protected:

    __declspec (property(get=get_Transport)) FederationWrapper & Transport;
    FederationWrapper & get_Transport() const { return serviceAdminClient_.Transport; }

    void InitializeRequest(MessageUPtr && request)
    {
        request_ = move(request);
        RSMessage::AddActivityHeader(*request_, activityHeader_);
    }

    virtual void OnStart(Common::AsyncOperationSPtr const& thisSPtr)
    {
        pendingRequests_.store(2);

        this->SendRequestToFMM(thisSPtr);

        this->SendRequestToFM(thisSPtr);
    }

    virtual ErrorCode OnProcessReply(__in MessageUPtr & reply)
    {
        return TryExtractErrorCodeFromBasicFailoverReplyMessage(*reply);
    }

private:

    void SendRequestToFMM(AsyncOperationSPtr const& thisSPtr)
    {
        auto operation = this->Transport.BeginRequestToFMM(
            request_->Clone(),
            FailoverConfig::GetConfig().RoutingRetryTimeout,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation) { this->OnRequestToFMMCompleted(operation, false); },
            thisSPtr);
        this->OnRequestToFMMCompleted(operation, true);
    }

    void SendRequestToFM(AsyncOperationSPtr const& thisSPtr)
    {
        auto operation = this->Transport.BeginRequestToFM(
            request_->Clone(),
            FailoverConfig::GetConfig().RoutingRetryTimeout,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation) { this->OnRequestToFMCompleted(operation, false); },
            thisSPtr);
        this->OnRequestToFMCompleted(operation, true);
    }

    void OnRequestToFMMCompleted(
        AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        MessageUPtr reply;
        auto error = this->Transport.EndRequestToFMM(operation, reply);

        this->ProcessReply(operation->Parent, error, reply);
    }

    void OnRequestToFMCompleted(
        AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        MessageUPtr reply;
        auto error = this->Transport.EndRequestToFM(operation, reply);

        this->ProcessReply(operation->Parent, error, reply);
    }

    void ProcessReply(AsyncOperationSPtr const& thisSPtr, __in ErrorCode & error, __in MessageUPtr & reply)
    {
        if (error.IsSuccess())
        {
            error = this->OnProcessReply(reply);
        }

        if (!error.IsSuccess())
        {
            AcquireWriteLock lock(lock_);

            error_ = ErrorCode::FirstError(error_, error);
        }

        this->FinishRequest(thisSPtr);
    }

    void FinishRequest(AsyncOperationSPtr const& thisSPtr)
    {
        if (--pendingRequests_ == 0)
        {
            ErrorCode error(ErrorCodeValue::Success);
            {
                AcquireReadLock lock(lock_);

                error = move(error_);
            }

            this->TryComplete(thisSPtr, error);
        }
    }

    ServiceAdminClient & serviceAdminClient_;
    MessageUPtr request_;
    Transport::FabricActivityHeader const activityHeader_;
    TimeoutHelper timeoutHelper_;

    Common::atomic_long pendingRequests_;
    ErrorCode error_;
    RwLock lock_;
};

// *** DeactivateNodes

class ServiceAdminClient::DeactivateNodesAsyncOperation : public NodeDeactivationBaseAsyncOperation
{
    DENY_COPY(DeactivateNodesAsyncOperation)

public:

    DeactivateNodesAsyncOperation(
        __in ServiceAdminClient & serviceAdminClient,
        map<Federation::NodeId, NodeDeactivationIntent::Enum> && deactivations,
        wstring const & batchId,
        FabricActivityHeader const& activityHeader,
        TimeSpan const timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : NodeDeactivationBaseAsyncOperation(
            serviceAdminClient,
            activityHeader,
            timeout,
            callback,
            parent)
    {
        auto request = RSMessage::GetDeactivateNodesRequest().CreateMessage(
            DeactivateNodesRequestMessageBody(
                move(deactivations),
                batchId,
                DateTime::Now().Ticks));

        this->InitializeRequest(move(request));
    }
};

// *** RemoveNodeDeactivations

class ServiceAdminClient::RemoveNodeDeactivationsAsyncOperation : public NodeDeactivationBaseAsyncOperation
{
    DENY_COPY(RemoveNodeDeactivationsAsyncOperation)

public:

    RemoveNodeDeactivationsAsyncOperation(
        __in ServiceAdminClient & serviceAdminClient,
        wstring const & batchId,
        FabricActivityHeader const& activityHeader,
        TimeSpan const timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : NodeDeactivationBaseAsyncOperation(
            serviceAdminClient,
            activityHeader,
            timeout,
            callback,
            parent)
    {
        auto request = RSMessage::GetRemoveNodeDeactivationRequest().CreateMessage(
            RemoveNodeDeactivationRequestMessageBody(
                batchId,
                DateTime::Now().Ticks));

        this->InitializeRequest(move(request));
    }
};

// *** GetNodeDeactivationStatus

class ServiceAdminClient::GetNodeDeactivationStatusAsyncOperation : public NodeDeactivationBaseAsyncOperation
{
    DENY_COPY(GetNodeDeactivationStatusAsyncOperation)

public:

    GetNodeDeactivationStatusAsyncOperation(
        __in ServiceAdminClient & serviceAdminClient,
        wstring const & batchId,
        FabricActivityHeader const& activityHeader,
        TimeSpan const timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : NodeDeactivationBaseAsyncOperation(
            serviceAdminClient,
            activityHeader,
            timeout,
            callback,
            parent)
        , status_(NodeDeactivationStatus::None)
        , progressDetails_()
    {
        auto request = RSMessage::GetNodeDeactivationStatusRequest().CreateMessage(
            NodeDeactivationStatusRequestMessageBody(
                batchId));

        this->InitializeRequest(move(request));
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out NodeDeactivationStatus::Enum & status,
        __out vector<NodeProgress> & progressDetails)
    {
        auto casted = AsyncOperation::End<GetNodeDeactivationStatusAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            status = casted->status_;
            progressDetails = casted->progressDetails_;
        }

        return casted->Error;
    }

protected:

    virtual ErrorCode OnProcessReply(__in MessageUPtr & reply)
    {
        ErrorCode error(ErrorCodeValue::Success);

        NodeDeactivationStatusReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.Error;

            if (error.IsSuccess())
            {
                AcquireExclusiveLock lock(lock_);

                ReliabilitySubsystem::WriteNoise(
                    "GetNodeDeactivationStatus",
                    "[{0}] Merging status ({1}, {2}\r\n{3}{4})",
                    ActivityId,
                    status_,
                    body.DeactivationStatus,
                    progressDetails_,
                    body.ProgressDetails);

                status_ = NodeDeactivationStatus::Merge(status_, body.DeactivationStatus);
                progressDetails_.insert(
                    progressDetails_.end(),
                    body.ProgressDetails.begin(),
                    body.ProgressDetails.end());
            }
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }

        return error;
    }

private:

    NodeDeactivationStatus::Enum status_;
    std::vector<NodeProgress> progressDetails_;

    ExclusiveLock lock_;
};

// *** ServiceAdminClient
//

ServiceAdminClient::ServiceAdminClient(FederationWrapper & transport)
  : transport_(transport)
{
}

AsyncOperationSPtr ServiceAdminClient::BeginCreateService(
    ServiceDescription const & service,
    std::vector<ConsistencyUnitDescription> const& consistencyUnitDescriptions,
    FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginCreateService(
        activityHeader.ActivityId,
        service,
        consistencyUnitDescriptions);

    CreateServiceMessageBody body(service, consistencyUnitDescriptions);

    MessageUPtr createMessage = RSMessage::GetCreateService().CreateMessage<CreateServiceMessageBody>(body);

    RSMessage::AddActivityHeader(*createMessage, activityHeader);
    createMessage->Idempotent = false;

    AsyncOperationSPtr operation = transport_.BeginRequestToFM(std::move(createMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

ErrorCode ServiceAdminClient::EndCreateService(AsyncOperationSPtr const & operation, __out CreateServiceReplyMessageBody & body)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        error = TryGetMessageBody(*reply, body);
    }

    ReliabilityEventSource::Events->EndCreateService(activityId, error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginUpdateService(
    wstring const& serviceName,
    Naming::ServiceUpdateDescription const& serviceUpdateDescription,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginUpdateSystemService(
        activityId,
        serviceName,
        serviceUpdateDescription);

    UpdateSystemServiceMessageBody body(serviceName, serviceUpdateDescription);

    MessageUPtr request = RSMessage::GetUpdateSystemService().CreateMessage<UpdateSystemServiceMessageBody>(body);

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    bool isTargetFMM = (serviceName == *ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName);

    AsyncOperationSPtr operation = isTargetFMM ?
        transport_.BeginRequestToFMM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent) :
        transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    operation->PushOperationContext(make_unique<OperationContext<bool>>(move(isTargetFMM)));

    return operation;
}

AsyncOperationSPtr ServiceAdminClient::BeginUpdateService(
    ServiceDescription const& serviceDescription,
    vector<ConsistencyUnitDescription> && addedCuids,
    vector<ConsistencyUnitDescription> && removedCuids,
    FabricActivityHeader const& activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginUpdateUserService(
        activityId,
        serviceDescription);

    UpdateServiceMessageBody body(serviceDescription, move(addedCuids), move(removedCuids));

    MessageUPtr request = RSMessage::GetUpdateService().CreateMessage<UpdateServiceMessageBody>(body);

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    AsyncOperationSPtr operation = transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    operation->PushOperationContext(make_unique<OperationContext<bool>>(move(false)));

    return operation;
}

ErrorCode ServiceAdminClient::EndUpdateService(AsyncOperationSPtr const & operation, __out UpdateServiceReplyMessageBody & body)
{
    bool isTargetFMM = operation->PopOperationContext<bool>()->Context;
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = isTargetFMM ?
        transport_.EndRequestToFMM(operation, reply) :
        transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        error = TryGetMessageBody(*reply, body);
    }

    ReliabilityEventSource::Events->EndUpdateService(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginDeleteService(
    std::wstring const & serviceName,
    FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginDeleteService(
        serviceName,
        false, //Unforcefully
        DateTime::Now().Ticks,
        activityHeader,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ServiceAdminClient::BeginDeleteService(
    std::wstring const & serviceName,
    bool const isForce,
    uint64 instanceId,
    FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginDeleteService(
        activityId,
        serviceName,
        isForce,
        instanceId);

    MessageUPtr deleteMessage = RSMessage::GetDeleteService().CreateMessage(DeleteServiceMessageBody(serviceName, isForce, instanceId));

    RSMessage::AddActivityHeader(*deleteMessage, activityHeader);
    deleteMessage->Idempotent = false;

    AsyncOperationSPtr operation = transport_.BeginRequestToFM(std::move(deleteMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

ErrorCode ServiceAdminClient::EndDeleteService(AsyncOperationSPtr const & operation)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }

    ReliabilityEventSource::Events->EndDeleteService(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginQuery(
    FabricActivityHeader const & activityHeader,
    QueryAddressHeader const & addressHeader,
    QueryRequestMessageBodyInternal const & requestMessageBody,
    bool isTargetFMM,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginQuery(
        activityId,
        wformatString(requestMessageBody.QueryName));

    MessageUPtr QueryMessage = RSMessage::GetQueryRequest().CreateMessage(requestMessageBody);

    RSMessage::AddActivityHeader(*QueryMessage, activityHeader);
    QueryMessage->Headers.Add(addressHeader);
    QueryMessage->Idempotent = true;

    QueryMessage->Headers.Replace(TimeoutHeader(timeout));

    AsyncOperationSPtr asyncOperationSPtr =
        isTargetFMM ?
            transport_.BeginRequestToFMM(std::move(QueryMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent):
            transport_.BeginRequestToFM(std::move(QueryMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    asyncOperationSPtr->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    asyncOperationSPtr->PushOperationContext(make_unique<OperationContext<bool>>(move(isTargetFMM)));

    return asyncOperationSPtr;
}

ErrorCode ServiceAdminClient::EndQuery(AsyncOperationSPtr const & operation, MessageUPtr & reply)
{
    bool isTargetFMM = operation->PopOperationContext<bool>()->Context;
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    ErrorCode error = isTargetFMM ? transport_.EndRequestToFMM(operation, reply) : transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        ServiceModel::QueryResult queryResult;
        error = TryGetMessageBody(*reply, queryResult);
        if (error.IsSuccess())
        {
            error = queryResult.QueryProcessingError;
        }
    }

    ReliabilityEventSource::Events->EndQuery(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginDeactivateNodes(
    map<Federation::NodeId, NodeDeactivationIntent::Enum> && deactivations,
    wstring const & batchId,
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilityEventSource::Events->BeginDeactivateNodes(
        activityHeader.ActivityId,
        batchId,
        wformatString(deactivations));

    return AsyncOperation::CreateAndStart<DeactivateNodesAsyncOperation>(
        *this,
        move(deactivations),
        batchId,
        activityHeader,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceAdminClient::EndDeactivateNodes(AsyncOperationSPtr const & operation)
{
    ErrorCode error = DeactivateNodesAsyncOperation::End(operation);

    ActivityId activityId = AsyncOperation::Get<DeactivateNodesAsyncOperation>(operation)->ActivityId;

    ReliabilityEventSource::Events->EndDeactivateNodes(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginRemoveNodeDeactivations(
    wstring const & batchId,
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilityEventSource::Events->BeginRemoveNodeDeactivations(
        activityHeader.ActivityId,
        batchId);

    return AsyncOperation::CreateAndStart<RemoveNodeDeactivationsAsyncOperation>(
        *this,
        batchId,
        activityHeader,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceAdminClient::EndRemoveNodeDeactivations(AsyncOperationSPtr const & operation)
{
    ErrorCode error = RemoveNodeDeactivationsAsyncOperation::End(operation);

    ActivityId activityId = AsyncOperation::Get<RemoveNodeDeactivationsAsyncOperation>(operation)->ActivityId;

    ReliabilityEventSource::Events->EndRemoveNodeDeactivations(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginGetNodeDeactivationStatus(
    wstring const & batchId,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilityEventSource::Events->BeginGetNodeDeactivationStatus(
        activityHeader.ActivityId,
        batchId);

    return AsyncOperation::CreateAndStart<GetNodeDeactivationStatusAsyncOperation>(
        *this,
        batchId,
        activityHeader,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceAdminClient::EndGetNodeDeactivationsStatus(
    AsyncOperationSPtr const & operation,
    __out NodeDeactivationStatus::Enum & status,
    __out vector<NodeProgress> & progressDetails)
{
    ErrorCode error = GetNodeDeactivationStatusAsyncOperation::End(operation, status, progressDetails);

    ActivityId activityId = AsyncOperation::Get<GetNodeDeactivationStatusAsyncOperation>(operation)->ActivityId;

    ReliabilityEventSource::Events->EndGetNodeDeactivationsStatus(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginNodeStateRemoved(
    Federation::NodeId nodeId,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginNodeStateRemoved(
        activityId,
        nodeId);

    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<NodeStateRemovedAsycOperation>(*this, nodeId, activityHeader, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndNodeStateRemoved(AsyncOperationSPtr const & operation)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    ErrorCode error = NodeStateRemovedAsycOperation::End(operation);

    ReliabilityEventSource::Events->EndNodeStateRemoved(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginRecoverPartitions(
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginRecoverPartitions(
        activityId);

    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<RecoverAllPartitionsAsyncOperation>(*this, activityHeader, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndRecoverPartitions(AsyncOperationSPtr const & operation)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    ErrorCode error = RecoverAllPartitionsAsyncOperation::End(operation);

    ReliabilityEventSource::Events->EndRecoverPartitions(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginRecoverPartition(
    PartitionId const& partitionId,
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginRecoverPartition(
        activityId,
        partitionId);

    MessageUPtr request = RSMessage::GetRecoverPartition().CreateMessage(RecoverPartitionRequestMessageBody(partitionId));

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    bool isTargetFMM = (partitionId.Guid == Constants::FMServiceGuid);

    AsyncOperationSPtr operation = isTargetFMM ?
        transport_.BeginRequestToFMM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent) :
        transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    operation->PushOperationContext(make_unique<OperationContext<bool>>(move(isTargetFMM)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndRecoverPartition(AsyncOperationSPtr const & operation)
{
    bool isTargetFMM = operation->PopOperationContext<bool>()->Context;
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = isTargetFMM ?
        transport_.EndRequestToFMM(operation, reply) :
        transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }

    ReliabilityEventSource::Events->EndRecoverPartition(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginRecoverServicePartitions(
    wstring const& serviceName,
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginRecoverServicePartitions(
        activityId,
        serviceName);

    MessageUPtr request = RSMessage::GetRecoverServicePartitions().CreateMessage(RecoverServicePartitionsRequestMessageBody(serviceName));

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    bool isTargetFMM = (serviceName == ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName);

    AsyncOperationSPtr operation = isTargetFMM ?
        transport_.BeginRequestToFMM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent) :
        transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    operation->PushOperationContext(make_unique<OperationContext<bool>>(move(isTargetFMM)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndRecoverServicePartitions(AsyncOperationSPtr const & operation)
{
    bool isTargetFMM = operation->PopOperationContext<bool>()->Context;
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = isTargetFMM ?
        transport_.EndRequestToFMM(operation, reply) :
        transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }

    ReliabilityEventSource::Events->EndRecoverServicePartitions(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginRecoverSystemPartitions(
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginRecoverSystemPartitions(
        activityId);

    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<RecoverSystemPartitionsAsyncOperation>(*this, activityHeader, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndRecoverSystemPartitions(AsyncOperationSPtr const & operation)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    ErrorCode error = RecoverSystemPartitionsAsyncOperation::End(operation);

    ReliabilityEventSource::Events->EndRecoverSystemPartitions(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginGetServiceDescription(
    wstring const& serviceName,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return this->BeginGetServiceDescriptionInternal(
        serviceName,
        false,
        activityHeader,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ServiceAdminClient::BeginGetServiceDescriptionWithCuids(
    wstring const& serviceName,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return this->BeginGetServiceDescriptionInternal(
        serviceName,
        true,
        activityHeader,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ServiceAdminClient::BeginGetServiceDescriptionInternal(
    wstring const& serviceName,
    bool includeCuids,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    ActivityId activityId = activityHeader.ActivityId;

    ReliabilityEventSource::Events->BeginGetServiceDescription(
        activityId,
        serviceName,
        includeCuids);

    ServiceDescriptionRequestMessageBody body(serviceName, includeCuids);

    MessageUPtr request = RSMessage::GetServiceDescriptionRequest().CreateMessage<ServiceDescriptionRequestMessageBody>(body);

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    bool isTargetFMM = (serviceName == *ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName);

    AsyncOperationSPtr operation = isTargetFMM ?
        transport_.BeginRequestToFMM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent) :
        transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));
    operation->PushOperationContext(make_unique<OperationContext<bool>>(move(isTargetFMM)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndGetServiceDescription(AsyncOperationSPtr const& operation, __out ServiceDescriptionReplyMessageBody & body)
{
    bool isTargetFMM = operation->PopOperationContext<bool>()->Context;
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = isTargetFMM ?
        transport_.EndRequestToFMM(operation, reply) :
        transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        error = TryGetMessageBody(*reply, body);
    }

    ReliabilityEventSource::Events->EndGetServiceDescription(
        activityId,
        error);

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginResetPartitionLoad(
    PartitionId const& partitionId,
    FabricActivityHeader const & activityHeader,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilitySubsystem::WriteInfo(
        Constants::AdminApiSource, L"ResetPartitionLoad",
        "[{0}] {1}: Starting ResetPartitionLoad for {2}",
        TraceId, activityHeader, partitionId);

    MessageUPtr request = RSMessage::GetResetPartitionLoad().CreateMessage(ResetPartitionLoadRequestMessageBody(partitionId));

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    AsyncOperationSPtr operation;
    if (partitionId.Guid == Constants::FMServiceGuid)
    {
        operation = transport_.BeginRequestToFMM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
    }
    else
    {
        operation = transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
    }

    operation->PushOperationContext(make_unique<OperationContext<PartitionId>>(PartitionId(partitionId)));
    return operation;
}

Common::ErrorCode ServiceAdminClient::EndResetPartitionLoad(AsyncOperationSPtr const & operation)
{
    PartitionId partitionId = operation->PopOperationContext<PartitionId>()->Context;

    MessageUPtr reply;
    ErrorCode error;
    if (partitionId.Guid == Constants::FMServiceGuid)
    {
        error = transport_.EndRequestToFMM(operation, reply);
    }
    else
    {
        error = transport_.EndRequestToFM(operation, reply);
    }

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }
    else
    {
        ReliabilitySubsystem::WriteWarning(
            Constants::AdminApiSource, L"ResetPartitionLoad",
            "ResetPartitionLoad completed with error {0}", error);
    }

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginToggleVerboseServicePlacementHealthReporting(
    bool enabled,
    Transport::FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    ReliabilitySubsystem::WriteInfo(
        Constants::AdminApiSource, L"ToggleVerboseServicePlacementHealthReporting",
        "[{0}] {1}: Starting ToggleVerboseServicePlacementHealthReporting for {2}",
        TraceId, activityHeader, enabled);

    MessageUPtr request = RSMessage::GetToggleVerboseServicePlacementHealthReporting().CreateMessage(ToggleVerboseServicePlacementHealthReportingRequestMessageBody(enabled));

    RSMessage::AddActivityHeader(*request, activityHeader);
    request->Idempotent = false;

    AsyncOperationSPtr operation;

    //We never route to the FMM, the FMM should always have verbose service placement enabled because if an FM replia isn't getting placed that should always raise a health alert
    operation = transport_.BeginRequestToFM(std::move(request), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<std::wstring>>(std::wstring(enabled ? L"true" : L"false")));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndToggleVerboseServicePlacementHealthReporting(Common::AsyncOperationSPtr const & operation)
{
    operation->PopOperationContext<std::wstring>()->Context;

    MessageUPtr reply;
    ErrorCode error;

    error = transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }
    else
    {
        ReliabilitySubsystem::WriteWarning(
            Constants::AdminApiSource, L"ToggleVerboseServicePlacementHealthReporting",
            "ToggleVerboseServicePlacementHealthReporting completed with error {0}", error);
    }

    return error;

}

AsyncOperationSPtr ServiceAdminClient::BeginCreateNetwork(
    ServiceModel::ModelV2::NetworkResourceDescription const &networkDescription,
    FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilitySubsystem::WriteInfo(
        Constants::NIMApiSource, L"CreateNetwork",
        "[{0}] {1}: Starting CreateNetwork for network {2} with type {3}",
        TraceId, activityHeader, networkDescription.Name, (int)networkDescription.NetworkType);

    if (!Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
    {
        ReliabilitySubsystem::WriteError(
            Constants::NIMApiSource, L"CreateNetwork",
            "[{0}] {1}: CreateNetwork: NetworkInventoryManager not configured (network {2} with type {3})",
            TraceId, activityHeader, networkDescription.Name, (int)networkDescription.NetworkType);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::SystemServiceNotFound, callback, parent);
    }

    ActivityId activityId = activityHeader.ActivityId;

    CreateNetworkMessageBody body(networkDescription);

    MessageUPtr createNetworkMessage = NIMMessage::GetCreateNetwork().CreateMessage<CreateNetworkMessageBody>(body);

    NIMMessage::AddActivityHeader(*createNetworkMessage, activityHeader);
    createNetworkMessage->Idempotent = false;

    AsyncOperationSPtr operation = transport_.BeginRequestToFM(std::move(createNetworkMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndCreateNetwork(AsyncOperationSPtr const & operation)
{    
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }
    else
    {
        ReliabilitySubsystem::WriteWarning(
            Constants::NIMApiSource, L"CreateNetwork",
            "CreateNetwork completed with error {0}", error);
    }

    return error;
}

AsyncOperationSPtr ServiceAdminClient::BeginDeleteNetwork(
    ServiceModel::DeleteNetworkDescription const &deleteNetworkDescription,
    FabricActivityHeader const & activityHeader,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ReliabilitySubsystem::WriteInfo(
        Constants::NIMApiSource, L"DeleteNetwork",
        "[{0}] {1}: Starting DeleteNetwork for network {2}",
        TraceId, activityHeader, deleteNetworkDescription.NetworkName);

    ActivityId activityId = activityHeader.ActivityId;

    DeleteNetworkMessageBody body(deleteNetworkDescription);

    MessageUPtr deleteNetworkMessage = NIMMessage::GetDeleteNetwork().CreateMessage<DeleteNetworkMessageBody>(body);

    NIMMessage::AddActivityHeader(*deleteNetworkMessage, activityHeader);
    deleteNetworkMessage->Idempotent = false;

    AsyncOperationSPtr operation = transport_.BeginRequestToFM(std::move(deleteNetworkMessage), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);

    operation->PushOperationContext(make_unique<OperationContext<ActivityId>>(move(activityId)));

    return operation;
}

Common::ErrorCode ServiceAdminClient::EndDeleteNetwork(AsyncOperationSPtr const & operation)
{
    ActivityId activityId = operation->PopOperationContext<ActivityId>()->Context;

    MessageUPtr reply;
    ErrorCode error = transport_.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        error = TryGetMessageBody(*reply, body);
        if (error.IsSuccess())
        {
            error = body.ErrorCodeValue;
        }
    }
    else
    {
        ReliabilitySubsystem::WriteWarning(
            Constants::NIMApiSource, L"DeleteNetwork",
            "DeleteNetwork completed with error {0}", error);
    }

    return error;
}

ServiceAdminClient::NodeStateRemovedAsycOperation::NodeStateRemovedAsycOperation(
    ServiceAdminClient & serviceAdminClient,
    Federation::NodeId nodeId,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
      serviceAdminClient_(serviceAdminClient),
      nodeId_(nodeId),
      activityHeader_(activityHeader),
      timeout_(timeout),
      isRequestToFMMComplete_(false),
      isRequestToFMComplete_(false),
      lockObject_()
{
}

ErrorCode ServiceAdminClient::NodeStateRemovedAsycOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<NodeStateRemovedAsycOperation>(operation);
    return thisPtr->Error;
}

void ServiceAdminClient::NodeStateRemovedAsycOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    MessageUPtr requestToFMM = RSMessage::GetNodeStateRemoved().CreateMessage<NodeStateRemovedRequestMessageBody>(NodeStateRemovedRequestMessageBody(nodeId_));
    RSMessage::AddActivityHeader(*requestToFMM, activityHeader_);
    auto requestToFMMOperation = serviceAdminClient_.transport_.BeginRequestToFMM(std::move(requestToFMM), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout_, OnRequestToFMMCompleted, thisSPtr);

    MessageUPtr requestToFM = RSMessage::GetNodeStateRemoved().CreateMessage<NodeStateRemovedRequestMessageBody>(NodeStateRemovedRequestMessageBody(nodeId_));
    RSMessage::AddActivityHeader(*requestToFM, activityHeader_);
    auto requestToFMOperation = serviceAdminClient_.transport_.BeginRequestToFM(std::move(requestToFM), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout_, OnRequestToFMCompleted, thisSPtr);
}

void ServiceAdminClient::NodeStateRemovedAsycOperation::OnRequestToFMMCompleted(AsyncOperationSPtr const& requestOperation)
{
    auto thisPtr = AsyncOperation::Get<NodeStateRemovedAsycOperation>(requestOperation->Parent);

    MessageUPtr reply;
    ErrorCode error = thisPtr->serviceAdminClient_.transport_.EndRequestToFMM(requestOperation, reply);

    if (error.IsSuccess())
    {
        AcquireExclusiveLock lock(thisPtr->lockObject_);

        error = TryExtractErrorCodeFromBasicFailoverReplyMessage(*reply);

        thisPtr->isRequestToFMMComplete_ = true;
        thisPtr->errorFMM_ = error;

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NodeNotFound) && !error.IsError(ErrorCodeValue::NodeIsUp))
        {
            thisPtr->Complete(requestOperation->Parent, error);
        }
        else if (thisPtr->isRequestToFMComplete_)
        {
            error = OnRequestCompleted(thisPtr->errorFMM_, thisPtr->errorFM_);
            thisPtr->Complete(requestOperation->Parent, error);
        }
    }
    else
    {
        thisPtr->Complete(requestOperation->Parent, error);
    }
}

void ServiceAdminClient::NodeStateRemovedAsycOperation::OnRequestToFMCompleted(AsyncOperationSPtr const& requestOperation)
{
    auto thisPtr = AsyncOperation::Get<NodeStateRemovedAsycOperation>(requestOperation->Parent);

    MessageUPtr reply;
    ErrorCode error = thisPtr->serviceAdminClient_.transport_.EndRequestToFM(requestOperation, reply);

    if (error.IsSuccess())
    {
        AcquireExclusiveLock lock(thisPtr->lockObject_);

        error = TryExtractErrorCodeFromBasicFailoverReplyMessage(*reply);

        thisPtr->isRequestToFMComplete_ = true;
        thisPtr->errorFM_ = error;

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NodeNotFound) && !error.IsError(ErrorCodeValue::NodeIsUp))
        {
            thisPtr->Complete(requestOperation->Parent, error);
        }
        else if (thisPtr->isRequestToFMMComplete_)
        {
            error = OnRequestCompleted(thisPtr->errorFM_, thisPtr->errorFMM_);
            thisPtr->Complete(requestOperation->Parent, error);
        }
    }
    else
    {
        thisPtr->Complete(requestOperation->Parent, error);
    }
}

ErrorCode ServiceAdminClient::NodeStateRemovedAsycOperation::OnRequestCompleted(ErrorCode thisError, ErrorCode otherError)
{
    if (thisError.IsSuccess())
    {
        if (otherError.IsSuccess() ||
            otherError.IsError(ErrorCodeValue::NodeNotFound))
        {
            return ErrorCodeValue::Success;
        }
        else if (otherError.IsError(ErrorCodeValue::NodeIsUp))
        {
            return ErrorCodeValue::InvalidState;
        }
        else
        {
            return otherError;
        }
    }
    else if (thisError.IsError(ErrorCodeValue::NodeNotFound))
    {
        if (otherError.IsSuccess())
        {
            return ErrorCodeValue::Success;
        }
        else if (otherError.IsError(ErrorCodeValue::NodeNotFound))
        {
            return ErrorCodeValue::NodeNotFound;
        }
        else if (otherError.IsError(ErrorCodeValue::NodeIsUp))
        {
            return ErrorCodeValue::InvalidState;
        }
        else
        {
            return otherError;
        }
    }
    else if (thisError.IsError(ErrorCodeValue::NodeIsUp))
    {
        if (otherError.IsError(ErrorCodeValue::NodeIsUp))
        {
            return ErrorCodeValue::NodeIsUp;
        }
        else if (otherError.IsError(ErrorCodeValue::NodeNotFound) ||
                 otherError.IsSuccess())
        {
            return ErrorCodeValue::InvalidState;
        }
        else
        {
            return otherError;
        }
    }
    else
    {
        return thisError;
    }
}

void ServiceAdminClient::NodeStateRemovedAsycOperation::Complete(AsyncOperationSPtr const& operation, ErrorCode error)
{
    TryComplete(operation, error);
}

ServiceAdminClient::RecoverPartitionsAsyncOperation::RecoverPartitionsAsyncOperation(
    ServiceAdminClient & serviceAdminClient,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
      serviceAdminClient_(serviceAdminClient),
      activityHeader_(activityHeader),
      timeout_(timeout),
      isRequestToFMMComplete_(false),
      isRequestToFMComplete_(false),
      lockObject_()
{
}

ErrorCode ServiceAdminClient::RecoverPartitionsAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<RecoverPartitionsAsyncOperation>(operation);
    return thisPtr->Error;
}

void ServiceAdminClient::RecoverPartitionsAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    MessageUPtr requestToFMM = CreateRequestMessage();
    RSMessage::AddActivityHeader(*requestToFMM, activityHeader_);
    auto requestToFMMOperation = serviceAdminClient_.transport_.BeginRequestToFMM(std::move(requestToFMM), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout_, OnRequestToFMMCompleted, thisSPtr);

    MessageUPtr requestToFM = CreateRequestMessage();
    RSMessage::AddActivityHeader(*requestToFM, activityHeader_);
    auto requestToFMOperation = serviceAdminClient_.transport_.BeginRequestToFM(std::move(requestToFM), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout_, OnRequestToFMCompleted, thisSPtr);
}

void ServiceAdminClient::RecoverPartitionsAsyncOperation::OnRequestToFMMCompleted(AsyncOperationSPtr const& requestOperation)
{
    auto thisPtr = AsyncOperation::Get<RecoverPartitionsAsyncOperation>(requestOperation->Parent);

    MessageUPtr reply;
    ErrorCode error = thisPtr->serviceAdminClient_.transport_.EndRequestToFMM(requestOperation, reply);

    if (error.IsSuccess())
    {
        AcquireExclusiveLock lock(thisPtr->lockObject_);

        error = TryExtractErrorCodeFromBasicFailoverReplyMessage(*reply);

        thisPtr->isRequestToFMMComplete_ = true;
        thisPtr->errorFMM_ = error;

        if (!error.IsSuccess())
        {
            thisPtr->Complete(requestOperation->Parent, error);
        }
        else if (thisPtr->isRequestToFMComplete_)
        {
            error = OnRequestCompleted(thisPtr->errorFMM_, thisPtr->errorFM_);
            thisPtr->Complete(requestOperation->Parent, error);
        }
    }
    else
    {
        thisPtr->Complete(requestOperation->Parent, error);
    }
}

void ServiceAdminClient::RecoverPartitionsAsyncOperation::OnRequestToFMCompleted(AsyncOperationSPtr const& requestOperation)
{
    auto thisPtr = AsyncOperation::Get<RecoverPartitionsAsyncOperation>(requestOperation->Parent);

    MessageUPtr reply;
    ErrorCode error = thisPtr->serviceAdminClient_.transport_.EndRequestToFM(requestOperation, reply);

    if (error.IsSuccess())
    {
        AcquireExclusiveLock lock(thisPtr->lockObject_);

        error = TryExtractErrorCodeFromBasicFailoverReplyMessage(*reply);

        thisPtr->isRequestToFMComplete_ = true;
        thisPtr->errorFM_ = error;

        if (!error.IsSuccess())
        {
            thisPtr->Complete(requestOperation->Parent, error);
        }
        else if (thisPtr->isRequestToFMMComplete_)
        {
            error = OnRequestCompleted(thisPtr->errorFM_, thisPtr->errorFMM_);
            thisPtr->Complete(requestOperation->Parent, error);
        }
    }
    else
    {
        thisPtr->Complete(requestOperation->Parent, error);
    }
}

ErrorCode ServiceAdminClient::RecoverPartitionsAsyncOperation::OnRequestCompleted(ErrorCode thisError, ErrorCode otherError)
{
    return thisError.IsSuccess() ? otherError : thisError;
}

void ServiceAdminClient::RecoverPartitionsAsyncOperation::Complete(AsyncOperationSPtr const& operation, ErrorCode error)
{
    TryComplete(operation, error);
}

ServiceAdminClient::RecoverAllPartitionsAsyncOperation::RecoverAllPartitionsAsyncOperation(
    ServiceAdminClient & serviceAdminClient,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const & parent)
    : RecoverPartitionsAsyncOperation(serviceAdminClient, activityHeader, timeout, callback, parent)
{
}

MessageUPtr ServiceAdminClient::RecoverAllPartitionsAsyncOperation::CreateRequestMessage() const
{
    return RSMessage::GetRecoverPartitions().CreateMessage<RecoverPartitionsRequestMessageBody>(RecoverPartitionsRequestMessageBody());
}

ServiceAdminClient::RecoverSystemPartitionsAsyncOperation::RecoverSystemPartitionsAsyncOperation(
    ServiceAdminClient & serviceAdminClient,
    FabricActivityHeader const& activityHeader,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const & parent)
    : RecoverPartitionsAsyncOperation(serviceAdminClient, activityHeader, timeout, callback, parent)
{
}

MessageUPtr ServiceAdminClient::RecoverSystemPartitionsAsyncOperation::CreateRequestMessage() const
{
    return RSMessage::GetRecoverSystemPartitions().CreateMessage<RecoverSystemPartitionsRequestMessageBody>(RecoverSystemPartitionsRequestMessageBody());
}
