// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Naming;
using namespace Federation;

StringLiteral const TraceComponent("ServiceNotificationManagerProxy");

class ServiceNotificationManagerProxy::NotificationAsyncOperationBase
    : public TimedAsyncOperation
{
public:
    NotificationAsyncOperationBase(
        ServiceNotificationManagerProxy &notificationManager,
        MessageUPtr && message,
        ISendTarget::SPtr const &target,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(notificationManager)
        , request_(move(message))
        , target_(target)
    {
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        activityId_ = FabricActivityHeader::FromMessage(*request_);
        OnStartRequest(thisSPtr);
    }

    virtual void OnStartRequest(AsyncOperationSPtr const &thisSPtr) = 0;

    static ErrorCode End(AsyncOperationSPtr const& operation, __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<NotificationAsyncOperationBase>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }


protected:
    ServiceNotificationManagerProxy &owner_;
    MessageUPtr request_;
    ISendTarget::SPtr target_;
    MessageUPtr reply_;
    FabricActivityHeader activityId_;
    ClientIdentityHeader clientId_;
};

class ServiceNotificationManagerProxy::NotificationClientConnectAsyncOperation
    : public NotificationAsyncOperationBase
{
public:
    NotificationClientConnectAsyncOperation(
        ServiceNotificationManagerProxy &owner,
        MessageUPtr && message,
        ISendTarget::SPtr const &target,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : NotificationAsyncOperationBase(owner, move(message), target, timeout, callback, parent)
    {
    }

    void OnStartRequest(AsyncOperationSPtr const &thisSPtr)
    {
        NotificationClientConnectionRequestBody body;
        if (!request_->GetBody(body))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(request_->GetStatus()));
        }   
        
        //
        // When a client retries the connect, we can just use the existing clientId and not generate a new one.
        //
        if (!owner_.TryGetClientRegistration(target_, body.GetClientId(), clientId_))
        {
            clientId_ = ClientIdentityHeader(target_, body.GetClientId());
            if (!owner_.TryAddOrUpdateClientRegistration(target_, ClientIdentityHeader(clientId_)))
            {
                TRACE_AND_TESTASSERT(
                    WriteError,
                    TraceComponent,
                    owner_.TraceId,
                    "Failed to update client registration for target:{0}, regn:{1}",
                    TracePtr(target_.get()),
                    clientId_);
                    
                TryComplete(thisSPtr, ErrorCodeValue::InvalidAddress);
            }
        }

        WriteInfo(TraceComponent, owner_.TraceId, "{0} Notification client connection {1}", activityId_, clientId_);
        request_->Headers.Replace(clientId_);

        auto op = owner_.proxyToEntreeServiceTransport_.BeginSendToEntreeService(
            move(request_),
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation, false);
            },
            thisSPtr);

        OnRequestComplete(op, true);
    }

private:
    void OnRequestComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyToEntreeServiceTransport_.EndSendToEntreeService(operation, reply_);
        if (!error.IsSuccess())
        {
            ClientIdentityHeader tempClientId;
            owner_.TryRemoveClientRegistration(*target_, tempClientId);
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
};

class ServiceNotificationManagerProxy::NotificationClientSyncAsyncOperation
    : public NotificationAsyncOperationBase
{
public:
    NotificationClientSyncAsyncOperation(
        ServiceNotificationManagerProxy &owner,
        MessageUPtr && message,
        ISendTarget::SPtr const &target,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : NotificationAsyncOperationBase(owner, move(message), target, timeout, callback, parent)
    {
    }

    void OnStartRequest(AsyncOperationSPtr const &thisSPtr)
    {
        NotificationClientSynchronizationRequestBody body;
        if (!request_->GetBody(body))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(request_->GetStatus()));
        }   
        
        if (owner_.TryGetClientRegistration(target_, body.GetClientId(), clientId_))
        {
            request_->Headers.Replace(clientId_);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                owner_.TraceId,
                "{0}: Target:{1} not connected before sending the synchronization request",
                activityId_,
                TracePtr(target_.get()));

            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }

        auto op = owner_.proxyToEntreeServiceTransport_.BeginSendToEntreeService(
            move(request_),
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation, false);
            },
            thisSPtr);

        OnRequestComplete(op, true);
    }

private:

    void OnRequestComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyToEntreeServiceTransport_.EndSendToEntreeService(operation, reply_);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
};

class ServiceNotificationManagerProxy::NotificationFilterRegistrationAsyncOperation
    : public NotificationAsyncOperationBase
{
public:
    NotificationFilterRegistrationAsyncOperation(
        ServiceNotificationManagerProxy &owner,
        MessageUPtr && message,
        ISendTarget::SPtr const &target,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : NotificationAsyncOperationBase(owner, move(message), target, timeout, callback, parent)
    {
    }

    void OnStartRequest(AsyncOperationSPtr const &thisSPtr)
    {
        RegisterServiceNotificationFilterRequestBody body;
        if (!request_->GetBody(body))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(request_->GetStatus()));
        }   
        
        if (owner_.TryGetClientRegistration(target_, body.TakeClientId(), clientId_))
        {
            request_->Headers.Replace(clientId_);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                owner_.TraceId,
                "{0}: Target:{1} not connected before registering filters",
                activityId_,
                TracePtr(target_.get()));

            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }

        auto op = owner_.proxyToEntreeServiceTransport_.BeginSendToEntreeService(
            move(request_),
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation, false);
            },
            thisSPtr);

        OnRequestComplete(op, true);
    }

private:

    void OnRequestComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyToEntreeServiceTransport_.EndSendToEntreeService(operation, reply_);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
};

class ServiceNotificationManagerProxy::NotificationFilterUnregistrationAsyncOperation
    : public NotificationAsyncOperationBase
{
public:
    NotificationFilterUnregistrationAsyncOperation(
        ServiceNotificationManagerProxy &owner,
        MessageUPtr && message,
        ISendTarget::SPtr const &target,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : NotificationAsyncOperationBase(owner, move(message), target, timeout, callback, parent)
    {
    }

    void OnStartRequest(AsyncOperationSPtr const &thisSPtr)
    {
        UnregisterServiceNotificationFilterRequestBody body;
        if (!request_->GetBody(body))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(request_->GetStatus()));
            return;
        }
        
        if (owner_.TryGetClientRegistration(target_, body.TakeClientId(), clientId_))
        {
            request_->Headers.Replace(clientId_);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                owner_.TraceId,
                "{0}: Target:{1} not connected before registering filters",
                activityId_,
                TracePtr(target_.get()));

            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        auto op = owner_.proxyToEntreeServiceTransport_.BeginSendToEntreeService(
            move(request_),
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation, false);
            },
            thisSPtr);

        OnRequestComplete(op, true);
    }

private:

    void OnRequestComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.proxyToEntreeServiceTransport_.EndSendToEntreeService(operation, reply_);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
};

class ServiceNotificationManagerProxy::ProcessNotificationAsyncOperation
    : public TimedAsyncOperation
{
public:
    ProcessNotificationAsyncOperation(
        ServiceNotificationManagerProxy &owner,
        MessageUPtr && message,
        TimeSpan const &timeout,
        AsyncCallback const &callback, 
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , request_(move(message))
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessNotificationAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        if (!request_->Headers.TryGetAndRemoveHeader(clientRegn_))
        {
            TRACE_AND_TESTASSERT(
                WriteWarning,
                TraceComponent,
                owner_.TraceId,
                "Client registration not sent from ServiceNotificationSender");

            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        ISendTarget::SPtr sendTarget;
        if (!owner_.TryGetSendTarget(clientRegn_, sendTarget))
        {
            //
            // This could happen when the ServiceNotificationManager in EntreeService tries to send a notification to 
            // a client that has disconnected but that disconnection message sent from ServiceNotificationManagerProxy 
            // hasnt reached the ServiceNotificationManager yet.
            //
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "Send target not found for client registration : {0}",
                clientRegn_);

            TryComplete(thisSPtr, ErrorCodeValue::CannotConnectToAnonymousTarget);
            return;
        }

        if (request_->Action == NamingMessage::DisconnectClientAction)
        {
            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "Processing cleanup for client registration : {0}",
                clientRegn_);

            ClientIdentityHeader temp;
            if (!owner_.TryRemoveClientRegistration(*sendTarget, temp))
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "Failed to cleanup client registration : {0}",
                    clientRegn_);
            }

            reply_ = move(NamingMessage::GetOperationSuccess());
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
        else
        {
            ForwardMessageHeader fwdHeader;
            if (request_->Headers.TryGetAndRemoveHeader(fwdHeader))
            {
                request_->Headers.Replace(ActorHeader(fwdHeader.Actor));
                request_->Headers.Replace(ActionHeader(fwdHeader.Action));
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    owner_.TraceId,
                    "Forward header not found");
            }

            MessageId forwardedMessageId;

            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "Forwarding notification page for {0}: old={1} new={2}",
                clientRegn_,
                request_->MessageId,
                forwardedMessageId);

            // The retry message from the gateway may arrive here
            // before the previous message instance has timed out on
            // the proxy. Replace the message ID when forwarding since
            // the previous message should be timing out on the proxy 
            // as well very soon.
            //
            request_->Headers.Replace(MessageIdHeader(forwardedMessageId));

            auto op = owner_.requestReply_->BeginNotification(
                move(request_),
                sendTarget,
                RemainingTime,
                [this](AsyncOperationSPtr const &operation)
                {
                    this->OnNotificationComplete(operation, false);
                },
                thisSPtr);

            OnNotificationComplete(op, true);
        }
    }
     
private:

    void OnNotificationComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.requestReply_->EndNotification(operation, reply);

        // This would be sent over IPC, so clone the reply received from client
        if (error.IsSuccess()) 
        { 
            if (reply.get() == nullptr)
            {
                WriteInfo(
                    TraceComponent,
                    owner_.TraceId,
                    "null notification reply message");

                error = ErrorCodeValue::OperationFailed;
            }
            else
            {
                reply_ = reply->Clone(); 
            }
        }

        TryComplete(operation->Parent, error);
    }

    ServiceNotificationManagerProxy &owner_;
    MessageUPtr request_;
    MessageUPtr reply_;
    FabricActivityHeader activityId_;
    ClientIdentityHeader clientRegn_;
};

ServiceNotificationManagerProxy::ServiceNotificationManagerProxy(
    ComponentRoot const &root,
    Federation::NodeInstance const &instance,
    RequestReplySPtr const & requestReply,
    ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport,
    IDatagramTransportSPtr const &publicTransport)
    : RootedObject(root)
    , NodeTraceComponent(instance)
    , lock_()
    , requestReply_(requestReply)
    , proxyToEntreeServiceTransport_(proxyToEntreeServiceTransport)
    , publicTransport_(publicTransport)
{
    REGISTER_MESSAGE_HEADER(ClientIdentityHeader);
}

ErrorCode ServiceNotificationManagerProxy::OnOpen()
{
    auto root = this->Root.CreateComponentRoot();

    publicTransport_->SetConnectionAcceptedHandler([this, root](ISendTarget const& st)
        {
            this->OnClientConnectionAccepted(st);
        });

    publicTransport_->SetConnectionFaultHandler([this, root](ISendTarget const& st, ErrorCode err)
        {
            this->OnClientConnectionFault(st, err);
        });

    proxyToEntreeServiceTransport_.RegisterNotificationsHandler(
        Actor::EntreeServiceProxy,
        [this, root](MessageUPtr & message, TimeSpan const timespan, AsyncCallback const &callback, AsyncOperationSPtr const &parent) -> AsyncOperationSPtr
        {
            return this->BeginProcessNotification(message, timespan, callback, parent);
        },
        [this, root](AsyncOperationSPtr const&operation, __out MessageUPtr &reply) -> ErrorCode
        {
            return this->EndProcessNotification(operation, reply);
        });

    return ErrorCodeValue::Success;
}

ErrorCode ServiceNotificationManagerProxy::OnClose()
{
    this->OnAbort();

    return ErrorCodeValue::Success;
}

void ServiceNotificationManagerProxy::OnAbort()
{
    publicTransport_->RemoveConnectionFaultHandler();
    publicTransport_->RemoveConnectionAcceptedHandler();
    proxyToEntreeServiceTransport_.UnregisterNotificationsHandler(Actor::EntreeServiceProxy);
}

void ServiceNotificationManagerProxy::SetConnectionAcceptedHandler(
    IDatagramTransport::ConnectionAcceptedHandler const & handler)
{
    AcquireWriteLock lock(lock_);

    connectionAcceptedHandler_ = handler;
}

void ServiceNotificationManagerProxy::SetConnectionFaultHandler(
    IDatagramTransport::ConnectionFaultHandler const & handler)
{
    AcquireWriteLock lock(lock_);

    connectionFaultHandler_ = handler;
}

AsyncOperationSPtr ServiceNotificationManagerProxy::BeginProcessRequest(
    MessageUPtr && message,
    ISendTarget::SPtr const &target,
    TimeSpan const &timeout,
    AsyncCallback const &callback, 
    AsyncOperationSPtr const & parent)
{
    ErrorCode error;
    auto activityHeader = FabricActivityHeader::FromMessage(*message);

    if (message->Action == NamingTcpMessage::NotificationClientConnectionRequestAction)
    {
        return AsyncOperation::CreateAndStart<NotificationClientConnectAsyncOperation>(
            *this,
            move(message),
            target,
            timeout,
            callback,
            parent);
    }
    else if (message->Action == NamingTcpMessage::NotificationClientSynchronizationRequestAction)
    {
        return AsyncOperation::CreateAndStart<NotificationClientSyncAsyncOperation>(
            *this,
            move(message),
            target,
            timeout,
            callback,
            parent);
    }
    else if (message->Action == NamingTcpMessage::RegisterServiceNotificationFilterRequestAction)
    {
        return AsyncOperation::CreateAndStart<NotificationFilterRegistrationAsyncOperation>(
            *this,
            move(message),
            target,
            timeout,
            callback,
            parent);
    }
    else if (message->Action == NamingTcpMessage::UnregisterServiceNotificationFilterRequestAction)
    {
        return AsyncOperation::CreateAndStart<NotificationFilterUnregistrationAsyncOperation>(
            *this,
            move(message),
            target,
            timeout,
            callback,
            parent);
    }
    else
    {
        TRACE_AND_TESTASSERT(
            WriteWarning,
            TraceComponent, 
            this->TraceId,
            "{0}: Unknown action {1}",
            activityHeader,
            message->Action);

        error = ErrorCodeValue::InvalidMessage;       
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }
}

ErrorCode ServiceNotificationManagerProxy::EndProcessRequest(
    AsyncOperationSPtr const &operation,
    __out MessageUPtr &reply)
{
    if (operation->FailedSynchronously)
    {
        return AsyncOperation::End(operation);
    }

    return NotificationAsyncOperationBase::End(operation, reply);
}

void ServiceNotificationManagerProxy::OnClientConnectionAccepted(ISendTarget const & sendTarget)
{
    IDatagramTransport::ConnectionAcceptedHandler handler = nullptr;
    {
        AcquireReadLock lock(lock_);

        handler = connectionAcceptedHandler_;
    }

    if (handler)
    {
        handler(sendTarget);
    }
}

void ServiceNotificationManagerProxy::OnClientConnectionFault(ISendTarget const & sendTarget, ErrorCode const & error)
{
    // 
    // We recommend using the gateway's client connection port for customer Azure LB probes. This
    // means that every 15 seconds, we will get a new TCP connection on the client connection port.
    // When the old probed connection disconnects, it happens with the following error.
    // Special-case this error to avoid noisy traces from potential LB probes.
    //
    // (ERROR_NETNAME_DELETED) 0x80070040
    // 
    if (error.IsError(ErrorCode::FromWin32Error(ERROR_NETNAME_DELETED).ReadValue()))
    {
        WriteNoise(
            TraceComponent, 
            this->TraceId, 
            "connection fault: target={0} error={1}", 
            sendTarget.Id(), 
            error);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId, 
            "connection fault: target={0} error={1}", 
            sendTarget.Id(),
            error);
    }

    IDatagramTransport::ConnectionFaultHandler handler = nullptr;
    {
        AcquireReadLock lock(lock_);

        handler = connectionFaultHandler_;
    }

    if (handler)
    {
        handler(sendTarget, error);
    }

    ClientIdentityHeader clientRegn;
    if (TryRemoveClientRegistration(sendTarget, clientRegn))
    {
        NotifyClientDisconnection(clientRegn);
    }
}

void ServiceNotificationManagerProxy::NotifyClientDisconnection(ClientIdentityHeader &clientRegn)
{
    auto message = NamingMessage::GetDisconnectClient();
    message->Headers.Replace(clientRegn);
    message->Headers.Replace(TimeoutHeader(ServiceModelConfig::GetConfig().ServiceNotificationTimeout));

    WriteInfo(
        TraceComponent,
        TraceId,
        "Sending client disconnection notification for {0}",
        clientRegn.TargetId);

    auto op = proxyToEntreeServiceTransport_.BeginSendToEntreeService(
        move(message),
        ServiceModelConfig::GetConfig().ServiceNotificationTimeout,
        [this](AsyncOperationSPtr const &operation)
        {
            OnNotifyClientDisconnectionComplete(operation);
        },
        Root.CreateAsyncOperationRoot());
}

void ServiceNotificationManagerProxy::OnNotifyClientDisconnectionComplete(AsyncOperationSPtr const &op)
{
    MessageUPtr reply;
    auto error = proxyToEntreeServiceTransport_.EndSendToEntreeService(op, reply);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Client disconnection notification to Entree service failed with {0}",
            error);
    }
}

AsyncOperationSPtr ServiceNotificationManagerProxy::BeginProcessNotification(
    MessageUPtr & message,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ProcessNotificationAsyncOperation>(
        *this,
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode ServiceNotificationManagerProxy::EndProcessNotification(
    AsyncOperationSPtr const &operation,
    __out MessageUPtr &reply)
{
    return ProcessNotificationAsyncOperation::End(operation, reply);
}

bool ServiceNotificationManagerProxy::TryAddOrUpdateClientRegistration(
    ISendTarget::SPtr const &target,
    ClientIdentityHeader &&clientRegn)
{
    AcquireWriteLock writeLock(lock_);
    auto result = sendTargetToClientRegistrationMap_.insert(make_pair(target->Id(), clientRegn));
    
    // Update the reverse map if needed
    if (result.second)
    {
        clientRegistrationToSendTargetMap_.insert(make_pair(move(clientRegn), target));
    }

    if (!target->TryEnableDuplicateDetection())
    {
        TRACE_AND_TESTASSERT(
            WriteWarning,
            TraceComponent,
            this->TraceId,
            "could not enable duplicate detection on target");
    }

    return result.second;
}

bool ServiceNotificationManagerProxy::TryGetClientRegistration(
    Transport::ISendTarget::SPtr const &target, 
    wstring const &clientId,
    __out ClientIdentityHeader &clientRegn)
{
    AcquireReadLock readLock(lock_);
    auto it = sendTargetToClientRegistrationMap_.find(target->Id());
    if (it != sendTargetToClientRegistrationMap_.end())
    {
        if (clientId.empty() || it->second.FriendlyName == clientId)
        {
            clientRegn = it->second;
            return true;
        }
    }

    return false;
}

bool ServiceNotificationManagerProxy::TryGetSendTarget(
    ClientIdentityHeader const &clientRegn,
    __out ISendTarget::SPtr &target)
{
    AcquireReadLock readLock(lock_);
    auto it = clientRegistrationToSendTargetMap_.find(clientRegn);
    if (it != clientRegistrationToSendTargetMap_.end())
    {
        target = it->second;
        return true;
    }

    return false;
}

bool ServiceNotificationManagerProxy::TryRemoveClientRegistration(
    ISendTarget const &target,
    __out ClientIdentityHeader &clientRegn)
{
    AcquireWriteLock writeLock(lock_);
    auto it = sendTargetToClientRegistrationMap_.find(target.Id());
    if (it != sendTargetToClientRegistrationMap_.end())
    {
        clientRegn = it->second;
        sendTargetToClientRegistrationMap_.erase(it);
        auto it2 = clientRegistrationToSendTargetMap_.find(clientRegn);
        if (it2 != clientRegistrationToSendTargetMap_.end())
        {
            // 
            // Ensure closing the connection from gateway in
            // cases where we have to deal with deadlocked
            // clients.
            //
            it2->second->Reset();

            clientRegistrationToSendTargetMap_.erase(it2);
        }

        return true;
    }

    return false;
}

size_t ServiceNotificationManagerProxy::GetTotalPendingTransportMessageCount(wstring const & clientId) const
{
    AcquireReadLock readLock(lock_);
    size_t total = 0;
    for (auto const & it : clientRegistrationToSendTargetMap_)
    {
        if(it.first.FriendlyName == clientId)
        {
            total += it.second->MessagesPendingForSend();
        }
    }

    return total;
}
