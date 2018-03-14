// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationTransport
            : public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
            , public Common::ComponentRoot
            , public Api::IServiceCommunicationTransport
        {
            DENY_COPY(ServiceCommunicationTransport);

        public:
            static Common::ErrorCode Create(ServiceCommunicationTransportSettingsUPtr  && settings,
                                            std::wstring const & address,
                                            Api::IServiceCommunicationTransportSPtr & transport);

            virtual Common::ErrorCode RegisterCommunicationListener(
                Communication::TcpServiceCommunication::ServiceMethodCallDispatcherSPtr const & dispatcher,
                __out std::wstring &endpointAddress);

            virtual  Common::AsyncOperationSPtr BeginUnregisterCommunicationListener(
                std::wstring const & servicePath,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndUnregisterCommunicationListener(
                Common::AsyncOperationSPtr const & asyncOperation);

            virtual Common::AsyncOperationSPtr BeginRequest(
                Transport::MessageUPtr && message,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndRequest(
                Common::AsyncOperationSPtr const &  operation,
                Transport::MessageUPtr & reply);

            virtual Common::ErrorCode SendOneWay(
                Transport::MessageUPtr && message);

            virtual  Common::ErrorCode  Open();
            virtual Common::ErrorCode  Close();

            virtual ServiceCommunicationTransportSettings const& GetSettings();
            
            bool IsClosed();

            ~ServiceCommunicationTransport();
        private:
            ServiceCommunicationTransport(
                ServiceCommunicationTransportSettingsUPtr && securitySettings,
                std::wstring const & address);
            std::wstring GetServiceLocation(std::wstring const & location);

            Common::ErrorCode GenerateEndpoint(std::wstring const & location, __out std::wstring & endpointAddress);
           void ProcessRequest(Transport::MessageUPtr &&, Transport::ReceiverContextUPtr &&);
           void OnConnectComplete(Common::AsyncOperationSPtr const & operation,
                                   bool expectedCompletedSynchronously);
            void OnDisconnectComplete(Common::AsyncOperationSPtr const & operation,
                                     bool expectedCompletedSynchronously);
            void Cleanup();
            Common::ErrorCode CloseCallerHoldingLock();
            bool IsClosedCallerHoldingLock();
            void OnDisconnect(Transport::ISendTarget const & sendTarget);
            void OnConnect(std::wstring clientId, ServiceMethodCallDispatcherSPtr dispatcher);
            Transport::ISendTarget::SPtr GetSendTarget(Transport::MessageUPtr const & message);
            void CallDisconnectHandlerIfExists(ServiceMethodCallDispatcherSPtr const & dispatcher,
                                               ClientConnectionStateInfoSPtr & connectionStateInfo);
            ClientConnectionStateInfoSPtr CallConnectHandlerIfItsNewClient(std::wstring const & clientId,
                                                                           ServiceMethodCallDispatcherSPtr const & dispatcher,
                                                                           bool newClient);
            void DisconnectClients(std::vector<ClientConnectionStateInfoSPtr> clientInfo,
                                                                              ServiceMethodCallDispatcherSPtr const & dispatcher);

                                          
            std::wstring listenAddress_;
            std::wstring  traceId_;
            Transport::IDatagramTransportSPtr transport_;
            Transport::DemuxerUPtr demuxer_;
            ServiceCommunicationMessageHandlerCollectionUPtr messageHandlerCollection_;
            Transport::RequestReply requestReply_;
            Common::TimeSpan defaultTimeout_;
          
            class OnConnectAsyncOperation;
            class OnDisconnectAsyncOperation;
            class UnRegisterAsyncOperation;
            class DisconnectClientsAsyncOperation;
            class ClientTable;
            std::unique_ptr<ClientTable> clientTable_;
            ServiceCommunicationTransportSettingsUPtr settings_;
            uint64 openListenerCount_;
            bool isClosed_;
            Common::RwLock lock_;
            Common::RwLock clientTablelock_;

        };
    }
}
