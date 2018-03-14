// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {

        class ServiceCommunicationClientFactory :public Api::IServiceCommunicationClientFactory, public Common::ComponentRoot
        {
            DENY_COPY(ServiceCommunicationClientFactory);
        public:

            ServiceCommunicationClientFactory::ServiceCommunicationClientFactory() {};

            virtual ~ServiceCommunicationClientFactory() {};

            static Api::IServiceCommunicationClientFactoryPtr Create();

            virtual Common::ErrorCode CreateServiceCommunicationClient(
                std::wstring const & address,
                ServiceCommunicationTransportSettingsUPtr const & setting,
                Api::IServiceCommunicationMessageHandlerPtr const & notificationHandler,
                Api::IServiceConnectionEventHandlerPtr const & connectionEventHandler,
                __out Api::IServiceCommunicationClientPtr & result,
                bool explicitConnect = false);
        };
    }
}
