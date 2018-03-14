// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationTransport;
        class ServiceCommunicationClient;
        class ServiceCommunicationListener;
        class ServiceCommunicationTransportSettings;
        class ServiceMethodCallDispatcher;
        class ServiceCommunicationMessageHandlerCollection;
        typedef std::shared_ptr<ServiceCommunicationTransport> ServiceCommunicationTransportSPtr;
        typedef std::shared_ptr<ServiceCommunicationClient> ServiceCommunicationClientSPtr;
        typedef std::shared_ptr<ServiceCommunicationListener> ServiceCommunicationListenerSPtr;
        typedef std::unique_ptr<ServiceCommunicationTransportSettings> ServiceCommunicationTransportSettingsUPtr;
        typedef std::shared_ptr<ServiceMethodCallDispatcher> ServiceMethodCallDispatcherSPtr;
        typedef std::unique_ptr<ServiceCommunicationMessageHandlerCollection>ServiceCommunicationMessageHandlerCollectionUPtr;
    }
}
