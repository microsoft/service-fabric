// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationListenerFactory
            : public Api::IServiceCommunicationListenerFactory,
              public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>,
              public Common::ComponentRoot
        {

        public:
            static Api::IServiceCommunicationListenerFactory& GetServiceCommunicationListenerFactory();

            virtual Common::ErrorCode CreateServiceCommunicationListener(
                Communication::TcpServiceCommunication::ServiceCommunicationTransportSettingsUPtr && setting,
                std::wstring const& address,
                std::wstring const& path,
                Api::IServiceCommunicationMessageHandlerPtr const & clientRequestHandler,
                Api::IServiceConnectionHandlerPtr const & clientConnectionHandler,
                __out Api::IServiceCommunicationListenerPtr & listener);

            ServiceCommunicationListenerFactory();
        private:
            static BOOL CALLBACK InitalizeCommunicationListenerFactory(PINIT_ONCE, PVOID, PVOID *);
            static INIT_ONCE initOnce_;
            static Common::Global<ServiceCommunicationListenerFactory> singletonComunicationListenerFactory_;

            Common::ErrorCode CreateServiceCommunicationTransportCallerHoldsLock(
                ServiceCommunicationTransportSettingsUPtr && setting,
                std::wstring const& address,
                __out Api::IServiceCommunicationTransportSPtr &transport);

            Common::ErrorCode CreateServiceCommunicationListenerInternal(
                std::wstring const& path,
                Api::IServiceCommunicationMessageHandlerPtr const& clientRequestHandler,
                Api::IServiceConnectionHandlerPtr const& clientConnectionHandler,
                Api::IServiceCommunicationTransportSPtr const& transport,
                __out Api::IServiceCommunicationListenerPtr & listener);

            std::map<std::wstring, Api::IServiceCommunicationTransportWPtr> knownTransports_;
            Common::RwLock knownTransportsMapLock_;
        };
    }
}

