// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#ifdef PLATFORM_UNIX
#include <arpa/inet.h>
#endif

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability::FailoverManagerComponent;
using namespace Management::NetworkInventoryManager;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const NIMNetworkInventoryProvider("NetworkInventoryService");

class NetworkPeriodicTaskAsyncOperation : public TimedAsyncOperation
{
    DENY_COPY(NetworkPeriodicTaskAsyncOperation)

public:
    NetworkPeriodicTaskAsyncOperation(
        TimeSpan const refreshInterval,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operation)
        : TimedAsyncOperation(refreshInterval, callback, operation)
    {
    }

    virtual ~NetworkPeriodicTaskAsyncOperation()
    {
    }
};

//--------
// Send a TRequest message to NIM agent and get the TReply.
template <class TRequest, class TReply>
class NetworkInventoryService::NIMRequestAsyncOperation : public AsyncOperation
{
public:

    NIMRequestAsyncOperation(
        __in NetworkInventoryService & owner,
        __in const NodeInstance nodeInstance,
        TRequest const & requestMessageBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        nodeInstance_(nodeInstance),
        timeoutHelper_(timeout),
        requestMessageBody_(requestMessageBody)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation,
        __out TReply & reply)
    {
        auto thisPtr = AsyncOperation::End<NIMRequestAsyncOperation>(operation);
        reply = move(thisPtr->reply_);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(NIMNetworkInventoryProvider, "NetworkInventoryService: Sending message to NIM agent. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateMessageRequest();

        NodeId target = nodeInstance_.Id;

        auto operation = owner_.FM.Federation.BeginRouteRequest(move(request),
            nodeInstance_.Id,
            nodeInstance_.InstanceId,
            true,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnRequestCompleted(operation, false); },
            thisSPtr);

        OnRequestCompleted(operation, true);
    }

    void OnRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr message;
        ErrorCode error = owner_.FM.Federation.EndRouteRequest(operation, message);

        if (!error.IsSuccess())
        {
            WriteWarning(NIMNetworkInventoryProvider, "End(NIMRequestAsyncOperation): ErrorCode={0}", error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (!message->GetBody(reply_))
        {
            error = ErrorCodeValue::InvalidMessage;
            WriteWarning(NIMNetworkInventoryProvider, "GetBody<TReply> failed: Message={0}, ErrorCode={1}",
                *message, error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

private:

    MessageUPtr CreateMessageRequest()
    {
        MessageUPtr message = make_unique<Transport::Message>(requestMessageBody_);

        message->Headers.Add(ActorHeader(Actor::NetworkInventoryAgent));
        message->Headers.Add(ActionHeader(TRequest::ActionName));

        WriteNoise(NIMNetworkInventoryProvider, "CreateActivateHostedServiceRequest: Message={0}, Body={1}",
            *message, requestMessageBody_);

        return move(message);
    }

    TimeoutHelper timeoutHelper_;
    NetworkInventoryService & owner_;
    NodeInstance nodeInstance_;

    TRequest const & requestMessageBody_;
    TReply reply_;
};



NetworkInventoryService::NetworkInventoryService(
    FailoverManager & fm)
    : fm_(fm)
{
    WriteInfo(NIMNetworkInventoryProvider, "NetworkInventoryService: constructing...");
}

NetworkInventoryService::~NetworkInventoryService()
{
    WriteInfo(NIMNetworkInventoryProvider, "NetworkInventoryService: destructing...");
}

ErrorCode NetworkInventoryService::Initialize()
{
    ErrorCode error = ErrorCodeValue::Success;

    auto bottom = NetworkInventoryManagerConfig::GetConfig().NIMMacPoolStart;
    auto top = NetworkInventoryManagerConfig::GetConfig().NIMMacPoolEnd;

    macAddressPool_ = make_shared<NIMMACAllocationPool>();
    this->macAddressPool_->Initialize(bottom, top);

    this->vsidPool_ = make_shared<NIMVSIDAllocationPool>();
    this->vsidPool_->InitializeScalar(NetworkInventoryManagerConfig::GetConfig().NIMVSIDRangeStart, 
        NetworkInventoryManagerConfig::GetConfig().NIMVSIDRangeEnd);

    this->allocationManagerUPtr_ = make_unique<NetworkInventoryAllocationManager>(*this);
    
    error = this->allocationManagerUPtr_->Initialize(this->macAddressPool_, this->vsidPool_);
    if (!error.IsSuccess())
    {
        return error;
    }

    //--------
    // Register to handle federation messages from nodes.
    RegisterRequestHandler();
   
    InternalStartRefreshProcessing();

    return error;
}

void NetworkInventoryService::Close()
{
    WriteInfo(NIMNetworkInventoryProvider, "NetworkInventoryService: closing...");

    if (periodicAsyncTask_ != nullptr)
    {
        periodicAsyncTask_->Cancel();
        periodicAsyncTask_.reset();
    }

    if (allocationManagerUPtr_ != nullptr)
    {
        allocationManagerUPtr_->Close();
    }

    UnregisterRequestHandler();
}

void NetworkInventoryService::RegisterRequestHandler()
{
    // Register the one way and request-reply message handlers
    fm_.Federation.RegisterMessageHandler(
        Actor::NetworkInventoryService,
        [this] (MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
    {
        WriteError(NIMNetworkInventoryProvider, "{0} received a oneway message for NetworkInventoryService: {1}",
            fm_.TraceId,
            *message);
        oneWayReceiverContext->Reject(ErrorCodeValue::InvalidMessage);
    },
        [this] (Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
    { 
        this->ProcessMessage(message, requestReceiverContext); 
    },
    false /*dispatchOnTransportThread*/);
}

void NetworkInventoryService::UnregisterRequestHandler()
{
    fm_.Federation.UnRegisterMessageHandler(Actor::NetworkInventoryService);
}

void NetworkInventoryService::ProcessMessage(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    wstring const & action = message->Action;
    if (action == Management::NetworkInventoryManager::NetworkAllocationRequestMessage::ActionName)
    {
        this->OnNetworkAllocationRequest(message, context);
    }
    else if (action == Management::NetworkInventoryManager::PublishNetworkTablesMessageRequest::ActionName)
    {
        this->OnRequestPublishNetworkTablesRequest(message, context);
    }    
    else if (action == Management::NetworkInventoryManager::NetworkCreateRequestMessage::ActionName)
    {
        this->OnNetworkCreateRequest(message, context);
    }
    else if (action == NIMMessage::GetCreateNetwork().Action)
    {
        this->OnNetworkCreateRequest(message, context);
    }
    else if (action == Management::NetworkInventoryManager::NetworkRemoveRequestMessage::ActionName)
    {
        this->OnNetworkNodeRemoveRequest(message, context);
    }
    else if (action == NIMMessage::GetDeleteNetwork().Action)
    {
        this->OnNetworkRemoveRequest(message, context);
    }
    else if (action == NIMMessage::GetValidateNetwork().Action)
    {
        this->OnNetworkValidateRequest(message, context);
    }
    else if (action == Management::NetworkInventoryManager::NetworkEnumerateRequestMessage::ActionName)
    {
        this->OnNetworkEnumerationRequest(message, context);
    }
    else
    {
        WriteWarning(NIMNetworkInventoryProvider, "Dropping unsupported message: {0}", message);
    }
}

#pragma region Message Processing

void NetworkInventoryService::OnNetworkAllocationRequest(
    __in Transport::MessageUPtr & message, 
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    Management::NetworkInventoryManager::NetworkAllocationRequestMessage requestBody;
    if (!message->GetBody<Management::NetworkInventoryManager::NetworkAllocationRequestMessage>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<Management::NetworkInventoryManager::NetworkAllocationRequestMessage> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnNetworkAllocationRequest: id={0}",
        requestBody.NetworkName);

    NetworkAllocationResponseMessage allocationResponse;
    auto networkName = requestBody.NetworkName;

    auto a = allocationManagerUPtr_->BeginAcquireEntryBlock(requestBody,
        FederationConfig::GetConfig().MessageTimeout,
        [this, networkName, ctx = context.release()](AsyncOperationSPtr const & contextSPtr) mutable -> void
    {
        auto context = std::unique_ptr<RequestReceiverContext>(ctx);
        std::shared_ptr<NetworkAllocationResponseMessage> reply;
        auto error = allocationManagerUPtr_->EndAcquireEntryBlock(contextSPtr, reply);

        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(*reply);
        context->Reply(move(replyMsg));

        if (error.IsSuccess())
        {
            //--------
            // Enqueue a job to send the mapping table
            error = allocationManagerUPtr_->EnqueuePublishNetworkMappings(networkName);
            if (!error.IsSuccess())
            {
                WriteWarning(NIMNetworkInventoryProvider, "OnNetworkAllocationRequest failed to EnqueuePublishNetworkMappings for network: {0}, ErrorCode={1}",
                    networkName,
                    error);
            }
        }
    });

    context.release();
}


void NetworkInventoryService::OnNetworkCreateRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)

{
    ErrorCode error = ErrorCodeValue::Success;

    Management::NetworkInventoryManager::CreateNetworkMessageBody requestBody;
    if (!message->GetBody<CreateNetworkMessageBody>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<Management::NetworkInventoryManager::CreateNetworkMessageBody> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    auto a = this->BeginNetworkCreate(requestBody,
        FederationConfig::GetConfig().MessageTimeout,
        [this, ctx = context.release(), requestBody](AsyncOperationSPtr const & contextSPtr) mutable -> void
    {
        auto context = std::unique_ptr<RequestReceiverContext>(ctx);
        std::shared_ptr<NetworkCreateResponseMessage> replyCreate;
        auto error = this->EndNetworkCreate(contextSPtr, replyCreate);

        if (error.IsSuccess())
        {
        }

        Reliability::BasicFailoverReplyMessageBody reply(error.ReadValue());
        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(reply);
        context->Reply(move(replyMsg));
    },
        this->FM.CreateAsyncOperationRoot());

    context.release();
}

Common::AsyncOperationSPtr NetworkInventoryService::BeginNetworkCreate(
    CreateNetworkMessageBody & requestBody,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    ErrorCode error = ErrorCodeValue::Success;

    auto netName = requestBody.NetworkDescription.Name;
    auto subnet = requestBody.NetworkDescription.NetworkAddressPrefix;
    int prefix = 0;
    std::wstring gateway;

    vector<wstring> segments;
    StringUtility::Split<wstring>(subnet, segments, L"//");
    if (segments.size() != 2)
    {
        WriteError(NIMNetworkInventoryProvider, "OnNetworkCreateRequest Failed: wrong subnet specified: [{0}]",
            subnet);

        error = ErrorCode(ErrorCodeValue::InvalidAddress);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    uint ipScalar = 0;
    StringUtility::ParseIntegralString<int, false>::Try(segments[1], prefix, 10U);
    error = NIMIPv4AllocationPool::GetScalarFromIpAddress(segments[0], ipScalar);
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkInventoryProvider, "OnNetworkCreateRequest Failed: wrong subnet format: [{0}]",
            subnet);

        error = ErrorCode(ErrorCodeValue::InvalidAddress);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    subnet = segments[0];
    //--------
    // GW is the first IP in the subnet
    gateway = NIMIPv4AllocationPool::GetIpString(++ipScalar);

    auto networkDefinition = make_shared<NetworkDefinition>(
        netName,
        netName,
        subnet,
        prefix,
        gateway,
        0);

    NetworkCreateRequestMessage networkCreateReq(
        netName,
        networkDefinition);

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnNetworkCreateRequest: id={0}",
        networkCreateReq.NetworkName);

    auto a = allocationManagerUPtr_->BeginNetworkCreate(networkCreateReq,
        timeout,
        callback);

    return a;
}

Common::ErrorCode NetworkInventoryService::EndNetworkCreate(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkCreateResponseMessage> & reply)
{
    auto error = operation->Error;
    if (error.IsSuccess())
    {
        error = allocationManagerUPtr_->EndNetworkCreate(operation, reply);
    }

    return error;
}

Common::AsyncOperationSPtr NetworkInventoryService::BeginNetworkAssociation(
    std::wstring const & networkName,
    std::wstring const & applicationName,
    Common::AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    return allocationManagerUPtr_->BeginAssociateApplication(networkName, applicationName, true, callback, state);
}

Common::ErrorCode NetworkInventoryService::EndNetworkAssociation(
    Common::AsyncOperationSPtr const & operation)
{
    return allocationManagerUPtr_->EndAssociateApplication(operation);
}

Common::AsyncOperationSPtr NetworkInventoryService::BeginNetworkDissociation(
    std::wstring const & networkName,
    std::wstring const & applicationName,
    Common::AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    return allocationManagerUPtr_->BeginAssociateApplication(networkName, applicationName, false, callback, state);
}

Common::ErrorCode NetworkInventoryService::EndNetworkDissociation(
    Common::AsyncOperationSPtr const & operation)
{
    return allocationManagerUPtr_->EndAssociateApplication(operation);
}

void NetworkInventoryService::OnNetworkRemoveRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    DeleteNetworkMessageBody requestBody;
    if (!message->GetBody<DeleteNetworkMessageBody>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<DeleteNetworkMessageBody> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    Management::NetworkInventoryManager::NetworkRemoveRequestMessage deleteRequest(requestBody.DeleteNetworkDescription.NetworkName);

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnNetworkRemoveRequest: id={0}",
        deleteRequest.NetworkId);

    auto netName = requestBody.DeleteNetworkDescription.NetworkName;
    auto a = allocationManagerUPtr_->BeginNetworkRemove(deleteRequest,
        FederationConfig::GetConfig().MessageTimeout,
        [this, ctx = context.release(), netName](AsyncOperationSPtr const & contextSPtr) mutable -> void
    {
        auto context = std::unique_ptr<RequestReceiverContext>(ctx);
        std::shared_ptr<NetworkErrorCodeResponseMessage> reply;
        auto error = allocationManagerUPtr_->EndNetworkRemove(contextSPtr, reply);

        if (error.IsSuccess())
        {
        }

        Reliability::BasicFailoverReplyMessageBody replyBody(error.ReadValue());
        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(replyBody);
        context->Reply(move(replyMsg));

    });

    context.release();
}


void NetworkInventoryService::OnNetworkValidateRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    ValidateNetworkMessageBody requestBody;
    if (!message->GetBody<ValidateNetworkMessageBody>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<ValidateNetworkMessageBody> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    for (const auto & networkName : requestBody.Networks)
    {
        auto network = allocationManagerUPtr_->GetNetworkByName(networkName);
        if (!network) 
        {
            error = ErrorCode(ErrorCodeValue::NetworkNotFound);
        }
    }

    Reliability::BasicFailoverReplyMessageBody reply(error.ReadValue());
    Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(reply);
    context->Reply(move(replyMsg));
}


void NetworkInventoryService::OnNetworkNodeRemoveRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    NetworkRemoveRequestMessage requestBody;
    if (!message->GetBody<NetworkRemoveRequestMessage>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<Management::NetworkInventoryManager::NetworkRemoveRequestMessage> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnNetworkNodeRemoveRequest: network: {0}, nodeId: {1}",
        requestBody.NetworkId, requestBody.NodeId);

    auto a = allocationManagerUPtr_->BeginNetworkNodeRemove(requestBody,
        FederationConfig::GetConfig().MessageTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & contextSPtr) mutable -> void
    {
        auto context = std::unique_ptr<RequestReceiverContext>(ctx);
        std::shared_ptr<NetworkErrorCodeResponseMessage> reply;
        auto error = allocationManagerUPtr_->EndNetworkNodeRemove(contextSPtr, reply);

        if (error.IsSuccess())
        {
        }

        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(*reply);
        context->Reply(move(replyMsg));
    });

    context.release();
}

void NetworkInventoryService::OnRequestPublishNetworkTablesRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    PublishNetworkTablesMessageRequest requestBody;
    if (!message->GetBody<PublishNetworkTablesMessageRequest>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<PublishNetworkTablesMessageRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnRequestPublishNetworkTablesRequest: network: {0}, nodeId: {1}",
        requestBody.NetworkId, requestBody.NodeId);

    error = allocationManagerUPtr_->EnqueuePublishNetworkMappings(requestBody.NetworkId);
    
    if (!error.IsSuccess())
    {
        WriteWarning(NIMNetworkInventoryProvider, "OnRequestPublishNetworkTablesRequest failed to EnqueuePublishNetworkMappings for network: {0}, ErrorCode={1}",
            requestBody.NetworkId,
            error);
    }

    std::shared_ptr<NetworkErrorCodeResponseMessage> reply = make_shared<NetworkErrorCodeResponseMessage>(0, error);
    Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(*reply);
    context->Reply(move(replyMsg));
}

void NetworkInventoryService::OnNetworkEnumerationRequest(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    ErrorCode error = ErrorCodeValue::Success;

    Management::NetworkInventoryManager::NetworkEnumerateRequestMessage requestBody;
    if (!message->GetBody<Management::NetworkInventoryManager::NetworkEnumerateRequestMessage>(requestBody))
    {
        error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(NIMNetworkInventoryProvider, "GetBody<Management::NetworkInventoryManager::NetworkEnumerateRequestMessage> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        context->Reject(error);
        return;
    }

    WriteNoise(NIMNetworkInventoryProvider, "Processing OnNetworkEnumerationRequest: id={0}",
        requestBody.NetworkNameFilter);

    auto a = allocationManagerUPtr_->BeginNetworkEnumerate(requestBody,
        FederationConfig::GetConfig().MessageTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & contextSPtr) mutable -> void
    {
        auto context = std::unique_ptr<RequestReceiverContext>(ctx);
        std::shared_ptr<NetworkEnumerateResponseMessage> reply;
        auto error = allocationManagerUPtr_->EndNetworkEnumerate(contextSPtr, reply);

        if (error.IsSuccess())
        {
        }

        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(*reply);
        context->Reply(move(replyMsg));

    });

    context.release();
}


ErrorCode NetworkInventoryService::EnumerateNetworks(
    QueryArgumentMap const & queryArgs,
    QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    ErrorCode error = ErrorCodeValue::Success;

    WriteNoise(NIMNetworkInventoryProvider, "Processing EnumerateNetworks: id={0}",
        activityId);

    wstring desiredNetworkName;
    bool netName = queryArgs.TryGetValue(Query::QueryResourceProperties::Network::NetworkName, desiredNetworkName);

    wstring desiredStatusFilter;
    bool netFilter = queryArgs.TryGetValue(Query::QueryResourceProperties::Network::NetworkStatusFilter, desiredStatusFilter);

    wstring continuationToken;
    queryArgs.TryGetValue(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    auto networkList = allocationManagerUPtr_->NetworkEnumerate(
        [netName, &desiredNetworkName, netFilter, &desiredStatusFilter, &continuationToken](ModelV2::NetworkResourceDescriptionQueryResult const & networkDesc)
    {
        if (netName)
        {
            if (StringUtility::Compare(desiredNetworkName, networkDesc.NetworkName) != 0)
            {
                return false;
            }
        }

        return true;
    });

    //--------
    // Sort the networks by the name.
    sort(networkList.begin(), networkList.end(), [](ModelV2::NetworkResourceDescriptionQueryResult const & l, ModelV2::NetworkResourceDescriptionQueryResult const & r)
    {
        return l.NetworkName < r.NetworkName;
    });

    queryResult = QueryResult(move(networkList));
    return error;
}

ErrorCode NetworkInventoryService::GetNetworkApplicationList(
    ServiceModel::QueryArgumentMap const & queryArgs,
    ServiceModel::QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    ErrorCode error = ErrorCodeValue::Success;

    WriteNoise(NIMNetworkInventoryProvider, "Processing GetNetworkApplicationList: id={0}",
        activityId);

    wstring desiredNetworkName;
    queryArgs.TryGetValue(Query::QueryResourceProperties::Network::NetworkName, desiredNetworkName);

    auto applicationList = allocationManagerUPtr_->GetNetworkApplicationList(desiredNetworkName);

    queryResult = QueryResult(move(applicationList));
    return error;
}


ErrorCode NetworkInventoryService::GetApplicationNetworkList(
    ServiceModel::QueryArgumentMap const & queryArgs,
    ServiceModel::QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    ErrorCode error = ErrorCodeValue::Success;

    WriteNoise(NIMNetworkInventoryProvider, "Processing GetApplicationNetworkList: id={0}",
        activityId);

    wstring desiredApplicationName;
    queryArgs.TryGetValue(Query::QueryResourceProperties::Application::ApplicationName, desiredApplicationName);

    auto networkList = allocationManagerUPtr_->GetApplicationNetworkList(desiredApplicationName);

    queryResult = QueryResult(move(networkList));
    return error;
}

std::vector<ServiceModel::ApplicationNetworkQueryResult> NetworkInventoryService::GetApplicationNetworkList(
    std::wstring const & applicationName)
{
    return allocationManagerUPtr_->GetApplicationNetworkList(applicationName);
}

ErrorCode NetworkInventoryService::GetNetworkNodeList(
    ServiceModel::QueryArgumentMap const & queryArgs,
    ServiceModel::QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    ErrorCode error = ErrorCodeValue::Success;

    WriteNoise(NIMNetworkInventoryProvider, "Processing GetNetworkNodeList: id={0}",
        activityId);

    wstring desiredNetworkName;
    queryArgs.TryGetValue(Query::QueryResourceProperties::Network::NetworkName, desiredNetworkName);

    auto nodeList = allocationManagerUPtr_->GetNetworkNodeList(desiredNetworkName);

    queryResult = QueryResult(move(nodeList));
    return error;
}

ErrorCode NetworkInventoryService::GetDeployedNetworkList(
    ServiceModel::QueryArgumentMap const & queryArgs,
    ServiceModel::QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(queryArgs);
    ErrorCode error = ErrorCodeValue::Success;

    WriteNoise(NIMNetworkInventoryProvider, "Processing GetDeployedNetworkList: id={0}",
        activityId);

    auto networkList = allocationManagerUPtr_->GetDeployedNetworkList();

    queryResult = QueryResult(move(networkList));
    return error;
}


#pragma endregion 

AsyncOperationSPtr NetworkInventoryService::BeginSendPublishNetworkTablesRequestMessage(
    PublishNetworkTablesRequestMessage const & params,
    const NodeInstance& nodeInstance,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryService::NIMRequestAsyncOperation<PublishNetworkTablesRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, nodeInstance, params, timeout, callback,
        this->FM.CreateAsyncOperationRoot());
}

ErrorCode NetworkInventoryService::EndSendPublishNetworkTablesRequestMessage(AsyncOperationSPtr const & operation,
    __out NetworkErrorCodeResponseMessage & reply)
{
    return NetworkInventoryService::NIMRequestAsyncOperation<PublishNetworkTablesRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}



ErrorCode NetworkInventoryService::InternalStartRefreshProcessing()
{
    TimeSpan refreshInterval = TimeSpan::FromSeconds(NetworkInventoryManagerConfig::GetConfig().NIMCleanupPeriodInSecs);
    
    auto operation = AsyncOperation::CreateAndStart<NetworkPeriodicTaskAsyncOperation>(
        refreshInterval,
        [this](AsyncOperationSPtr const &operation)
        {
            auto error = AsyncOperation::End(operation);
            if (error.ReadValue() == ErrorCodeValue::Timeout)
            {
                //--------
                // Clean up stale records.
                this->allocationManagerUPtr_->CleanUpNetworks();
            }
            else if (error.ReadValue() == ErrorCodeValue::OperationCanceled)
            {
                return;
            }
            else
            {
                WriteInfo(NIMNetworkInventoryProvider, "OnRefreshComplete failed: error={0}", error);
            }

            //--------
            // Start a new refresh cycle.
            InternalStartRefreshProcessing();
        },
        fm_.CreateAsyncOperationRoot());

    if (operation->Error.IsSuccess())
    {
        periodicAsyncTask_ = operation;
    }
    
    return operation->Error;
}

ErrorCode NetworkInventoryService::ProcessNIMQuery(Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    QueryResult & queryResult,
    Common::ActivityId const & activityId)
{
        ErrorCode errorCode = ErrorCodeValue::InvalidConfiguration;
        switch (queryName)
        {
        case Query::QueryNames::GetNetworkList:
            errorCode = EnumerateNetworks(queryArgs, queryResult, activityId);
            break;
        case Query::QueryNames::GetNetworkApplicationList:
            errorCode = GetNetworkApplicationList(queryArgs, queryResult, activityId);
            break;
        case Query::QueryNames::GetNetworkNodeList:
            errorCode = GetNetworkNodeList(queryArgs, queryResult, activityId);
            break;
        case Query::QueryNames::GetApplicationNetworkList:
            errorCode = GetApplicationNetworkList(queryArgs, queryResult, activityId);
            break;
        case Query::QueryNames::GetDeployedNetworkList:
            errorCode = GetDeployedNetworkList(queryArgs, queryResult, activityId);
            break;
        }

    return errorCode;
}