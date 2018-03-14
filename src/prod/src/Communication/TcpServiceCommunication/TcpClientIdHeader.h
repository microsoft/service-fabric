// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class TcpClientIdHeader : public Transport::MessageHeader<Transport::MessageHeaderId::TcpClientIdHeader>, public Serialization::FabricSerializable
        {
            DENY_COPY_ASSIGNMENT(TcpClientIdHeader);

        public:
            TcpClientIdHeader() = default;
            TcpClientIdHeader(std::wstring const & clientId)
                :clientId_(clientId)
            {
            }

            TcpClientIdHeader(TcpClientIdHeader && rhs)
                :TcpClientIdHeader(rhs.ClientId)
            {
            }

            TcpClientIdHeader & operator=(TcpClientIdHeader && rhs)
            {
                if (this != &rhs)
                {
                    this->clientId_ = rhs.ClientId;
                }
                return *this;
            }

            __declspec(property(get = getClientId)) std::wstring const & ClientId;
            std::wstring const & getClientId() const { return clientId_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &)
            {
                w.Write("{0}", clientId_);

            }

            FABRIC_FIELDS_01(clientId_);

        private:
            std::wstring clientId_;
        };
    }
}
