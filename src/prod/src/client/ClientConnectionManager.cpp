// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AzureActiveDirectory/wrapper.client/ClientWrapper.h"

#ifdef PLATFORM_UNIX
#include "Common/CryptoUtility.Linux.h"
#include "Transport/TransportSecurity.Linux.h"
#endif

#include "Transport/ListenInstance.h"
#include "Transport/SecurityNegotiationHeader.h"
#include "Transport/SecurityContext.h"
#include "Transport/SecurityContextSsl.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Naming;

using namespace Client;
using namespace ClientServerTransport;

const StringLiteral TraceComponent("ClientConnectionManager");

//
// RequestReplyAsyncOperationBase
//

class ClientConnectionManager::RequestReplyAsyncOperationBase
    : public AsyncOperation
    , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
{
public:
    RequestReplyAsyncOperationBase(
        __in ClientConnectionManager & owner,
        ClientServerRequestMessageUPtr request,
        TransportPriority::Enum priority,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(move(request))
        , requestAction_(request_->Action)
        , priority_(priority)
        , timeoutHelper_(timeout)
        , activityId_(request_->ActivityId)
    {
        request_->Headers.Add(*ClientProtocolVersionHeader::CurrentVersionHeader);
    }

    ActivityId const & GetActivityId() { return activityId_; }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<RequestReplyAsyncOperationBase>(operation)->Error;
    }

protected:

    virtual void OnSendRequestComplete(
        AsyncOperationSPtr const & thisSPtr, 
        __in ErrorCode &,
        ClientServerReplyMessageUPtr && reply) = 0;

protected:

    ClientConnectionManager & GetOwner() { return owner_; }

    wstring const & GetTraceId() { return owner_.TraceId; }

    wstring const & GetRequestAction() { return requestAction_; }

    TimeSpan const GetRemainingTime() { return timeoutHelper_.GetRemainingTime(); }

    void SendRequest(AsyncOperationSPtr const & thisSPtr, ISendTarget::SPtr const & target)
    {
        ErrorCode nonRetryableError;
        if (owner_.NonRetryableErrorEncountered(nonRetryableError))
        {
            WriteInfo(
                TraceComponent,
                this->GetTraceId(),
                "{0}: completing '{1}' with non-retryable error {2}",
                this->GetActivityId(),
                request_->Action,
                nonRetryableError);

            TryComplete(thisSPtr, nonRetryableError);
            return;
        }

        auto timeout = timeoutHelper_.GetRemainingTime();

        request_->Headers.Add(TimeoutHeader(timeout));

        WriteNoise(
            TraceComponent,
            this->GetTraceId(),
            "{0}: sending '{1}' to gateway {2}: priority={3}",
            this->GetActivityId(),
            requestAction_,
            target->Address(),
            priority_);
        
        auto operation = owner_.transport_->BeginRequestReply(
            move(request_),
            target,
            priority_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnRequestComplete(operation, false); },
            thisSPtr);
        this->OnRequestComplete(operation, true);
    }

private:

    void OnRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ClientServerReplyMessageUPtr reply;
        auto error = owner_.transport_->EndRequestReply(operation, reply);

        if (!error.IsSuccess())
        {
            owner_.RecordNonRetryableError(error);
            WriteInfo(
                TraceComponent,
                this->GetTraceId(),
                "{0}: '{1}' request failed: {2}:{3}",
                this->GetActivityId(),
                requestAction_,
                error,
                error.Message);
        }
        
        this->OnSendRequestComplete(thisSPtr, error, move(reply));
    }

protected:

    ErrorCode ProcessReply(__in ClientServerReplyMessageUPtr & reply)
    {
        if (!reply)
        {
            auto msg = wformatString(
                "{0}: gateway sent null reply",
                this->GetActivityId());

            WriteError(
                TraceComponent,
                this->GetTraceId(),
                "{0}",
                msg);

            Assert::TestAssert("{0}", msg);

            return ErrorCodeValue::OperationFailed;
        }
        else if (reply->Action == NamingTcpMessage::ClientOperationFailureAction)
        {
            ClientOperationFailureMessageBody body;
            if (reply->GetBody(body))
            {
                auto error = body.TakeError();

                WriteInfo(
                    TraceComponent,
                    this->GetTraceId(),
                    "{0}: gateway replied with error={1}",
                    this->GetActivityId(),
                    error);

                return error;
            }
            else
            {
                return ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else
        {
            WriteNoise(
                TraceComponent,
                this->GetTraceId(),
                "{0}: gateway replied with success",
                this->GetActivityId());

            return ErrorCodeValue::Success;
        }
    } 

private:

    ClientConnectionManager & owner_;
    ClientServerRequestMessageUPtr request_;
    std::wstring const requestAction_;
    TransportPriority::Enum priority_;
    TimeoutHelper timeoutHelper_;
    ActivityId activityId_;
};

//
// SendToGatewayAsyncOperation
//

class ClientConnectionManager::SendToGatewayAsyncOperation : public RequestReplyAsyncOperationBase
{
public:
    SendToGatewayAsyncOperation(
        __in ClientConnectionManager & owner,
        ClientServerRequestMessageUPtr && request,
        TransportPriority::Enum priority,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestReplyAsyncOperationBase(owner, move(request), priority, timeout, callback, parent)
        , reply_()
        , retryTimer_()
        , gatewayTimer_()
        , timerLock_()
    {
    }

    uint64 GetWaiterId() const
    {
        return reinterpret_cast<uint64>(static_cast<void const*>(this));
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ClientServerReplyMessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<SendToGatewayAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->TrySendToCurrentGateway(thisSPtr);
    }

    void OnGatewayConnected(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireWriteLock lock(timerLock_);

            if (gatewayTimer_)
            {
                gatewayTimer_->Cancel();
            }
        }

        this->TrySendToCurrentGateway(thisSPtr);
    }

private:
    void TrySendToCurrentGateway(AsyncOperationSPtr const & thisSPtr)
    {
        if (!this->GetOwner().IsOpened()) 
        { 
            this->TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed); 

            return;
        }

        ErrorCode nonRetryableError;
        if (GetOwner().NonRetryableErrorEncountered(nonRetryableError))
        {
            WriteInfo(
                TraceComponent,
                this->GetTraceId(),
                "{0}: completing '{1}' with non-retryable error {2}",
                this->GetActivityId(),
                GetRequestAction(),
                nonRetryableError);

            TryComplete(thisSPtr, nonRetryableError);
            return;
        }

        ISendTarget::SPtr target;
        if (!this->GetOwner().TryGetCurrentTarget(target))
        {
            // OnGatewayConnected() can fire as soon as we have added
            // ourselves to the waiters list during TryGetCurrentTargetOrAddWaiter()
            //
            AcquireExclusiveLock lock(timerLock_);

            if (!this->GetOwner().TryGetCurrentTargetOrAddWaiter(thisSPtr, target))
            {
                // Optimization: We could alternatively just schedule a retry with
                //               ConnectionInitializationTimeout. It would be simpler,
                //               but the trade-off is that incoming requests in a
                //               disconnected state can be delayed.
                //
                this->StartGatewayTimerCallerHoldsLock(thisSPtr);
            }
        }

        if (target)
        {
            this->SendRequest(thisSPtr, target);
        }
    }

protected:
    void OnSendRequestComplete(
        AsyncOperationSPtr const & thisSPtr, 
        __in ErrorCode & error,
        ClientServerReplyMessageUPtr && reply)
    {
        if (error.IsSuccess())
        {
            error = this->ProcessReply(reply);

            if (error.IsSuccess())
            {
                reply_ = move(reply);
            }

            this->TryComplete(thisSPtr, error);
        }
        else if (error.IsError(ErrorCodeValue::Timeout) || error.IsError(ErrorCodeValue::OperationCanceled))
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->ScheduleSendRequest(thisSPtr);
        }
    }

    void StartGatewayTimerCallerHoldsLock(AsyncOperationSPtr const & thisSPtr)
    {
        gatewayTimer_ = Timer::Create("Client.GatewayTimeout", [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();

                this->OnGatewayTimeout(thisSPtr);
            });
        gatewayTimer_->Change(this->GetRemainingTime());
    }

    // These retries should only occur when the session has either not been
    // established yet or has been disconnected. These retries must not
    // occur on behalf of the application. It is up to the application to
    // decide the idempotency of their request and retry as appropriate.
    //
    void ScheduleSendRequest(AsyncOperationSPtr const & thisSPtr)
    {
        auto delay = this->GetOwner().ClientSettings->ConnectionInitializationTimeout;
        if (delay > this->GetRemainingTime())
        {
            delay = this->GetRemainingTime();
        }

        if (delay > TimeSpan::Zero)
        {
            WriteNoise(
                TraceComponent,
                this->GetTraceId(),
                "{0}: scheduling {1} retry in {2}",
                this->GetActivityId(),
                this->GetRequestAction(),
                delay);

            AcquireExclusiveLock lock(timerLock_);

            if (gatewayTimer_)
            {
                gatewayTimer_->Cancel();
            }

            if (retryTimer_)
            {
                retryTimer_->Cancel();
            }

            retryTimer_ = Timer::Create("Client.SendRetry", [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();

                    this->TrySendToCurrentGateway(thisSPtr);
                });
            retryTimer_->Change(delay);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                this->GetTraceId(),
                "{0}: timed out retrying send to gateway",
                this->GetActivityId());

            this->TryComplete(thisSPtr, ErrorCodeValue::GatewayUnreachable);
        }
    }

    void OnGatewayTimeout(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            this->GetTraceId(),
            "{0}: timed out waiting for gateway connection",
            this->GetActivityId());

        this->GetOwner().RemoveWaiter(thisSPtr);

        this->TryComplete(thisSPtr, ErrorCodeValue::GatewayUnreachable);
    }

    ClientServerReplyMessageUPtr reply_;
    TimerSPtr retryTimer_;
    TimerSPtr gatewayTimer_;
    ExclusiveLock timerLock_;
};

//
// PingGatewayAsyncOperation
//

class ClientConnectionManager::PingGatewayAsyncOperation : public RequestReplyAsyncOperationBase
{
public:
    PingGatewayAsyncOperation(
        __in ClientConnectionManager & owner,
        ISendTarget::SPtr const & target,
        ClientServerRequestMessageUPtr && request,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : RequestReplyAsyncOperationBase(
            owner, 
            move(request),
            TransportPriority::Normal,
            owner.ClientSettings->ConnectionInitializationTimeout,
            callback, 
            parent)
        , pingTarget_(target)
        , gatewayDescription_()
    {
    }

    static AsyncOperationSPtr Begin(
        __in ClientConnectionManager & owner,
        ISendTarget::SPtr const & target,
        ActivityId const & pingActivity,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto request = NamingTcpMessage::GetGatewayPingRequest();
        request->Headers.Replace(FabricActivityHeader(pingActivity));

        return AsyncOperation::CreateAndStart<PingGatewayAsyncOperation>(
            owner,
            target,
            move(request),
            callback,
            parent);
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out ISendTarget::SPtr & targetResult,
        __out GatewayDescription & gatewayResult,
        __out ActivityId & pingActivity)
    {
        auto casted = AsyncOperation::End<PingGatewayAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            targetResult = casted->pingTarget_;
            gatewayResult = casted->gatewayDescription_;
        }

        pingActivity = casted->GetActivityId();

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->SendRequest(thisSPtr, pingTarget_);
    }

private:
    void OnSendRequestComplete(
        AsyncOperationSPtr const & thisSPtr, 
        __in ErrorCode & error,
        ClientServerReplyMessageUPtr && reply)
    {
        if (error.IsSuccess())
        {
            error = this->ProcessReply(reply);
        }

        if (error.IsSuccess())
        {
            ClientProtocolVersionHeader versionHeader;
            if (reply->Headers.TryReadFirst(versionHeader)
                && versionHeader.IsPingReplyBodySupported())
            {
                PingReplyMessageBody body;
                if (reply->GetBody(body))
                {
                    gatewayDescription_ = body.TakeDescription();
                }
                else
                {
                    error = ErrorCode::FromNtStatus(reply->GetStatus());
                }
            }
        }

        this->TryComplete(thisSPtr, error);
    }

    ISendTarget::SPtr pingTarget_;
    GatewayDescription gatewayDescription_;
};

//
// ConnectionEventQueueItem
//
class ClientConnectionManager::ConnectionEventQueueItem
{
public:

    static ConnectionEventQueueItemSPtr CreateConnectionEvent(
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway)
    {
        return shared_ptr<ConnectionEventQueueItem>(new ConnectionEventQueueItem(true, target, gateway, ErrorCodeValue::Success));
    }

    static ConnectionEventQueueItemSPtr CreateDisconnectionEvent(
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway,
        ErrorCode const & error)
    {
        return shared_ptr<ConnectionEventQueueItem>(new ConnectionEventQueueItem(false, target, gateway, error));
    }

    __declspec (property(get=get_IsConnectionEvent)) bool IsConnectionEvent;
    bool get_IsConnectionEvent() const { return isConnectionEvent_; }

    __declspec (property(get=get_Target)) Transport::ISendTarget::SPtr const & Target;
    Transport::ISendTarget::SPtr const & get_Target() const { return target_; }

    __declspec (property(get=get_Gateway)) Naming::GatewayDescription const & Gateway;
    Naming::GatewayDescription const & get_Gateway() const { return gateway_; }

    __declspec (property(get=get_Error)) ErrorCode const & Error;
    ErrorCode const & get_Error() const { return error_; }

private:
    ConnectionEventQueueItem(
        bool isConnectionEvent,
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway,
        ErrorCode const & error)
        : isConnectionEvent_(isConnectionEvent)
        , target_(target)
        , gateway_(gateway)
        , error_(error)
    {
    }

    bool isConnectionEvent_;
    ISendTarget::SPtr target_;
    GatewayDescription gateway_;
    ErrorCode error_;
};

//
// ConnectionEventHandlerEntry
//
class ClientConnectionManager::ConnectionEventHandlerEntry : public ComponentRoot
{
public:

    static ConnectionEventHandlerEntrySPtr Create(
        HandlerId const & handlerId,
        ConnectionHandler const & connectionHandler,
        DisconnectionHandler const & disconnectionHandler,
        ClaimsRetrievalHandler const & claimsRetrievalHandler)
    {
        return shared_ptr<ConnectionEventHandlerEntry>(new ConnectionEventHandlerEntry(
            handlerId,
            connectionHandler, 
            disconnectionHandler,
            claimsRetrievalHandler));
    }

    void PostConnectionEvent(
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway)
    {
        {
            AcquireWriteLock lock(eventsLock_);

            eventQueue_.push(ConnectionEventQueueItem::CreateConnectionEvent(
                target,
                gateway));
        }

        this->PostRaiseEventHandlers();
    }

    void PostDisconnectionEvent(
        ISendTarget::SPtr const & target,
        GatewayDescription const & gateway,
        ErrorCode const & error)
    {
        {
            AcquireWriteLock lock(eventsLock_);

            eventQueue_.push(ConnectionEventQueueItem::CreateDisconnectionEvent(
                target,
                gateway,
                error));
        }

        this->PostRaiseEventHandlers();
    }

    ErrorCode RaiseClaimsRetrievalEventHandler(ClaimsRetrievalMetadataSPtr const & metadata, __out std::wstring & token)
    {
        return claimsRetrievalHandler_(handlerId_, metadata, token);
    }

private:
    ConnectionEventHandlerEntry(
        HandlerId const & handlerId,
        ConnectionHandler const & connectionHandler,
        DisconnectionHandler const & disconnectionHandler,
        ClaimsRetrievalHandler const & claimsRetrievalHandler)
        : handlerId_(handlerId)
        , connectionHandler_(connectionHandler)
        , disconnectionHandler_(disconnectionHandler)
        , claimsRetrievalHandler_(claimsRetrievalHandler)
        , eventQueue_()
        , isRaiseHandlerThreadActive_(false)
        , eventsLock_()
    {
    }
        
    void PostRaiseEventHandlers()
    {
        // Each registered handler pair is raised using it's own thread
        // to avoid issues with blocked handlers. For example, one such
        // handler pair is exposed through the FabricClient's public API 
        // and we do not want a blocked application handler to affect our
        // processing of service notification reconnections (different
        // handler pair).
        //
        auto root = this->CreateComponentRoot();
        Threadpool::Post([this, root]() { this->RaiseEventHandlers(); });
    }

    void RaiseEventHandlers()
    {
        bool done = false;

        while (!done)
        {
            // Only keep one thread active for raising event handlers
            // to provide ordering guarantees between connection/disconnection
            // events.
            //
            done = isRaiseHandlerThreadActive_.exchange(true);

            if (!done)
            {
                queue<ConnectionEventQueueItemSPtr> eventsToDispatch;
                {
                    AcquireWriteLock lock(eventsLock_);

                    eventsToDispatch.swap(eventQueue_);
                }

                while (!eventsToDispatch.empty())
                {
                    auto event = eventsToDispatch.front();

                    if (event->IsConnectionEvent)
                    {
                        connectionHandler_(handlerId_, event->Target, event->Gateway);
                    }
                    else
                    {
                        disconnectionHandler_(handlerId_, event->Target, event->Gateway, event->Error);
                    }

                    eventsToDispatch.pop();
                }

                isRaiseHandlerThreadActive_.store(false);

                {
                    AcquireReadLock lock(eventsLock_);

                    done = eventQueue_.empty();
                }

            } // if !isRaiseHandlerThreadActive_
        } // while !done
    }

    HandlerId handlerId_;
    ConnectionHandler connectionHandler_;
    DisconnectionHandler disconnectionHandler_;
    ClaimsRetrievalHandler claimsRetrievalHandler_;
    queue<ConnectionEventQueueItemSPtr> eventQueue_;
    Common::atomic_bool isRaiseHandlerThreadActive_;
    RwLock eventsLock_;
};

//
// ClientConnectionManager
//

ClientConnectionManager::ClientConnectionManager(
    std::wstring const & traceId,
    INotificationClientSettingsUPtr && settings,
    std::vector<std::wstring> && gatewayAddresses,
    INamingMessageProcessorSPtr const &namingMessageProcessorSPtr,
    Common::ComponentRoot const & root)
    : RootedObject(root)
    , FabricComponent()
    , traceId_(traceId)
    , gatewayAddresses_(move(gatewayAddresses))
    , settings_(move(settings))
    , currentAddressIndex_(0)
    , currentTarget_()
    , currentGateway_()
    , currentPingActivityId_()
    , isConnected_(false)
    , waitingRequests_()
    , targetLock_()
    , connectionTimer_()
    , timerLock_()
    , nextHandlerId_(1)
    , connectionEventHandlers_()
    , handlersLock_()
    , transport_()
    , nonRetryableErrorLock_()
    , nonRetryableError_()
    , refreshClaimsToken_(true)
{
    if (namingMessageProcessorSPtr.get() != NULL)
    {
        transport_ = make_shared<ClientServerPassThroughTransport>(
            root,
            this->TraceId,
            L"FabricClient",
            namingMessageProcessorSPtr);

        // Initialize the gateway address list with a dummy address.
        gatewayAddresses_.push_back(L"localhost:0");
    }
    else
    {
        //
        // We try to maintain a connection with the gateway always(ping logic below). So
        // setting the connection idle timeout to zero.
        //
        transport_ = make_shared<ClientServerTcpTransport>(
            root, 
            this->TraceId, 
            L"FabricClient", 
            ServiceModelConfig::GetConfig().MaxMessageSize,
            (*settings_)->KeepAliveInterval,
            TimeSpan::Zero); // connection idle timeout
    }
}

ErrorCode ClientConnectionManager::OnOpen()
{
    // Don't access settings in FabricComponent state callbacks since FabricClientInternalSettingsHolder
    // may take the state lock when updating the settings.
    //
    auto error = transport_->Open();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "Transport::Open failed: {0}",
            error);
        return error;
    }

    error = this->InitializeAndValidateGatewayAddresses();
    if (!error.IsSuccess())
    {
        return error;
    }

    auto root = this->Root.CreateComponentRoot();

    transport_->SetClaimsRetrievalHandler([this, root](ClaimsRetrievalMetadataSPtr const & metadata, SecurityContextSPtr const & context)
        {
            this->HandleClaimsRetrieval(metadata, context);
        });

    transport_->SetConnectionFaultHandler([this, root](ISendTarget const & target, ErrorCode error)
        {
            this->OnConnectionFault(target, error);
        });

    ActivityId openActivity;
    {
        AcquireReadLock lock(targetLock_);

        openActivity = currentPingActivityId_;
    }

    WriteInfo(
        TraceComponent,
        this->TraceId,
        "{0}: scheduling ping on open",
        openActivity);

    this->ScheduleEstablishConnection(
        TimeSpan::Zero,
        openActivity);

    return error;
}

ErrorCode ClientConnectionManager::OnClose()
{
    {
        AcquireExclusiveLock lock(timerLock_);

        if (connectionTimer_)
        {
            connectionTimer_->Cancel();
        }
    }

    ErrorCode error(ErrorCodeValue::Success);
    if (transport_)
    {
        transport_->RemoveConnectionFaultHandler();
        transport_->RemoveClaimsRetrievalHandler();

        error = transport_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                 TraceComponent,
                 this->TraceId,
                "Transport::Close failed: {0}",
                error);
        }
    }

    return error;
}

void ClientConnectionManager::OnAbort()
{
    if (transport_)
    {
        transport_->RemoveConnectionFaultHandler();
        transport_->Abort();
    }
}

void ClientConnectionManager::HandleClaimsRetrieval(
    ClaimsRetrievalMetadataSPtr const & metadata,
    SecurityContextSPtr const & context)
{
    auto root = this->Root.CreateComponentRoot();
    Threadpool::Post([this, root, metadata, context] { this->DefaultClaimsRetrievalHandler(metadata, context); });
}

void ClientConnectionManager::DefaultClaimsRetrievalHandler(
    ClaimsRetrievalMetadataSPtr const & metadata,
    SecurityContextSPtr const & context)
{
    if (!metadata)
    {
        auto msg = wformatString("ClaimsRetrievalMetadata is null");

        WriteError(
            TraceComponent,
            this->TraceId,
            "{0}",
            msg);

        Assert::TestAssert("{0}", msg);

        return;
    }

    ErrorCode error;
    wstring token;

    map<HandlerId, ConnectionEventHandlerEntrySPtr> handlers;
    {
        AcquireReadLock lock(handlersLock_);

        handlers = connectionEventHandlers_;
    }

    for (auto const & handlersPair : handlers)
    {
        error = handlersPair.second->RaiseClaimsRetrievalEventHandler(metadata, token);

        WriteInfo(
            TraceComponent, 
            this->TraceId, 
            "ClaimsRetrievalHandler: id={0} error={1} token={2}", 
            handlersPair.first,
            error,
            !token.empty());

        if (error.IsSuccess() && !token.empty())
        {
            break;
        }
    }

    if (token.empty() && metadata->IsAAD)
    {
        error = AzureActiveDirectory::ClientWrapper::GetToken(
            metadata,
            (*settings_)->AuthTokenBufferSize,
            refreshClaimsToken_,
            token); // out

        if (error.IsSuccess())
        {
            // Force session refresh when a new client is created
            // (e.g. reconnecting in PowerShell). Otherwise, delegate
            // token refreshing decisions to library.
            //
            refreshClaimsToken_ = false;
        }
    }

    context->CompleteClaimsRetrieval(error, token);
}

ErrorCode ClientConnectionManager::InitializeAndValidateGatewayAddresses()
{
    if (gatewayAddresses_.empty())
    {
        StringUtility::Split<wstring>(ClientConfig::GetConfig().NodeAddresses, gatewayAddresses_, L",");

        WriteInfo(
            TraceComponent,
            this->TraceId,
            "loaded {0} gateway addresses from configuration: {1}",
            gatewayAddresses_.size(),
            gatewayAddresses_);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "initialized {0} gateway addresses from ctor: {1}",
            gatewayAddresses_.size(),
            gatewayAddresses_);
    }

    if (gatewayAddresses_.empty())
    {
        return ErrorCodeValue::InvalidAddress;
    }

    auto & transportConfig = TransportConfig::GetConfig();
    ResolveOptions::Enum hostnameResolveOption;
    if (!ResolveOptions::TryParse(transportConfig.ResolveOption, hostnameResolveOption))
    {
        WriteError(
            TraceComponent,
            this->TraceId,
            "failed to parse configured address type {0}",
            transportConfig.ResolveOption);

        return ErrorCodeValue::InvalidConfiguration;
    }

    for (auto const & address : gatewayAddresses_)
    {
        if (TcpTransportUtility::IsValidEndpointString(address))
        {
            continue;
        }

        wstring hostname;
        USHORT port;

        if (!TcpTransportUtility::TryParseHostNameAddress(address, hostname, port))
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "failed to parse {0} as a hostname address (<hostname>:<0-65535>)",
                address);

            return ErrorCodeValue::InvalidAddress;
        }
    }

    std::shuffle(gatewayAddresses_.begin(), gatewayAddresses_.end(), std::default_random_engine{});

    WriteInfo(
        TraceComponent,
        this->TraceId,
        "randomized gateway addresses: {0}",
        gatewayAddresses_);

    return ErrorCodeValue::Success;
}

void ClientConnectionManager::UpdateTraceId(wstring const & id)
{
    traceId_ = id;
}

ErrorCode ClientConnectionManager::SetSecurity(SecuritySettings const & securitySettings)
{
    auto error = transport_->SetSecurity(securitySettings);
    if (error.IsSuccess())
    {
        ClearNonRetryableError();

        //
        // Reset the current target so that the ping workflow can be restarted with the new
        // credentials.
        //
        {
            AcquireWriteLock grab(targetLock_);
            if (currentTarget_)
            {
                currentTarget_.reset();
            }
        }

        ActivityId activityId;
        this->ScheduleEstablishConnection(
            TimeSpan::Zero,
            activityId);
    }

    return error;
}

void ClientConnectionManager::SetKeepAlive(TimeSpan const interval)
{
    transport_->SetKeepAliveTimeout(interval);
}

ErrorCode ClientConnectionManager::SetNotificationHandler(DuplexRequestReply::NotificationHandler const & handler)
{
    if (!this->IsOpened()) { return ErrorCodeValue::ObjectClosed; }

    transport_->SetNotificationHandler(handler, false);

    return ErrorCodeValue::Success;
}

ErrorCode ClientConnectionManager::RemoveNotificationHandler()
{
    if (!this->IsOpened()) { return ErrorCodeValue::ObjectClosed; }

    transport_->RemoveNotificationHandler();

    return ErrorCodeValue::Success;
}

void ClientConnectionManager::OnConnectionFault(ISendTarget const & disconnectedTarget, ErrorCode const & error)
{
    if (RecordNonRetryableError(error))
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "stop connection retry on non-retryable {0} from {1}",
            error,
            disconnectedTarget.Address());

        AcquireReadLock grab(timerLock_);
        connectionTimer_->Change(TimeSpan::MaxValue);

        //
        // The current target need not be reset here, because the same non retryable error will be received
        // even if we try another gateway.
        //

        return;
    }

    if (error.IsError(ErrorCodeValue::CannotConnect))
    {
        WriteNoise(
            TraceComponent,
            this->TraceId,
            "cannot connect to gateway {0}",
            disconnectedTarget.Address());

        return;
    }

    ActivityId disconnectActivity;

    WriteInfo(
        TraceComponent,
        this->TraceId,
        "{0}: disconnected from gateway {1}: {2}",
        disconnectActivity,
        disconnectedTarget.Address(),
        error);

    bool wasConnected = false;
    ISendTarget::SPtr snapTarget;
    GatewayDescription snapGateway;
    {
        AcquireWriteLock targetLock(targetLock_);

        if (currentTarget_ && disconnectedTarget.Address() == currentTarget_->Address())
        {
            // The transport will fire a disconnection event on every ping attempt.
            // Schedule the subsequent ping in this case. Otherwise, if we were
            // already connected, then we want to re-connect as soon as possible.
            //
            // There is a case where the connection may fault immediately after
            // a successful connection, so we cannot ignore this disconnection
            // event even if we don't seem to be connected "yet".
            //
            wasConnected = isConnected_;

            snapTarget = currentTarget_;

            snapGateway = currentGateway_;

            currentTarget_.reset();

            currentGateway_.Clear();

            currentPingActivityId_ = disconnectActivity;

            isConnected_ = false;

            if (wasConnected)
            {
                AcquireReadLock handlersLock(handlersLock_);

                for (auto const & handlersPair : connectionEventHandlers_)
                {
                    handlersPair.second->PostDisconnectionEvent(snapTarget, snapGateway, error);
                }
            }
        }
    }

    if (snapTarget)
    {
        this->ScheduleEstablishConnection(
            (wasConnected ? TimeSpan::Zero : this->ClientSettings->ConnectionInitializationTimeout),
            disconnectActivity);
    }
}

void ClientConnectionManager::ScheduleEstablishConnection(
    TimeSpan const delay,
    ActivityId const & activityId)
{
    if (!this->IsOpeningOrOpened()) { return; }

    AcquireExclusiveLock lock(timerLock_);

    if (connectionTimer_)
    {
        connectionTimer_->Cancel();
    }

    auto root = this->Root.CreateComponentRoot();
    connectionTimer_ = Timer::Create("Client.Connect", [this, root, activityId](TimerSPtr const & timer)
        {
            timer->Cancel();

            this->EstablishConnection(activityId);
        });
    connectionTimer_->Change(delay);
}

void ClientConnectionManager::EstablishConnection(ActivityId const & activityId)
{
    ISendTarget::SPtr pingTarget;
    ActivityId pingActivity;
    wstring address;
    {
        AcquireWriteLock lock(targetLock_);

        if (currentTarget_ && currentPingActivityId_ != activityId) 
        { 
            WriteInfo(
                TraceComponent,
                this->TraceId,
                "{0}: already pinging gateway {1}: ignoring ping workflow {2}",
                currentPingActivityId_,
                currentTarget_->Address(),
                activityId);

            return; 
        }

        // In practice, a new ping is likely triggered by a faulting gateway, 
        // which means that pinging the same gateway again immediately is likely 
        // to fail. Increase our chances of reconnecting quickly by switching 
        // targets on every new ping attempt.
        //
        currentAddressIndex_ = (currentAddressIndex_ + 1) % gatewayAddresses_.size();

        address = gatewayAddresses_[currentAddressIndex_];

        currentTarget_ = transport_->ResolveTarget(address);

        pingTarget = currentTarget_;

        pingActivity = currentPingActivityId_;
    }

    if (pingTarget)
    {
        // Account for how much time was spent performing the ping when scheduling
        // retries to avoid pinging too frequently.
        //
        Stopwatch stopwatch;
        stopwatch.Start();

        auto operation = PingGatewayAsyncOperation::Begin(
            *this,
            pingTarget,
            pingActivity,
            [this, stopwatch](AsyncOperationSPtr const & operation) { this->OnPingRequestComplete(stopwatch, operation, false); },
            this->Root.CreateAsyncOperationRoot());
        this->OnPingRequestComplete(stopwatch, operation, true);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "{0}: failed to resolve ping target {1}",
            pingActivity,
            address);

        this->ScheduleEstablishConnection(
            this->ClientSettings->ConnectionInitializationTimeout,
            pingActivity);
    }
}

void ClientConnectionManager::OnPingRequestComplete(
    Stopwatch const & stopwatch,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ISendTarget::SPtr connectedTarget;
    GatewayDescription gateway;
    ActivityId pingActivity;
    auto error = PingGatewayAsyncOperation::End(operation, connectedTarget, gateway, pingActivity);

    WaitingRequestsMap waitersToFlush;
    bool scheduleRetry = false;

    if (error.IsSuccess())
    {
        AcquireWriteLock lock1(targetLock_);

        if (currentTarget_)
        {
            if (currentPingActivityId_ == pingActivity)
            {
                currentGateway_ = gateway;

                isConnected_ = true;

                waitersToFlush = move(waitingRequests_);

                WriteInfo(
                    TraceComponent,
                    this->TraceId,
                    "{0}: connected to target={1} gateway={2} priority={3}",
                    currentPingActivityId_,
                    connectedTarget->Address(),
                    currentGateway_,
                    TransportPriority::Normal);

                {
                    AcquireReadLock lock2(handlersLock_);

                    for (auto const & handlerPair : connectionEventHandlers_)
                    {
                        handlerPair.second->PostConnectionEvent(currentTarget_, currentGateway_);
                    }
                }
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    this->TraceId,
                    "{0}: ping activity no longer current: target={1} activity={2}",
                    pingActivity,
                    currentTarget_->Address(),
                    currentPingActivityId_);
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                this->TraceId,
                "{0}: ping activity no longer current: activity={1}",
                pingActivity,
                currentPingActivityId_);
        }
    }
    else
    {
        AcquireWriteLock lock1(targetLock_);

        if (IsNonRetryableError(error))
        {
            waitersToFlush = move(waitingRequests_);
        }
        else
        {
            if (currentTarget_)
            {
                if (currentPingActivityId_ == pingActivity)
                {
                    WriteInfo(
                        TraceComponent,
                        this->TraceId,
                        "{0}: retrying ping: target={1}",
                        pingActivity,
                        currentTarget_->Address());

                    // Only schedule a retry if this ping activity is still current to
                    // avoid cancelling the timer of the actual current ping activity.
                    //
                    scheduleRetry = true;
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        this->TraceId,
                        "{0}: ping activity no longer current: target={1} activity={2}",
                        pingActivity,
                        currentTarget_->Address(),
                        currentPingActivityId_);
                }
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    this->TraceId,
                    "{0}: ping activity no longer current: activity={1}",
                    pingActivity,
                    currentPingActivityId_);
            }
        }
    }

    if (!waitersToFlush.empty())
    {
        FlushWaitingRequests(error, pingActivity, move(waitersToFlush));
        return;
    }

    if (scheduleRetry)
    {
        auto delay = this->ClientSettings->ConnectionInitializationTimeout.SubtractWithMaxAndMinValueCheck(stopwatch.Elapsed);
        if (delay < TimeSpan::Zero)
        {
            delay = TimeSpan::Zero;
        }

        this->ScheduleEstablishConnection(delay, pingActivity);
    }
}

bool ClientConnectionManager::TryGetCurrentTarget(
    __out ISendTarget::SPtr & target)
{
    // Improve parallelism in connected path at the expense of the unconnected path
    //
    AcquireReadLock lock(targetLock_);

    if (isConnected_ && currentTarget_)
    {
        target = currentTarget_;

        return true;
    }

    return false;
}

bool ClientConnectionManager::TryGetCurrentTargetOrAddWaiter(
    AsyncOperationSPtr const & uncastedWaiter, 
    __out ISendTarget::SPtr & target)
{
    AcquireWriteLock lock(targetLock_);

    if (isConnected_ && currentTarget_)
    {
        target = currentTarget_;

        return true;
    }
    else
    {
        // Lazy dynamic cast
        //
        SendToGatewayAsyncOperationSPtr waiter;
        if (this->TryGetCasted(uncastedWaiter, waiter))
        {
            auto result = waitingRequests_.insert(make_pair(
                waiter->GetWaiterId(),
                waiter));

            if (!result.second)
            {
                auto msg = wformatString("duplicate waiter: activity={0} waiter={1}", waiter->GetActivityId(), waiter->GetWaiterId());

                WriteError(
                    TraceComponent,
                    this->TraceId,
                    "{0}",
                    msg);

                Assert::TestAssert("{0}", msg);
            }
        }
    }

    return false;
}

void ClientConnectionManager::RemoveWaiter(AsyncOperationSPtr const & uncastedWaiter)
{
    AcquireWriteLock lock(targetLock_);

    // Lazy dynamic cast
    //
    SendToGatewayAsyncOperationSPtr waiter;
    if (this->TryGetCasted(uncastedWaiter, waiter))
    {
        waitingRequests_.erase(waiter->GetWaiterId());
    }
}

void ClientConnectionManager::FlushWaitingRequests(
    ErrorCode const & error,
    ActivityId const & activityId,
    WaitingRequestsMap && waitersToFlush)
{
    if (!waitersToFlush.empty())
    {
        auto waitersSPtr = make_shared<WaitingRequestsMap>(move(waitersToFlush));
        
        Threadpool::Post([this, error, activityId, waitersSPtr]()
        {
            WriteInfo(
                TraceComponent,
                this->TraceId,
                "{0}: flushing {1} waiting requests with {2}",
                activityId,
                waitersSPtr->size(),
                error);

            for (auto const & waiterPair : *waitersSPtr)
            {
                auto const & waiter = waiterPair.second;

                if (error.IsSuccess())
                {
                    waiter->OnGatewayConnected(waiter);
                }
                else
                {
                    waiter->TryComplete(waiter, error);
                }
            }
        });
    }
}

bool ClientConnectionManager::TryGetCasted(
    AsyncOperationSPtr const & uncasted,
    __out SendToGatewayAsyncOperationSPtr & casted)
{
    auto waiter = dynamic_pointer_cast<SendToGatewayAsyncOperation>(uncasted);
    if (waiter)
    {
        casted = move(waiter);

        return true;
    }
    else
    {
        auto msg = wformatString("could not cast to SendToGatewayAsyncOperation");

        WriteError(
            TraceComponent,
            this->TraceId,
            "{0}",
            msg);

        Assert::TestAssert("{0}", msg);
    }

    return false;
}

ISendTarget::SPtr ClientConnectionManager::TryGetCurrentTargetAndGateway(__out unique_ptr<GatewayDescription> & gateway)
{
    AcquireReadLock lock(targetLock_);

    if (isConnected_ && currentTarget_)
    {
        gateway = make_unique<GatewayDescription>(currentGateway_);

        return currentTarget_;
    }

    return ISendTarget::SPtr();
}

ClientConnectionManager::HandlerId ClientConnectionManager::RegisterConnectionHandlers(
    ConnectionHandler const & connectionHandler,
    DisconnectionHandler const & disconnectionHandler,
    ClaimsRetrievalHandler const & claimsRetrievalHandler)
{
    AcquireWriteLock lock(handlersLock_);

    auto handlerId = nextHandlerId_++;

    auto result = connectionEventHandlers_.insert(make_pair(
        handlerId,
        ConnectionEventHandlerEntry::Create(handlerId, connectionHandler, disconnectionHandler, claimsRetrievalHandler)));

    if (!result.second)
    {
        auto msg = wformatString("connection handler {0} already exists", handlerId);

        WriteError(
            TraceComponent,
            this->TraceId,
            "{0}",
            msg);

        Assert::TestAssert("{0}", msg);

        return 0;
    }

    return handlerId;
}

void ClientConnectionManager::UnregisterConnectionHandlers(HandlerId id)
{
    AcquireWriteLock lock(handlersLock_);

    connectionEventHandlers_.erase(id);
}

ISendTarget::SPtr const & ClientConnectionManager::get_CurrentTarget() const
{
    AcquireReadLock lock(targetLock_);

    return currentTarget_;
}

GatewayDescription const & ClientConnectionManager::get_CurrentGateway() const
{
    AcquireReadLock lock(targetLock_);

    return currentGateway_;
}

bool ClientConnectionManager::IsCurrentGateway(
    GatewayDescription const & toCheck,
    __out GatewayDescription & current)
{
    AcquireReadLock lock(targetLock_);

    bool isCurrent = (toCheck == currentGateway_);

    if (!isCurrent)
    {
        current = currentGateway_;
    }

    return isCurrent;
}

bool ClientConnectionManager::TryGetCurrentGateway(__out GatewayDescription & current)
{
    auto temp = make_unique<GatewayDescription>();
    if (this->TryGetCurrentTargetAndGateway(temp))
    {
        current = *temp;
        return true;
    }

    return false;
}

void ClientConnectionManager::ForceDisconnect()
{
    AcquireReadLock lock(targetLock_);

    if (currentTarget_)
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "force disconnect: {0} ({1})",
            currentTarget_->Id(),
            currentGateway_);

        currentTarget_->Reset();
    }
}

AsyncOperationSPtr ClientConnectionManager::BeginSendToGateway(
    ClientServerRequestMessageUPtr && request,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SendToGatewayAsyncOperation>(
        *this,
        move(request),
        TransportPriority::Normal,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ClientConnectionManager::BeginSendHighPriorityToGateway(
    ClientServerRequestMessageUPtr && request,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SendToGatewayAsyncOperation>(
        *this,
        move(request),
        TransportPriority::High,
        timeout,
        callback,
        parent);
}

ErrorCode ClientConnectionManager::EndSendToGateway(
    AsyncOperationSPtr const & operation,
    __out ClientServerReplyMessageUPtr & reply)
{
    return SendToGatewayAsyncOperation::End(operation, reply);
}

ErrorCode ClientConnectionManager::SendOneWayToGateway(
    ClientServerRequestMessageUPtr && request, 
    __inout unique_ptr<GatewayDescription> & gateway,
    TimeSpan timeout)
{
    unique_ptr<GatewayDescription> currentGateway;
    auto target = this->TryGetCurrentTargetAndGateway(currentGateway);

    if (target)
    {
        if (!gateway)
        {
            gateway = move(currentGateway);
        }
        else if (*currentGateway != *gateway)
        {
            // The gateway has changed since the last request.
            //
            return ErrorCodeValue::GatewayUnreachable;
        }

        return transport_->SendOneWay(target, move(request), timeout);
    }
    else
    {
        return ErrorCodeValue::NotReady;
    }
}

ISendTarget::SPtr ClientConnectionManager::Test_ResolveTarget(wstring const & address)
{
    return transport_->ResolveTarget(address);
}

AsyncOperationSPtr ClientConnectionManager::Test_BeginSend(
    ClientServerRequestMessageUPtr && request,
    ISendTarget::SPtr const & target,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return transport_->BeginRequestReply(
        move(request),
        target,
        TransportPriority::Normal,
        timeout,
        callback,
        parent);
}

ErrorCode ClientConnectionManager::Test_EndSend(
    AsyncOperationSPtr const & operation,
    __out ClientServerReplyMessageUPtr & reply)
{
    return transport_->EndRequestReply(operation, reply);
}

bool ClientConnectionManager::RecordNonRetryableError(Common::ErrorCode const & error)
{
    if (!IsNonRetryableError(error))
    {
        return false;
    }

    AcquireWriteLock grab(nonRetryableErrorLock_);
    nonRetryableError_.Overwrite(error);
    nonRetryableErrorTime_ = Stopwatch::Now();
    return true;
}

_Use_decl_annotations_
bool ClientConnectionManager::NonRetryableErrorEncountered(ErrorCode & error) const
{
    auto now = Stopwatch::Now();
    auto cacheExpiration = ClientConfig::GetConfig().NonRetryableErrorExpiration;

    AcquireReadLock grab(nonRetryableErrorLock_);

    if ((nonRetryableErrorTime_ + cacheExpiration) <= now)
    {
        return false;
    }

    error = nonRetryableError_;
    return IsNonRetryableError(error);
}

bool ClientConnectionManager::IsNonRetryableError(ErrorCode const & error)
{
    return
        error.IsError(ErrorCodeValue::ConnectionDenied) ||
        error.IsError(ErrorCodeValue::ServerAuthenticationFailed) ||
        error.IsError(ErrorCodeValue::InvalidCredentials);
}

void ClientConnectionManager::ClearNonRetryableError()
{
    AcquireWriteLock grab(nonRetryableErrorLock_);
    nonRetryableError_.ReadValue();
    nonRetryableError_.Reset(ErrorCodeValue::Success);
    nonRetryableErrorTime_ = StopwatchTime::Zero;
}
