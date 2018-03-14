// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicationActorHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::ReplicationActor>
            , public Serialization::FabricSerializable
        {
        public:
            ReplicationActorHeader() {}

            ReplicationActorHeader(ReplicationEndpointId const & actor)
                : actor_(actor)
            {
            }

            __declspec(property(get=get_Actor)) ReplicationEndpointId const & Actor;
            ReplicationEndpointId const & get_Actor() const { return actor_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write("{0}", actor_);
            }

            FABRIC_FIELDS_01(actor_);

        private:
            ReplicationEndpointId actor_;
        };
    }
}
