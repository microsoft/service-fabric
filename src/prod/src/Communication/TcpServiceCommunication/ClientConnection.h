// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ClientConnection : 
            public Api::IClientConnection, 
            public Common::ComponentRoot,
            public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {
            DENY_COPY(ClientConnection)

        public:
            ClientConnection(std::wstring const & clientId,
                             ServiceCommunicationTransportSPtr transport)
                             :clientId_(clientId),
                             transport_(transport)
            {
            }

            Common::AsyncOperationSPtr BeginRequest(
                Transport::MessageUPtr && message,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                TcpClientIdHeader header{ clientId_ };
                message->Headers.Add(header);
                return transport_->BeginRequest(move(message), timeout, callback, parent);
            }

            Common::ErrorCode EndRequest(
                Common::AsyncOperationSPtr const &  operation,
                Transport::MessageUPtr & reply)
            {
                return transport_->EndRequest(operation, reply);
            }

            Common::ErrorCode SendOneWay(
                Transport::MessageUPtr && message)
            {
                TcpClientIdHeader header{ clientId_ };
                message->Headers.Add(header);
                return transport_->SendOneWay(move(message));
            }

            std::wstring const & Get_ClientId() const
            {
                return clientId_;
            }

            ~ClientConnection()
            {
                WriteInfo("ClientConnection", "Destructing ClientConnection for Client {0}",this->clientId_);
            }
        private:
            std::wstring  clientId_;
            ServiceCommunicationTransportSPtr transport_;
        };
    }

}
