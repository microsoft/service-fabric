// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Api;
using namespace Communication::TcpServiceCommunication;
using namespace std;

static const StringLiteral TraceType("ServiceCommunicationTransport");

class ServiceCommunicationTransport::ClientTable
{
    DENY_COPY(ClientTable);
public:
    ClientTable(wstring const & traceId);

    bool UpdateIfNeeded(wstring const & clientId, ISendTarget::SPtr const & sendTarget, ReceiverContextUPtr && context, ServiceMethodCallDispatcherSPtr const & dispatcher);
    ClientConnectionStateInfoSPtr  GetClientConnectionStateInfo(wstring const & clientId);
    ClientConnectionStateInfoSPtr  GetClientConnectionStateInfoUsingSendTarget(wstring const & targetId);
    void Remove(wstring const & clientId);
    ISendTarget::SPtr GetSendTarget(wstring const & clientId);

    vector<ClientConnectionStateInfoSPtr> RemoveClients(wstring const & serviceInfo);

private:
    unordered_map<wstring /* client ID */, ClientConnectionStateInfoSPtr> clients_;
    //TODO: Optimize lookup to not to use 2 maps.
    unordered_map<wstring /* Send Target ID */, pair<wstring/*ClientId*/, wstring /*ServiceLocation*/ >> sendTargetToClientId_;
    wstring traceId_;
};

ClientConnectionStateInfoSPtr ServiceCommunicationTransport::ClientTable::GetClientConnectionStateInfo(
    wstring const & clientId)
{
    auto iter = clients_.find(clientId);
    if (iter == clients_.end())
    {
        return  nullptr;
    }
    return iter->second;
}

ClientConnectionStateInfoSPtr ServiceCommunicationTransport::ClientTable::GetClientConnectionStateInfoUsingSendTarget(
    wstring const & sendTargetId)
{
    auto iter = sendTargetToClientId_.find(sendTargetId);
    if (iter == sendTargetToClientId_.end())
    {
        return  nullptr;
    }

    auto clientId = iter->second.first;
    return GetClientConnectionStateInfo(clientId);

}

ServiceCommunicationTransport::ClientTable::ClientTable(wstring const &traceId)
{
    this->traceId_ = traceId;
}

bool ServiceCommunicationTransport::ClientTable::UpdateIfNeeded(wstring const & clientId, ISendTarget::SPtr const & sendTarget, ReceiverContextUPtr && context, ServiceMethodCallDispatcherSPtr const & dispatcher)
{

    auto iter = clients_.find(clientId);
    if (iter == clients_.end())
    {
        auto clientConnectionState = make_shared<ClientConnectionStateInfo>(sendTarget, dispatcher, clientId, move(context));
        clients_.emplace(pair<wstring, ClientConnectionStateInfoSPtr>(clientId, clientConnectionState));
        sendTargetToClientId_.emplace(pair<wstring, pair<wstring, wstring>>(sendTarget->Id(), pair<wstring, wstring>(clientId, dispatcher->ServiceInfo)));
        WriteNoise(TraceType, this->traceId_, "Updating Client  {0} Entry", clientId);
        return true;
    }
    return false;
}

void ServiceCommunicationTransport::ClientTable::Remove(wstring const & clientId)
{
    auto clientIditer = clients_.find(clientId);
    if (clientIditer == clients_.cend())
    {
        return;
    }

    auto connectionStateInfo = clientIditer->second;
    auto target = connectionStateInfo->Get_SendTarget();
    clients_.erase(clientIditer);
    auto iterator = sendTargetToClientId_.find(target->Id());

    if (iterator == sendTargetToClientId_.cend())
    {
        return;
    }
    sendTargetToClientId_.erase(iterator);
    WriteInfo(TraceType, this->traceId_, "Removed Client Entry {0} Connected to Service {1}", clientId, connectionStateInfo->Get_ServiceConnectedTo());

}

ISendTarget::SPtr ServiceCommunicationTransport::ClientTable::GetSendTarget(wstring const & clientId)
{
    auto iter = clients_.find(clientId);
    if (iter == clients_.cend())
    {
        return nullptr;
    }
    return iter->second->Get_SendTarget();
}

vector<ClientConnectionStateInfoSPtr> ServiceCommunicationTransport::ClientTable::RemoveClients(wstring const& serviceInfo)
{
    auto sendTargetsItr = clients_.begin();
    vector<ClientConnectionStateInfoSPtr> clientInfoList;
    WriteInfo(TraceType, this->traceId_, "Removing ClientTable Entries  for Service {0}", serviceInfo);
    while (sendTargetsItr != clients_.end())
    {
        WriteNoise(TraceType, this->traceId_, "Checking Client  Info {0}", sendTargetsItr->first);

        if (StringUtility::AreEqualCaseInsensitive(sendTargetsItr->second->Get_ServiceConnectedTo(), serviceInfo))
        {
            clientInfoList.push_back(sendTargetsItr->second);
            auto clientId = sendTargetsItr->first;
            ++sendTargetsItr;
            this->Remove(clientId);
        }
        else
        {
            ++sendTargetsItr;
        }
    }
    return clientInfoList;
}

class  ServiceCommunicationTransport::OnConnectAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(OnConnectAsyncOperation);

public:
    OnConnectAsyncOperation(
        ServiceMethodCallDispatcherSPtr & dispatcher,
        IClientConnectionPtr clientConnection,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , dispatcher_(dispatcher)
        , clientConnection_(clientConnection)
    {
    }

    ~OnConnectAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation, wstring & clientId)
    {
        auto casted = AsyncOperation::End<OnConnectAsyncOperation>(operation);
        clientId = casted->clientConnection_->Get_ClientId();
        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->dispatcher_->ConnectionHandler->BeginProcessConnect(
            clientConnection_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnConnectComplete(operation, false); },
            thisSPtr);
        this->OnConnectComplete(operation, true);

    }

private:

    void OnConnectComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->dispatcher_->ConnectionHandler->EndProcessConnect(operation);

        this->TryComplete(operation->Parent, error);
    }

    IClientConnectionPtr clientConnection_;
    ServiceMethodCallDispatcherSPtr dispatcher_;
};

class  ServiceCommunicationTransport::OnDisconnectAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(OnDisconnectAsyncOperation);

public:
    OnDisconnectAsyncOperation(
        __in IServiceConnectionHandlerPtr const & connectHandler,
        ClientConnectionStateInfoSPtr const & clientInfo,
        wstring const & serviceLocation,
        wstring const & traceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , connectHandler_(connectHandler)
        , clientInfo_(clientInfo)
        , serviceLocation_(serviceLocation)
        , traceId_(traceId)
    {
    }

    ~OnDisconnectAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<OnDisconnectAsyncOperation>(operation);

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if ((this->connectHandler_.get() == nullptr) || !clientInfo_->IsConnected())
        {
            if (clientInfo_ != nullptr)
            {
                clientInfo_->Get_SendTarget()->Reset();
                clientInfo_.reset();
            }

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        auto operation = this->connectHandler_->BeginProcessDisconnect(
            clientInfo_->Get_ClientId(),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDisconnectComplete(operation, false); },
            thisSPtr);
        this->OnDisconnectComplete(operation, true);
    }

private:

    void OnDisconnectComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->connectHandler_->EndProcessDisconnect(operation);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, this->traceId_, "{0}: Error Occured while Disconnecting Client {1} , was  connected to {2} ", error, clientInfo_->Get_ClientId(), serviceLocation_);
        }
        else
        {
            WriteNoise(TraceType, this->traceId_, "Disconnected the Client :{0} , was  connected to {1}", clientInfo_->Get_ClientId(), serviceLocation_);
        }

        if (clientInfo_ != nullptr)
        {
            clientInfo_->Get_SendTarget()->Reset();
            clientInfo_.reset();
        }

        this->TryComplete(operation->Parent, error);
    }

    IServiceConnectionHandlerPtr const & connectHandler_;
    ClientConnectionStateInfoSPtr clientInfo_;
    wstring serviceLocation_;
    wstring traceId_;
};

class ServiceCommunicationTransport::DisconnectClientsAsyncOperation
    : public ParallelVectorAsyncOperation<ClientConnectionStateInfoSPtr, ErrorCode>
{
    DENY_COPY(DisconnectClientsAsyncOperation);

public:
    DisconnectClientsAsyncOperation(
        vector<ClientConnectionStateInfoSPtr> const & clientIds,
        ServiceMethodCallDispatcherSPtr const & dispatcher,
        wstring const & traceId,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent) :
        ParallelVectorAsyncOperation<ClientConnectionStateInfoSPtr, ErrorCode>(clientIds, callback, parent)
        , dispatcher_(dispatcher)
        , traceId_(traceId)
    {
    }

    ~DisconnectClientsAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation, vector<ErrorCode> & errors)
    {
        return ParallelVectorAsyncOperation::End(operation, errors);
    }

protected:
    Common::AsyncOperationSPtr OnStartOperation(ClientConnectionStateInfoSPtr const  & clientInfo, Common::AsyncCallback const & operation, Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<OnDisconnectAsyncOperation>(
            dispatcher_->ConnectionHandler,
            clientInfo,
            dispatcher_->ServiceInfo,
            this->traceId_,
            dispatcher_->get_Timeout(),
            operation,
            parent);
    }

    virtual Common::ErrorCode OnEndOperation(
        Common::AsyncOperationSPtr const & operation,
        ClientConnectionStateInfoSPtr const & /* unused */,
        __out ErrorCode & output)
    {
        output = OnDisconnectAsyncOperation::End(operation);
        return output;
    }

private:

    ServiceMethodCallDispatcherSPtr dispatcher_;
    wstring traceId_;
};

class ServiceCommunicationTransport::UnRegisterAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(UnRegisterAsyncOperation);

public:
    UnRegisterAsyncOperation(
        ServiceCommunicationTransport  & owner,
        wstring const & serviceInfo,
        wstring const & traceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , transport_(owner)
        , serviceInfo_(serviceInfo)
        , traceId_(traceId)
    {
    }

    ~UnRegisterAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<UnRegisterAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto dispatcher = transport_.messageHandlerCollection_->GetMessageHandler(serviceInfo_);
        transport_.messageHandlerCollection_->UnregisterMessageHandler(serviceInfo_);
        vector<ClientConnectionStateInfoSPtr> clientInfo;
        {
            AcquireWriteLock grab(transport_.clientTablelock_);
            clientInfo = transport_.clientTable_->RemoveClients(dispatcher->get_ServiceInfo());
        }

        WriteInfo(
            TraceType,
            this->traceId_,
            "{0} Connected  Clients Found , to disconnect from this Service {1}",
            clientInfo.size(),
            serviceInfo_);

        auto operation = AsyncOperation::CreateAndStart<DisconnectClientsAsyncOperation>(
            clientInfo,
            dispatcher,
            this->traceId_,
            [this](AsyncOperationSPtr const & operation) { this->OnUnregisterComplete(operation, false); },
            thisSPtr);

        this->OnUnregisterComplete(operation, true);
    }

    void OnUnregisterComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        WriteInfo(
            TraceType,
            this->traceId_,
            "{0} unregistered service location",
            serviceInfo_);

        this->TryComplete(operation->Parent, operation->Error);
    }
private:
    ServiceCommunicationTransport & transport_;
    wstring const & serviceInfo_;
    wstring const & traceId_;

};



AsyncOperationSPtr ServiceCommunicationTransport::BeginRequest(
    MessageUPtr && message,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    message->Headers.Add(IsAsyncOperationHeader(true));
    message->Headers.Add(ActorHeader(Actor::ServiceCommunicationActor));
    message->Headers.Add(FabricActivityHeader(ActivityId()));
    message->Headers.Add(ActionHeader(Constants::FabricServiceCommunicationAction));
    message->Headers.Replace(TimeoutHeader(timeout));

    auto sendTarget = GetSendTarget(message);

    if (!sendTarget)
    {
        WriteWarning(TraceType, this->traceId_, "Cannot Connect to Unknown Client");
        //Request Reply Operation has the logic of handling nullptr send Target
        return requestReply_.BeginRequest(move(message), sendTarget, timeout, callback, parent);
    }

    return this->requestReply_.BeginRequest(move(message), sendTarget, TransportPriority::Normal, timeout, callback, parent);
}

ErrorCode ServiceCommunicationTransport::EndRequest(
    AsyncOperationSPtr const &  operation,
    MessageUPtr & reply)
{
    auto errorcode = this->requestReply_.EndRequest(operation, reply);
    if (!errorcode.IsSuccess() && ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(errorcode))
    {
        errorcode = ErrorCodeValue::ServiceCommunicationCannotConnect;
    }
    return errorcode;
}

ErrorCode  ServiceCommunicationTransport::SendOneWay(
    MessageUPtr && message)

{
    auto sendTarget = GetSendTarget(message);

    if (!sendTarget)
    {
        return ErrorCodeValue::NotFound;
    }

    if (message->MessageId.IsEmpty())
    {
        message->Headers.Add(MessageIdHeader());
    }

    message->Headers.Add(IsAsyncOperationHeader(false));
    message->Headers.Add(ActorHeader(Actor::ServiceCommunicationActor));

    auto errorcode = transport_->SendOneWay(sendTarget, move(message));
    if (!errorcode.IsSuccess() && ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(errorcode))
    {
        errorcode = ErrorCodeValue::ServiceCommunicationCannotConnect;
    }
    return errorcode;
}

ErrorCode ServiceCommunicationTransport::Open()
{
    AcquireWriteLock grab(lock_);
    //reopening the same transport is not supported


    if (this->IsClosedCallerHoldingLock())
    {
        return ErrorCodeValue::ObjectClosed;
    }

    openListenerCount_++;
    
    if (openListenerCount_ > 1) 
    {
        WriteInfo(
            TraceType,
            this->traceId_,
            "Transport already open");

        return ErrorCodeValue::Success ;
    }
	
	this->requestReply_.Open();

    auto error = demuxer_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            this->traceId_,
            "Demuxer::Open failed : {0}",
            error);
        //cleanup
        this->CloseCallerHoldingLock();

        return error;
    }


    auto root = this->CreateComponentRoot();
    transport_->SetConnectionFaultHandler([this, root](ISendTarget const & target, ErrorCode const & )
    {
        this->OnDisconnect(target);
    });


    error = transport_->Start();
    listenAddress_ = transport_->ListenAddress();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            this->traceId_,
            "Start failed : {0}",
            error);

        //cleanup done to make sure other components are closed
        this->CloseCallerHoldingLock();

        return error;
    }

    WriteNoise(TraceType, this->traceId_, "ServiceCommunication Transport Started listening at : {0}", listenAddress_);

    demuxer_->RegisterMessageHandler(
        Actor::ServiceCommunicationActor,
        [this](MessageUPtr & m, ReceiverContextUPtr & r)
    {
        this->ProcessRequest(move(m), move(r));
    },
        true);

    return ErrorCodeValue::Success;
}

ErrorCode ServiceCommunicationTransport::Close()
{    
    AcquireWriteLock grab(lock_);
    //no-op
    if (this->IsClosedCallerHoldingLock())
    {
        return ErrorCodeValue::Success;
    }
    return this->CloseCallerHoldingLock();
}

bool ServiceCommunicationTransport::IsClosed()
{
    bool isClosed;
    {
        AcquireReadLock grab(lock_);
        isClosed = this->IsClosedCallerHoldingLock();
    }
    return isClosed;
}


ErrorCode ServiceCommunicationTransport::RegisterCommunicationListener(
    ServiceMethodCallDispatcherSPtr  const &  dispatcher,
    __out std::wstring & endpointAddress)
{
    WriteInfo(TraceType, this->traceId_, "Registering Listener {0}", dispatcher->ServiceInfo);

    auto errorcode = messageHandlerCollection_->RegisterMessageHandler(
        dispatcher);

    if (errorcode.IsSuccess())
    {
        errorcode = GenerateEndpoint(dispatcher->ServiceInfo, endpointAddress);
        WriteInfo(TraceType, this->traceId_, "Registered Listener {0} to this endpoint {1}", dispatcher->ServiceInfo, endpointAddress);

    }
    return errorcode;
}


Common::AsyncOperationSPtr ServiceCommunicationTransport::BeginUnregisterCommunicationListener(
    std::wstring const & servicePath,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const &  parent)
{
    return  AsyncOperation::CreateAndStart<UnRegisterAsyncOperation>(
        *this,
        servicePath,
        this->traceId_,
        callback,
        parent);
}

Common::ErrorCode ServiceCommunicationTransport::EndUnregisterCommunicationListener(
    Common::AsyncOperationSPtr const & asyncOperation)
{
    return UnRegisterAsyncOperation::End(asyncOperation);
}

ErrorCode ServiceCommunicationTransport::Create(ServiceCommunicationTransportSettingsUPtr  && settings,
    std::wstring const & address,
    IServiceCommunicationTransportSPtr & transportSPtr)
{

    auto securitysettings = settings->get_SecuritySetting();
    securitysettings.SetDefaultRemoteRole(RoleMask::User); //when client role is not enabled, all clients are treated as non-admin client for message size limit checking
    auto serviceCommunicationTransportSptr = shared_ptr<ServiceCommunicationTransport>(new ServiceCommunicationTransport(move(settings), address));
    auto error = serviceCommunicationTransportSptr->transport_->SetSecurity(securitysettings);
    if (error.IsSuccess())
    {
        transportSPtr = serviceCommunicationTransportSptr;
    }
    return error;
}

ServiceCommunicationTransport::ServiceCommunicationTransport(
    ServiceCommunicationTransportSettingsUPtr && settings,
    std::wstring const & address)
    : listenAddress_(address)
    , traceId_(Guid::NewGuid().ToString())
    , transport_(DatagramTransportFactory::CreateTcp(listenAddress_, listenAddress_ + L"+" + traceId_, L"ServiceCommunicationTransport"))
    , requestReply_(*this, transport_, false /* dispatchOnTransportThread */)
    , clientTable_(make_unique<ClientTable>(traceId_))
    , messageHandlerCollection_(make_unique<ServiceCommunicationMessageHandlerCollection>(*this, settings->MaxConcurrentCalls, settings->MaxQueueSize))
    , settings_(move(settings))
    , openListenerCount_(0)
    , isClosed_(false)
{
    demuxer_ = make_unique<Demuxer>(*this, transport_);
    demuxer_->SetReplyHandler(this->requestReply_);
    transport_->SetMaxIncomingFrameSize(settings_->get_MaxMessageSize());
    transport_->SetMaxOutgoingFrameSize(settings_->get_MaxMessageSize());
    transport_->EnableInboundActivityTracing();
    transport_->SetKeepAliveTimeout(settings_->KeepAliveTimeout);
    defaultTimeout_ = settings_->OperationTimeout;
    transport_->SetConnectionIdleTimeout(TimeSpan::Zero);
    requestReply_.EnableDifferentTimeoutError();
}

ServiceCommunicationTransportSettings const& ServiceCommunicationTransport::GetSettings()
{
    return *settings_;
}

ServiceCommunicationTransport::~ServiceCommunicationTransport()
{
    WriteInfo(TraceType, this->traceId_, "destructing ServiceCommunicationTransport {0} ", this->listenAddress_);
}

ErrorCode ServiceCommunicationTransport::GenerateEndpoint(std::wstring const & location, __out std::wstring & endpointAddress)
{
    StringWriter(endpointAddress).Write(this->listenAddress_);
    StringWriter(endpointAddress).Write(Constants::ListenAddressPathDelimiter);
    StringWriter(endpointAddress).Write(location);
    return ErrorCodeValue::Success;
}

ISendTarget::SPtr ServiceCommunicationTransport::GetSendTarget(MessageUPtr const & message)
{
    ISendTarget::SPtr sendTarget;
    TcpClientIdHeader clientIdHeader;

    if (!message->Headers.TryReadFirst(clientIdHeader))
    {
        return sendTarget;
    }

    {
        AcquireReadLock grab(clientTablelock_);
        sendTarget = clientTable_->GetSendTarget(clientIdHeader.ClientId);
    }
    return sendTarget;
}

void ServiceCommunicationTransport::Cleanup()
{
    this->transport_->Stop();
    this->requestReply_.Close();
    this->demuxer_->Close();
    WriteInfo(TraceType, this->traceId_, "ServiceCommunication Transport Stopped Listening at :{0}", listenAddress_);
}

void  ServiceCommunicationTransport::OnDisconnect(ISendTarget const & target)
{

    ClientConnectionStateInfoSPtr connectionStateInfo;
    {
        AcquireWriteLock grab(clientTablelock_);
        connectionStateInfo = clientTable_->GetClientConnectionStateInfoUsingSendTarget(target.Id());
        if (connectionStateInfo != nullptr)
        {
            WriteInfo(TraceType, this->traceId_, "Disconnecting Client {0}  connected to Service {1}", connectionStateInfo->Get_ClientId(), connectionStateInfo->Get_ServiceConnectedTo());
            clientTable_->Remove(connectionStateInfo->Get_ClientId());
        }
    }

    if (connectionStateInfo != nullptr)
    {
        auto dispatcher = messageHandlerCollection_->GetMessageHandler(connectionStateInfo->Get_ServiceConnectedTo());
        CallDisconnectHandlerIfExists(dispatcher, connectionStateInfo);
    }

}

void ServiceCommunicationTransport::CallDisconnectHandlerIfExists(
    ServiceMethodCallDispatcherSPtr const & dispatcher,
    ClientConnectionStateInfoSPtr  & connectionStateInfo)
{
    if ((dispatcher != nullptr))
    {
        auto operation = AsyncOperation::CreateAndStart<OnDisconnectAsyncOperation>(
            dispatcher->ConnectionHandler,
            connectionStateInfo,
            dispatcher->ServiceInfo,
            this->traceId_,
            defaultTimeout_,
            [this](AsyncOperationSPtr const & operation) { return this->OnDisconnectComplete(operation, false); },
            this->CreateAsyncOperationRoot());


        this->OnDisconnectComplete(operation, true);
    }
}

void  ServiceCommunicationTransport::OnDisconnectComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = OnDisconnectAsyncOperation::End(operation);
}

wstring ServiceCommunicationTransport::GetServiceLocation(wstring const & location)
{
    auto index = location.find(*Constants::ConnectActorDelimiter);
    if (index != wstring::npos)
    {
        return location.substr(0, index);
    }
	else
	{
		return location;
	}
}

void ServiceCommunicationTransport::ProcessRequest(MessageUPtr && message, ReceiverContextUPtr && context)
{

    ServiceLocationActorHeader actorHeader;
    if (!message->Headers.TryReadFirst(actorHeader))
    {
        // Not Valid Service Address
        WriteWarning(TraceType, this->traceId_, "Message  has invalid/Empty Service Address {0} ", message->TraceId());
        auto msgReply = ServiceCommunicationHelper::CreateReplyMessageWithError(ErrorCodeValue::InvalidAddress);
        context->Reply(move(msgReply));
        return;
    }

    auto action = message->Action;
    wstring serviceLocation;

    if (action == Constants::ConnectAction)
    {
        //Parse Service Location
        serviceLocation = this->GetServiceLocation(actorHeader.Actor);
    }
    else
    {
        serviceLocation = actorHeader.Actor;
    }

    //Find Dispatcher

    auto  dispatcher = messageHandlerCollection_->GetMessageHandler(serviceLocation);
    if (dispatcher == nullptr)
    {
        // Endpoint Not Found
        WriteWarning(TraceType, this->traceId_, "Service not Found for this Actor : {0}", serviceLocation);
        auto msgReply = ServiceCommunicationHelper::CreateReplyMessageWithError(ErrorCodeValue::EndpointNotFound);
        context->Reply(move(msgReply));
        return;
    }

    //Parse ClientId
    TcpClientIdHeader clientIdHeader;
    wstring clientId;
    bool useExplicitConnect;
    if (message->Headers.TryReadFirst(clientIdHeader))
    {
        clientId = clientIdHeader.ClientId;
        useExplicitConnect = true;
    }
    else
    { //For old clients header won't be there
        clientId = context->ReplyTarget->Id();
        useExplicitConnect = false;
    }

    //Check Action
    if (action == Constants::ConnectAction)
    {

        ClientConnectionStateInfoSPtr connectionStateInfo;
        {
            bool newClient;
            //Add Entry to ClientTable
            {
                AcquireWriteLock grab(clientTablelock_);
                newClient = this->clientTable_->UpdateIfNeeded(clientId, context->ReplyTarget, move(context), dispatcher);
            }
            //Call ConnectHandler If its new Client
            CallConnectHandlerIfItsNewClient(clientId, dispatcher, newClient);

        }
        return;
    }

    //For any other Action in Request
    ClientConnectionStateInfoSPtr connectionStateInfo;
    {
        AcquireReadLock grab(clientTablelock_);
        connectionStateInfo = clientTable_->GetClientConnectionStateInfo(clientIdHeader.ClientId);
    }

    //Update If ClientInfo Not Exists And Connect. It will be true for old clients where explicit connect change has not been there.
    if (connectionStateInfo == nullptr)
    {
        if (!useExplicitConnect)
        {
            bool newClient;
            //Add Entry to ClientTable
            {
                AcquireWriteLock grab(clientTablelock_);
                auto emptyContext = unique_ptr<ReceiverContext>();
                newClient = this->clientTable_->UpdateIfNeeded(clientId, context->ReplyTarget, move(emptyContext), dispatcher);
            }
            //Call ConnectHandler If its new Client
            connectionStateInfo = CallConnectHandlerIfItsNewClient(clientId, dispatcher, newClient);
        }
        else
        {
            //if new clients try to send Request without sending Connect Request.
            WriteWarning(TraceType, this->traceId_, "Client has not send a Connect Request : {0}", serviceLocation);
            auto msgReply = ServiceCommunicationHelper::CreateReplyMessageWithError(ErrorCodeValue::InvalidMessage);
            context->Reply(move(msgReply));
            return;
        }
    }

    //Check If Client Connected or in Connecting State
    if (!connectionStateInfo->IsConnected())
    {
        //Add the Items in Temp Queue till service gets connecmated to Client
        bool result = connectionStateInfo->AddToQueueIfNotConnected(message, context);
        //For cases where AddToQueueIfNotConnected and SetConnected called same time
        if (!result)
        {
            dispatcher->AddToQueue(message, context);
        }
    }
    else
    {
        dispatcher->AddToQueue(message, context);
    }
}

ClientConnectionStateInfoSPtr ServiceCommunicationTransport::CallConnectHandlerIfItsNewClient(
    wstring const & clientId,
    ServiceMethodCallDispatcherSPtr const & dispatcher,
    bool newClient)
{
    ClientConnectionStateInfoSPtr connectionStateInfo;
    {
        AcquireReadLock grab(clientTablelock_);
        connectionStateInfo = this->clientTable_->GetClientConnectionStateInfo(clientId);
    }

    //Check If its existing Client
    if (!newClient)
    {
        return connectionStateInfo;
    }


    //Check if Service has passed valid ConnectionHandler
    if (dispatcher->ConnectionHandler.get() == nullptr)
    {
        connectionStateInfo->SetConnected();
        connectionStateInfo->SentConnectReply(ServiceCommunicationHelper::CreateReplyMessageWithError(ErrorCodeValue::Success));
    }
    else
    {
        auto root = this->CreateComponentRoot();

        Threadpool::Post([this, root, dispatcher, clientId]
        {
            this->OnConnect(clientId, dispatcher);
        });
    }
    return connectionStateInfo;
}

void ServiceCommunicationTransport::OnConnect(wstring clientId, ServiceMethodCallDispatcherSPtr dispatcher)
{
    auto connectionHandler = dispatcher->ConnectionHandler;

    //Create Client Connection 
    auto clientConnectionSPtr = make_shared<ClientConnection>(clientId, dynamic_pointer_cast<ServiceCommunicationTransport>(this->shared_from_this()));
    auto clientConnectionPtr = RootedObjectPointer<IClientConnection>(clientConnectionSPtr.get(), clientConnectionSPtr->CreateComponentRoot());

    auto operation = AsyncOperation::CreateAndStart<OnConnectAsyncOperation>(
        dispatcher,
        clientConnectionPtr,
        defaultTimeout_,
        [this](AsyncOperationSPtr const & operation)
    {
        return this->OnConnectComplete(operation, false);
    },
        this->CreateAsyncOperationRoot());

    this->OnConnectComplete(operation, true);
}

void  ServiceCommunicationTransport::OnConnectComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    wstring clientId;
    auto error = OnConnectAsyncOperation::End(operation, clientId);
    ClientConnectionStateInfoSPtr connectionStateInfo;
    {
        AcquireReadLock grab(clientTablelock_);
        connectionStateInfo = this->clientTable_->GetClientConnectionStateInfo(clientId);
    }

    //Cases where during disconnect state-info has been removed.
    if (!connectionStateInfo)
    {
        WriteInfo(TraceType, this->traceId_, "Client : {0} is already disconnected to service", clientId);
        return;
    }

    connectionStateInfo->SentConnectReply(ServiceCommunicationHelper::CreateReplyMessageWithError(operation->Error));

    if (operation->Error.IsSuccess())
    {
        //Move the items from client's temporary queue to service queue
        connectionStateInfo->SetConnected();
    }
    else
    {
        //When Connection Fails
        auto &sendTarget = *connectionStateInfo->Get_SendTarget().get();
        WriteWarning(TraceType, this->traceId_, "{0}: Error Occured while Connecting Client :{1}  to Service: {2}", operation->Error, clientId, connectionStateInfo->Get_ServiceConnectedTo());
        this->OnDisconnect(sendTarget);
    }
}

ErrorCode ServiceCommunicationTransport::CloseCallerHoldingLock()
{

    if (openListenerCount_ > 0)
    {
        openListenerCount_--;
    }

    //Last Listner to close;
    if (openListenerCount_ == 0)
    {
        this->Cleanup();
        isClosed_ = true;
    }

    return ErrorCodeValue::Success;
}

bool ServiceCommunicationTransport::IsClosedCallerHoldingLock() 
{
    return isClosed_;
}
