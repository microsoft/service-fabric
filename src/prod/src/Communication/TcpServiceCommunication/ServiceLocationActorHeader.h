// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {

        class ServiceLocationActorHeader : public Transport::MessageHeader<Transport::MessageHeaderId::ServiceLocationActor>,
            public Serialization::FabricSerializable
        {
            DENY_COPY_ASSIGNMENT(ServiceLocationActorHeader);
            DEFAULT_COPY_CONSTRUCTOR(ServiceLocationActorHeader);
        public:

            ServiceLocationActorHeader() {}

            ServiceLocationActorHeader(std::wstring const & servicePath) :
                servicePath_(servicePath)
            {
            }

            ServiceLocationActorHeader & operator=(ServiceLocationActorHeader && rhs)
            {
                if (this != &rhs)
                {
                    this->servicePath_ = rhs.Actor;
                }
                return *this;
            }

            __declspec(property(get = get_Actor)) std::wstring const & Actor;
            std::wstring  const & get_Actor() const { return servicePath_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w.Write("{0}", servicePath_);
            }

            FABRIC_FIELDS_01(servicePath_);

        private:
            std::wstring servicePath_;
        };
    }
}
