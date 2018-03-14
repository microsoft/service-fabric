// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceCommunicationListenerFactory
    {

    public:
        virtual ~IServiceCommunicationListenerFactory() {};

        virtual Common::ErrorCode CreateServiceCommunicationListener(
            Communication::TcpServiceCommunication::ServiceCommunicationTransportSettingsUPtr && setting,
            std::wstring const& address,
            std::wstring const& path,
            Api::IServiceCommunicationMessageHandlerPtr const & clientRequestHandler,
            Api::IServiceConnectionHandlerPtr const & clientConnectionHandler,
            __out Api::IServiceCommunicationListenerPtr & listener) =0;

    };
}
