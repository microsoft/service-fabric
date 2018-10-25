// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IDatagramTransport
    {
    public:
        IDatagramTransport();
        virtual ~IDatagramTransport() = 0;

        virtual Common::ErrorCode Start(bool completeStart = true) = 0;
        virtual Common::ErrorCode CompleteStart() = 0;
        virtual void Stop(Common::TimeSpan timeout = Common::TimeSpan::Zero) = 0;

        __declspec (property(get=get_IdString)) std::wstring const & IdString;
        virtual std::wstring const & get_IdString() const = 0;
        virtual std::wstring const & TraceId() const = 0;

        virtual void SetInstance(uint64 instance) = 0;

        virtual TransportSecuritySPtr Security() const = 0;
        virtual Common::ErrorCode SetSecurity(SecuritySettings const & securitySettings) = 0;

        virtual void SetFrameHeaderErrorChecking(bool enabled) = 0;
        virtual void SetMessageErrorChecking(bool enabled) = 0;

        virtual void DisableSecureSessionExpiration() = 0;

        virtual void DisableThrottle() = 0;
        virtual void AllowThrottleReplyMessage() = 0;

        virtual void DisableListenInstanceMessage() = 0;

        // Callback function object type for receiving messages.
        typedef std::function<void(
            MessageUPtr & message,
            ISendTarget::SPtr const & sender)> MessageHandler;

        typedef std::shared_ptr<MessageHandler> MessageHandlerSPtr;
        typedef std::weak_ptr<MessageHandler> MessageHandlerWPtr;

        // Set the handler for incoming messages
        virtual void SetMessageHandler(MessageHandler const & handler) = 0;

        // Creates the ISendTarget corresponding to an address.
        ISendTarget::SPtr ResolveTarget(NamedAddress const & namedAddress);

        ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId = L"",
            uint64 instance = 0);

        ISendTarget::SPtr ResolveTarget(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance = 0);

        static std::wstring TargetAddressToTransportAddress(std::wstring const & targetAddress);

        virtual size_t SendTargetCount() const = 0;

        // Sends a one-way message to a target.
        virtual Common::ErrorCode SendOneWay(
            ISendTarget::SPtr const & target,
            MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            TransportPriority::Enum = TransportPriority::Normal) = 0;

        typedef std::function<void(
            ISendTarget const & target)> ConnectionAcceptedHandler;

        // Handler for connection fault (including normal closure)
        // Deprecated by DisconnectEvent, todo, remove
        typedef std::function<void(
            ISendTarget const & target,
            Common::ErrorCode fault)> ConnectionFaultHandler;

        struct DisconnectEventArgs
        {
            DisconnectEventArgs(ISendTarget const* target, Common::ErrorCode const & fault)
            : Target(target), Fault(fault)
            {
            }

            ISendTarget const* Target;
            Common::ErrorCode Fault;
        };

        // Disconnect event is fired when a new connection fails to establish 
        // or an established connection is closed
        typedef Common::EventT<DisconnectEventArgs>::EventHandler DisconnectEventHandler;
        typedef Common::EventT<DisconnectEventArgs>::HHandler DisconnectHHandler;
        typedef Common::EventT<DisconnectEventArgs> DisconnectEvent;

        virtual DisconnectHHandler RegisterDisconnectEvent(DisconnectEventHandler eventHandler) = 0;
        virtual bool UnregisterDisconnectEvent(DisconnectHHandler hHandler) = 0;

        // Sets and removes handler for connection status.
        virtual void SetConnectionAcceptedHandler(ConnectionAcceptedHandler const &) = 0;
        virtual void RemoveConnectionAcceptedHandler() = 0;

        // Deprecated by DisconnectEvent, todo, remove
        virtual void SetConnectionFaultHandler(ConnectionFaultHandler const &) = 0;
        virtual void RemoveConnectionFaultHandler() = 0;

        virtual std::wstring const & ListenAddress() const = 0;

        // Set send queue size limit per send target
        virtual Common::ErrorCode SetPerTargetSendQueueLimit(ULONG limitInBytes) = 0;

        // Set how long an outgoing message can be queued until being sent or dropped
        virtual Common::ErrorCode SetOutgoingMessageExpiration(Common::TimeSpan expiration) = 0;

        virtual void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata && handler) = 0;
        virtual void SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const & handler) = 0;
        virtual void RemoveClaimsRetrievalHandler() = 0;

        virtual void SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler) = 0;
        virtual void RemoveClaimsHandler() = 0;

        // For DOS attack alleviation 
        virtual void SetMaxIncomingFrameSize(ULONG) = 0;
        virtual void SetMaxOutgoingFrameSize(ULONG) = 0;

        // The following is meaningful only When the underlaying transport is session based, e.g. TCP
        virtual Common::TimeSpan ConnectionOpenTimeout() const = 0;
        virtual void SetConnectionOpenTimeout(Common::TimeSpan timeout) = 0;
        virtual Common::TimeSpan ConnectionIdleTimeout() const = 0;
        virtual void SetConnectionIdleTimeout(Common::TimeSpan idleTimeout) = 0;
        virtual Common::TimeSpan KeepAliveTimeout() const = 0;
        virtual void SetKeepAliveTimeout(Common::TimeSpan timeout) = 0;

        virtual void EnableInboundActivityTracing() = 0;
        static bool IsPerMessageTraceDisabled(Actor::Enum actor);

        // Used by V1 replicator to disable per message traces without transport actor header
        virtual void DisableAllPerMessageTraces() = 0;

#ifdef PLATFORM_UNIX
        virtual Common::EventLoopPool* EventLoops() const = 0; 
        virtual void SetEventLoopPool(Common::EventLoopPool* pool) = 0;
        virtual void SetEventLoopReadDispatch(bool asyncDispatch) = 0;
        virtual void SetEventLoopWriteDispatch(bool asyncDispatch) = 0;
        static Common::EventLoopPool* GetDefaultTransportEventLoopPool();
#endif

        virtual void SetBufferFactory(std::unique_ptr<IBufferFactory> && bufferFactory) = 0;

        virtual void Test_Reset(); // Close all connections

        using HealthReportingCallback = std::function<void(
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & dynamicProperty,
            std::wstring const & description,
            Common::TimeSpan ttl)>;

        static void SetHealthReportingCallback(HealthReportingCallback && callback);
        static void RerportHealth(
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & dynamicProperty,
            std::wstring const & description,
            Common::TimeSpan ttl = Common::TimeSpan::MaxValue);

        // TODO: FLOW CONTROL
        // Currently there is no feedback from SendOneWay to know when a send has
        // completed.

    private:
        virtual ISendTarget::SPtr Resolve(
            std::wstring const & address,
            std::wstring const & targetId,
            std::wstring const & sspiTarget,
            uint64 instance) = 0;

        friend class UnreliableTransport;
    };
}

#define TraceTransport TextTracePtrAs(this, IDatagramTransport)

