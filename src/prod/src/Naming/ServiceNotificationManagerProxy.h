// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // This class maintain's the information needed at EntreeServiceProxy for push based service notifications.
    //
    class ServiceNotificationManagerProxy
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Federation::NodeTraceComponent<Common::TraceTaskCodes::EntreeServiceProxy>        
    {
        DENY_COPY(ServiceNotificationManagerProxy)

    public:

        ServiceNotificationManagerProxy(
            Common::ComponentRoot const &root,
            Federation::NodeInstance const &instance,
            Transport::RequestReplySPtr const & requestReply,
            ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport,
            Transport::IDatagramTransportSPtr const &);

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr && message,
            Transport::ISendTarget::SPtr const &,
            Common::TimeSpan const&,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &);

        // Used by test code
        size_t GetTotalPendingTransportMessageCount(std::wstring const & clientId) const;

        void SetConnectionAcceptedHandler(
            Transport::IDatagramTransport::ConnectionAcceptedHandler const &);

        void SetConnectionFaultHandler(
            Transport::IDatagramTransport::ConnectionFaultHandler const &);

    protected:

        Common::ErrorCode OnOpen();

        Common::ErrorCode OnClose();

        void OnAbort();

    private:
        class NotificationAsyncOperationBase;
        class NotificationClientConnectAsyncOperation;
        class NotificationClientSyncAsyncOperation;
        class NotificationFilterRegistrationAsyncOperation;
        class NotificationFilterUnregistrationAsyncOperation;

        class ProcessNotificationAsyncOperation;

        void OnClientConnectionAccepted(Transport::ISendTarget const&);
        void OnClientConnectionFault(Transport::ISendTarget const&, Common::ErrorCode const&);

        Common::AsyncOperationSPtr BeginProcessNotification(
            Transport::MessageUPtr & message,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndProcessNotification(
            Common::AsyncOperationSPtr const &,
            _Out_ Transport::MessageUPtr & );

        //
        // Returns true for new registrations and false if the client identity already exists
        //
        bool TryAddOrUpdateClientRegistration(
            Transport::ISendTarget::SPtr const &,
            ClientIdentityHeader &&);

        bool TryGetClientRegistration(
            Transport::ISendTarget::SPtr const &, 
            std::wstring const &clientId,
            __out ClientIdentityHeader &);

        bool TryRemoveClientRegistration(
            Transport::ISendTarget const&,
            __out ClientIdentityHeader &);

        bool TryGetSendTarget(
            ClientIdentityHeader const &,
            __out Transport::ISendTarget::SPtr &); // TODO: returns a sptr copy, can this be optimized to return a ref?

        void NotifyClientDisconnection(ClientIdentityHeader &);
        void OnNotifyClientDisconnectionComplete(Common::AsyncOperationSPtr const &op);

        mutable Common::RwLock lock_;
        std::map<std::wstring, ClientIdentityHeader> sendTargetToClientRegistrationMap_;
        std::map<ClientIdentityHeader, Transport::ISendTarget::SPtr> clientRegistrationToSendTargetMap_;
        Transport::RequestReplySPtr const & requestReply_;
        ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport_;
        Transport::IDatagramTransportSPtr const &publicTransport_;

        // Forward connection events to owner from transport
        //
        Transport::IDatagramTransport::ConnectionAcceptedHandler connectionAcceptedHandler_;
        Transport::IDatagramTransport::ConnectionFaultHandler connectionFaultHandler_;

    };
}
