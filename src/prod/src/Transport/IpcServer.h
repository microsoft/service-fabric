// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcServer : public Common::FabricComponent, public Common::TextTraceComponent<Common::TraceTaskCodes::IPC>
    {
        DENY_COPY(IpcServer);

    public:
        IpcServer(
            Common::ComponentRoot const & root,
            std::wstring const & transportListenAddress,
            std::wstring const & transportListenAddressTls,
            std::wstring const & serverId,
            bool useUnreliableTransport,
            std::wstring const & owner);

        IpcServer(
            Common::ComponentRoot const & root,
            std::wstring const & transportListenAddress,
            std::wstring const & serverId,
            bool useUnreliableTransport,
            std::wstring const & owner);

        ~IpcServer(); // disable clang generated destructor, which somehow requires ClientTable declaration 

        void RegisterMessageHandler(Actor::Enum actor, IpcMessageHandler const & messageHandler, bool dispatchOnTransportThread);

        void UnregisterMessageHandler(Actor::Enum actor);

        Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && request,
            std::wstring const & client,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        Common::ErrorCode EndRequest(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);

        Common::ErrorCode SendOneWay(
            std::wstring const & client, Transport::MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue);

        void RemoveClient(std::wstring const & client);

        // Read Properties
        __declspec(property(get=get_transportListenAddress)) std::wstring const & TransportListenAddress;
        inline std::wstring const & get_transportListenAddress() const { return localUnit_.listenAddress_; };
        __declspec(property(get=get_SecuritySettings)) Transport::SecuritySettings const & SecuritySettings;
        Transport::SecuritySettings const & get_SecuritySettings() const;
        Common::ErrorCode SetSecurity(Transport::SecuritySettings const & value); 

        bool IsTlsListenerEnabled() const { return tlsUnit_ != nullptr;}
        __declspec(property(get = get_transportListenAddressTls)) std::wstring const & TransportListenAddressTls;
        inline std::wstring const & get_transportListenAddressTls() const
        {
            Invariant(tlsUnit_);
            return tlsUnit_->listenAddress_;
        };

        __declspec(property(get = get_SecuritySettingsTls)) Transport::SecuritySettings const & SecuritySettingsTls;
        Transport::SecuritySettings const & get_SecuritySettingsTls() const;
        Common::ErrorCode SetSecurityTls(Transport::SecuritySettings const & value);
        
        __declspec(property(get = get_ServerCertThumbprintTls)) Common::Thumbprint const & ServerCertThumbprintTls;
        Common::Thumbprint const & get_ServerCertThumbprintTls() const;

        void SetMaxIncomingMessageSize(uint value);
        void DisableIncomingMessageSizeLimit();

        Common::HHandler RegisterTlsSecuritySettingsUpdatedEvent(Common::EventHandler const & handler);
        bool UnRegisterTlsSecuritySettingsUpdatedEvent(Common::HHandler hHandler);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        void OnSending(Transport::MessageUPtr & message) const;
        class ClientTable;
        void OnReconnectMessageReceived(Transport::MessageUPtr & message, IpcReceiverContextUPtr & receiverContext, ClientTable & clientTable);
        void Cleanup();
        void OnTlsSecuritySettingsUpdated();

        const std::wstring serverId_;
        const std::wstring traceId_;

        struct TransportUnit
        {
            TransportUnit(
                IpcServer* ipcServer,
                Common::ComponentRoot const & root,
                std::wstring const & listenAddress,
                std::wstring const & serverId,
                std::wstring const & owner,
                std::wstring const & traceId,
                bool useUnreliableTransport);

            Common::ErrorCode Open();
            void Close();

            IpcServer* const ipcServer_;
            std::wstring listenAddress_;
            IDatagramTransportSPtr transport_;
            IpcDemuxer demuxer_;
            RequestReply requestReply_;
            std::unique_ptr<ClientTable> clientTable_;
        };

        TransportUnit localUnit_;
        std::unique_ptr<TransportUnit> tlsUnit_;
        Common::Event tlsSecurityUpdatedEvent_;
    };

    typedef std::shared_ptr<IpcServer> IpcServerSPtr;
    typedef std::unique_ptr<IpcServer> IpcServerUPtr;
}
