// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Transport/TransportFwd.h"

namespace Client
{
    // This class attempts to maintain an active connection by pinging its 
    // list of gateway addresses whenever the current connection is disconnected.
    // The initial connection attempt occurs upon opening the client.
    //
    // Maintaining an active connection is needed for the service notification
    // feature.
    //
    class ClientConnectionManager 
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
    {
    public:
        typedef uint64 HandlerId;
        typedef std::function<void(HandlerId, Transport::ISendTarget::SPtr const &, Naming::GatewayDescription const &)> ConnectionHandler;
        typedef std::function<void(HandlerId, Transport::ISendTarget::SPtr const &, Naming::GatewayDescription const &, Common::ErrorCode const &)> DisconnectionHandler;
        typedef std::function<Common::ErrorCode(HandlerId, Transport::ClaimsRetrievalMetadataSPtr const &, std::wstring &)> ClaimsRetrievalHandler;

    public:

        ClientConnectionManager(
            std::wstring const & traceId,
            INotificationClientSettingsUPtr &&,
            std::vector<std::wstring> && gatewayAddresses,
            Naming::INamingMessageProcessorSPtr const &namingMessageProcessorSPtr,
            Common::ComponentRoot const & root);

        __declspec (property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec (property(get=get_CurrentTarget)) Transport::ISendTarget::SPtr const & CurrentTarget;
        Transport::ISendTarget::SPtr const & get_CurrentTarget() const;

        __declspec (property(get=get_CurrentGateway)) Naming::GatewayDescription const & CurrentGateway;
        Naming::GatewayDescription const & get_CurrentGateway() const;

        __declspec (property(get=get_ClientSettings)) INotificationClientSettings const & ClientSettings;
        INotificationClientSettings const & get_ClientSettings() const { return *settings_; }

        void UpdateTraceId(std::wstring const &);

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const &);
        void SetKeepAlive(Common::TimeSpan const);

        Common::ErrorCode SetNotificationHandler(Transport::DuplexRequestReply::NotificationHandler const &);
        Common::ErrorCode RemoveNotificationHandler();

        HandlerId RegisterConnectionHandlers(ConnectionHandler const &, DisconnectionHandler const &, ClaimsRetrievalHandler const &);
        void UnregisterConnectionHandlers(HandlerId);

        bool IsCurrentGateway(Naming::GatewayDescription const &, __out Naming::GatewayDescription & current);
        bool TryGetCurrentGateway(__out Naming::GatewayDescription & current);

        void ForceDisconnect();

        Common::AsyncOperationSPtr BeginSendToGateway(
            Client::ClientServerRequestMessageUPtr && request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        // This is used to establish a separate underlying connection for notification
        // traffic that does not interfere with normal client/server request/reply traffic.
        //
        Common::AsyncOperationSPtr BeginSendHighPriorityToGateway(
            Client::ClientServerRequestMessageUPtr && request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndSendToGateway(
            Common::AsyncOperationSPtr const & operation,
            __out ClientServerReplyMessageUPtr & reply);

    public:

        //
        // For testing only
        //

        Transport::ISendTarget::SPtr Test_ResolveTarget(std::wstring const &);

        Common::AsyncOperationSPtr Test_BeginSend(
            Client::ClientServerRequestMessageUPtr && request,
            Transport::ISendTarget::SPtr const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode Test_EndSend(
            Common::AsyncOperationSPtr const & operation,
            __out ClientServerReplyMessageUPtr & reply);

    public:

        //
        // FileTransferClient support
        //
        // ClientServerTransport is currently only exposed for use by FileTransferClient, which 
        // implements some of its own protocol at the transport level. Ideally, it should just 
        // go through the same Request/Reply path as other client operations. Higher level components 
        // should avoid accessing the transport level directly.
        //

        __declspec (property(get=get_Transport)) ClientServerTransportSPtr const & Transport;
        ClientServerTransportSPtr const & get_Transport() { return transport_; }

        // Sends one-way message to current gateway (if connected) and returns the gateway description if the second
        // argument is nullptr. Otherwise, checks that the current gateway is the same as the input argument
        // before sending.
        //
        // NotReady - not connected to any gateway yet.
        // GatewayUnreachable - the current connected gateway is different from the input argument
        //
        Common::ErrorCode SendOneWayToGateway(
            Client::ClientServerRequestMessageUPtr && request, 
            __inout std::unique_ptr<Naming::GatewayDescription> &,
            Common::TimeSpan timeout);

    protected:

        //
        // FabricComponent
        //
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();
        
    private:
        class RequestReplyAsyncOperationBase;
        class SendToGatewayAsyncOperation;
        class PingGatewayAsyncOperation;
        class ConnectionEventQueueItem;
        class ConnectionEventHandlerEntry;

        typedef std::shared_ptr<SendToGatewayAsyncOperation> SendToGatewayAsyncOperationSPtr;
        typedef std::map<uint64, SendToGatewayAsyncOperationSPtr> WaitingRequestsMap;
        typedef std::shared_ptr<ConnectionEventQueueItem> ConnectionEventQueueItemSPtr;
        typedef std::shared_ptr<ConnectionEventHandlerEntry> ConnectionEventHandlerEntrySPtr;

        void HandleClaimsRetrieval(
            Transport::ClaimsRetrievalMetadataSPtr const & metadata,
            Transport::SecurityContextSPtr const & context);

        void DefaultClaimsRetrievalHandler(
            Transport::ClaimsRetrievalMetadataSPtr const & metadata,
            Transport::SecurityContextSPtr const & context);

        Common::ErrorCode GetAccessTokenHelper(
            std::wstring const & authority,
            std::wstring const & audience,
            std::wstring const & clientId,
            std::wstring const & redirectUri,
            bool refreshClaimsToken,
            __out std::wstring & token);

        Common::ErrorCode InitializeAndValidateGatewayAddresses();

        void OnConnectionFault(Transport::ISendTarget const & disconnectedTarget, Common::ErrorCode const &);
        void ScheduleEstablishConnection(Common::TimeSpan const delay, Common::ActivityId const & pingActivity);
        void EstablishConnection(Common::ActivityId const & pingActivity);
        void OnPingRequestComplete(
            Common::Stopwatch const &,
            Common::AsyncOperationSPtr const &, 
            bool expectedCompletedSynchronously);

        bool TryGetCurrentTarget(
            __out Transport::ISendTarget::SPtr &);
        bool TryGetCurrentTargetOrAddWaiter(
            Common::AsyncOperationSPtr const &, 
            __out Transport::ISendTarget::SPtr &);
        void RemoveWaiter(Common::AsyncOperationSPtr const &);
        void FlushWaitingRequests(Common::ErrorCode const & error, Common::ActivityId const &, WaitingRequestsMap &&);

        bool TryGetCasted(Common::AsyncOperationSPtr const &, __out SendToGatewayAsyncOperationSPtr &);

        Transport::ISendTarget::SPtr TryGetCurrentTargetAndGateway(__out std::unique_ptr<Naming::GatewayDescription> &);

        bool RecordNonRetryableError(Common::ErrorCode const & error);
        void ClearNonRetryableError();
        bool NonRetryableErrorEncountered(_Out_ Common::ErrorCode & error) const;
        static bool IsNonRetryableError(Common::ErrorCode const & error);

        std::wstring traceId_;
        std::vector<std::wstring> gatewayAddresses_;

        INotificationClientSettingsUPtr settings_;

        size_t currentAddressIndex_;
        Transport::ISendTarget::SPtr currentTarget_;
        Naming::GatewayDescription currentGateway_;
        Common::ActivityId currentPingActivityId_;
        bool isConnected_;
        WaitingRequestsMap waitingRequests_;
        mutable Common::RwLock targetLock_;

        Common::TimerSPtr connectionTimer_;
        Common::ExclusiveLock timerLock_;

        HandlerId nextHandlerId_;
        std::map<HandlerId, ConnectionEventHandlerEntrySPtr> connectionEventHandlers_;
        Common::RwLock handlersLock_;

        ClientServerTransportSPtr transport_;
        mutable Common::RwLock nonRetryableErrorLock_;
        Common::ErrorCode nonRetryableError_;
        Common::StopwatchTime nonRetryableErrorTime_ = Common::StopwatchTime::Zero;

        bool refreshClaimsToken_;
    };

    typedef std::unique_ptr<ClientConnectionManager> ClientConnectionManagerUPtr;
    typedef std::shared_ptr<ClientConnectionManager> ClientConnectionManagerSPtr;
}
