// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceCommunicationClientFactory
    {

    public:
        virtual ~IServiceCommunicationClientFactory() {};

        virtual Common::ErrorCode CreateServiceCommunicationClient(
            std::wstring const & address,
            Communication::TcpServiceCommunication::ServiceCommunicationTransportSettingsUPtr const & setting,
            IServiceCommunicationMessageHandlerPtr const & notificationHandler,
            IServiceConnectionEventHandlerPtr const & connectionEventHandler,
            __out IServiceCommunicationClientPtr & result,
            bool explicitConnect=false) = 0;

    };
}
