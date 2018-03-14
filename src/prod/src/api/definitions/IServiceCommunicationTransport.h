// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceCommunicationTransport : public Api::ICommunicationMessageSender
    {

    public:

        virtual Common::ErrorCode RegisterCommunicationListener(
            Communication::TcpServiceCommunication::ServiceMethodCallDispatcherSPtr const & dispatcher,
            __out std::wstring &endpointAddress)=0;
            
        virtual  Common::AsyncOperationSPtr BeginUnregisterCommunicationListener(
            std::wstring const & servicePath,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)=0;

        virtual Common::ErrorCode EndUnregisterCommunicationListener(
            Common::AsyncOperationSPtr const & asyncOperation)=0;

        virtual Communication::TcpServiceCommunication::ServiceCommunicationTransportSettings const& GetSettings() = 0;
  
        virtual Common::ErrorCode  Open() = 0;
        virtual Common::ErrorCode  Close() = 0;

        virtual bool IsClosed()=0;

        virtual ~IServiceCommunicationTransport() {};
    };
}
