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
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;
using namespace Management;
using namespace Management::ClusterManager;
using namespace Management::GatewayResourceManager;
using namespace ClientServerTransport;
using namespace Query;
using namespace ServiceModel::ModelV2;

StringLiteral const TraceComponent("GatewayResourceManagerAgent");

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class GatewayResourceManagerAgent::DispatchMessageAsyncOperationBase 
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::GatewayResourceManager>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IGatewayResourceManager & service,
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

    __declspec(property(get=get_Service)) IGatewayResourceManager & Service;
    IGatewayResourceManager & get_Service() { return service_; }

    __declspec(property(get=get_Request)) Message & Request;
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
protected:
    MessageUPtr reply_;

private:

    IGatewayResourceManager & service_;
    MessageUPtr request_; 
    IpcReceiverContextUPtr receiverContext_;

};

//------------------------------------------------------------------------------
// GatewayResourceManagerAgent
//

ErrorCode GatewayResourceManagerAgent::Create(__out shared_ptr<GatewayResourceManagerAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    auto hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    ComPointer<ComFabricRuntime> castedRuntimeCPtr;
    hr = runtimeCPtr->QueryInterface(CLSID_ComFabricRuntime, castedRuntimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    auto runtimeImpl = castedRuntimeCPtr->Runtime;

    FabricNodeContextSPtr fabricNodeContext;
    auto error = runtimeImpl->Host.GetFabricNodeContext(fabricNodeContext);
    if(!error.IsSuccess())
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

    IpcClient & ipcClient = runtimeImpl->Host.Client;

    shared_ptr<GatewayResourceManagerAgent> agentSPtr(new GatewayResourceManagerAgent(nodeInstance, ipcClient, runtimeCPtr));

    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        result.swap(agentSPtr);
    }

    return error;
}

GatewayResourceManagerAgent::GatewayResourceManagerAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : ComponentRoot()
    , NodeTraceComponent(nodeInstance)
    , routingAgentProxyUPtr_()
    , registeredServiceLocations_()
    , lock_()
    , runtimeCPtr_(runtimeCPtr)
    , queryMessageHandler_(nullptr)
    , servicePtr_()
{
    WriteInfo(TraceComponent, "{0} ctor", this->TraceId);

    auto temp = ServiceRoutingAgentProxy::Create(nodeInstance, ipcClient, *this);
    routingAgentProxyUPtr_.swap(temp);
}

GatewayResourceManagerAgent::~GatewayResourceManagerAgent()
{
    WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
}

ErrorCode GatewayResourceManagerAgent::Initialize()
{
    return routingAgentProxyUPtr_->Open();
}

void GatewayResourceManagerAgent::Release()
{
    routingAgentProxyUPtr_->Close().ReadValue();
}

void GatewayResourceManagerAgent::RegisterGatewayResourceManager(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    IGatewayResourceManagerPtr const & rootedService,
    __out wstring & serviceAddress)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);

    this->ReplaceServiceLocation(serviceLocation);

    auto selfRoot = this->CreateComponentRoot();

    // Create and register the query handlers
    queryMessageHandler_ = make_unique<QueryMessageHandler>(*selfRoot, QueryAddresses::GRMAddressSegment);
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

    servicePtr_ = rootedService;
    routingAgentProxyUPtr_->RegisterMessageHandler(
        serviceLocation,
        [this, rootedService](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
    {
        this->DispatchMessage(rootedService, message, receiverContext);
    });

    serviceAddress = wformatString("{0}", serviceLocation);
}

void GatewayResourceManagerAgent::UnregisterGatewayResourceManager(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);

    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            serviceLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }
    }

    // Expect to find location in our own list, but calling UnregisterMessageHandler()
    // is harmless even if we did not find it
    //
    routingAgentProxyUPtr_->UnregisterMessageHandler(serviceLocation);

    if (queryMessageHandler_)
    {
        queryMessageHandler_->Close();
    }
}

void GatewayResourceManagerAgent::ReplaceServiceLocation(SystemServiceLocation const & serviceLocation)
{
    // If there is already a handler registration for this partition+replica, make sure its handler is
    // unregistered from the routing agent proxy to be more robust to re-opened replicas
    // that may not have cleanly unregistered (e.g. rudely aborted).
    //
    bool foundExisting = false;
    SystemServiceLocation existingLocation;
    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            foundExisting = true;
            existingLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }

        registeredServiceLocations_.push_back(serviceLocation);
    }

    if (foundExisting)
    {
        routingAgentProxyUPtr_->UnregisterMessageHandler(existingLocation);
    }
}

AsyncOperationSPtr GatewayResourceManagerAgent::BeginProcessQuery(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Trace.WriteInfo(TraceComponent, "Inside agent/BeginProcessQuery");
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode GatewayResourceManagerAgent::EndProcessQuery(
    AsyncOperationSPtr const & operation,
    Transport::MessageUPtr & reply)
{
    return GatewayResourceManager::ProcessQueryAsyncOperation::End(operation, reply);
}

AsyncOperationSPtr GatewayResourceManagerAgent::BeginGetGatewayResourceList(
    GatewayResourceQueryDescription const & gatewayQueryDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ScopedHeap heap;

    LPCWSTR gatewayResourceName = heap.AddString(gatewayQueryDescription.GatewayNameFilter);
    return (*servicePtr_.get()).BeginGetGatewayResourceList(
        gatewayResourceName,
        timeout,
        callback,
        parent);
}

ErrorCode GatewayResourceManagerAgent::EndGetGatewayResourceList(
    AsyncOperationSPtr const & operation,
    MessageUPtr & realReply)
{
    vector<wstring> list;
    Trace.WriteInfo(TraceComponent, "GatewayResourceManager - Calling EndGetGatewayResourceList");

    IFabricStringListResult * gatewayResourceQueryResult;
    ErrorCode error = (*servicePtr_.get()).EndGetGatewayResourceList(operation, &gatewayResourceQueryResult);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "GatewayResourceManager - EndGetGatewayResourceList failed, error='{0}'", error);
        return error;
    }

    FABRIC_STRING_LIST strings;
    error = ErrorCode::FromHResult(gatewayResourceQueryResult->GetStrings(&strings.Count, &strings.Items));
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "GatewayResourceManager - EndGetGatewayResourceList failed, error='{0}'", error);
        return error;
    }

    error = StringList::FromPublicApi(
        strings, list);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "GatewayResourceManager - EndGetGatewayResourceList converting to GatewayResourceDescription list failed, error = '{0}'", error);
        return error;
    }

    QueryResult queryResult = ServiceModel::QueryResult(move(list));

    realReply = GatewayResourceManagerMessage::GetQueryReply(move(queryResult));
    return queryResult.QueryProcessingError;
}

void GatewayResourceManagerAgent::DispatchMessage(
    IGatewayResourceManagerPtr const & servicePtr,
    __in Transport::MessageUPtr & message, 
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
    if (message->Actor == Actor::GatewayResourceManager)
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteInfo(TraceComponent, "{0} missing TimeoutHeader: message Id = {1}", this->TraceId, message->MessageId);
            routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
            return;
        }

        vector<ComponentRootSPtr> roots;
        roots.push_back(servicePtr.get_Root());
        roots.push_back(this->CreateComponentRoot());

        if (message->Action == GatewayResourceManagerTcpMessage::CreateGatewayResourceAction)
        {
            auto operation = AsyncOperation::CreateAndStart<CreateGatewayResourceAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == GatewayResourceManagerTcpMessage::DeleteGatewayResourceAction)
        {
            auto operation = AsyncOperation::CreateAndStart<DeleteGatewayResourceAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == QueryTcpMessage::QueryAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GatewayResourceManagerAgentQueryMessageHandler>(
                *this,
                queryMessageHandler_,
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete2(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));

            this->OnDispatchMessageComplete2(operation, true);
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid Actor {1}: message Id = {2}", this->TraceId, message->Actor, message->MessageId);
        routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void GatewayResourceManagerAgent::OnDispatchMessageComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode error = DispatchMessageAsyncOperationBase::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}


class GatewayResourceManagerAgent::GatewayResourceManagerAgentQueryMessageHandler :
    public TimedAsyncOperation
{
public:

    GatewayResourceManagerAgentQueryMessageHandler(
        __in GatewayResourceManagerAgent & owner,
        __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
        MessageUPtr && request,
        IpcReceiverContextUPtr && requestContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root) :
        TimedAsyncOperation(timeout, callback, root)
        , owner_(owner)
        , queryMessageHandler_(queryMessageHandler)
        , error_(ErrorCodeValue::Success)
        , request_(move(request))
        , requestContext_(move(requestContext))
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        auto operation = this->BeginAcceptRequest(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnAcceptRequestComplete(operation, false); },
            thisSPtr);
        this->OnAcceptRequestComplete(operation, true);
    }

    void OnAcceptRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->EndAcceptRequest(operation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation,
        __out MessageUPtr & reply,
        __out IpcReceiverContextUPtr & requestContext)
    {
        auto casted = AsyncOperation::End<GatewayResourceManagerAgentQueryMessageHandler>(asyncOperation);

        if (casted->Error.IsSuccess())
        {
            if (casted->reply_)
            {
                reply = move(casted->reply_);
            }
        }
        // request context is used to reply with errors to client, so always return it
        requestContext = move(casted->requestContext_);
        return casted->Error;
    }

protected:

    Common::AsyncOperationSPtr BeginAcceptRequest(
        Common::TimeSpan const timespan,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return queryMessageHandler_->BeginProcessQueryMessage(*request_, timespan, callback, parent);
    }

    Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &operation)
    {
        MessageUPtr reply;
        ErrorCode error = queryMessageHandler_->EndProcessQueryMessage(operation, reply);
        if (error.IsSuccess())
        {
            reply_ = std::move(reply);
        }

        auto thisSPtr = this->shared_from_this();
        this->TryComplete(thisSPtr, error);

        return error;
    }

private:

    Query::QueryMessageHandlerUPtr & queryMessageHandler_;
    GatewayResourceManagerAgent & owner_;
    Common::ErrorCodeValue::Enum error_;
    Transport::MessageUPtr request_;
    Transport::MessageUPtr reply_;
    Transport::IpcReceiverContextUPtr requestContext_;
};

void GatewayResourceManagerAgent::OnDispatchMessageComplete2(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode error = GatewayResourceManagerAgentQueryMessageHandler::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}

class GatewayResourceManagerAgent::CreateGatewayResourceAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    CreateGatewayResourceAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IGatewayResourceManager & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~CreateGatewayResourceAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CreateGatewayResourceMessageBody body;

        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginCreateOrUpdateGatewayResource(
                body.GatewayDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndCreateOrUpdateGatewayResource(operation, &result_);

        if (!error.IsSuccess())
        {
            return error;
        }

        std::wstring descriptionStr(result_->get_String());
        CreateGatewayResourceMessageBody replyBody(move(descriptionStr));
        reply = GatewayResourceManagerMessage::GetClientOperationSuccess(replyBody);
        return error;
    }

    IFabricStringResult* result_;
};

class GatewayResourceManagerAgent::DeleteGatewayResourceAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    DeleteGatewayResourceAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IGatewayResourceManager & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~DeleteGatewayResourceAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        DeleteGatewayResourceMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginDeleteGatewayResource(
                body.Name,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndDeleteGatewayResource(operation);

        if (error.IsSuccess())
        {
            reply = GatewayResourceManagerMessage::GetClientOperationSuccess();
        }

        return error;
    }
};
