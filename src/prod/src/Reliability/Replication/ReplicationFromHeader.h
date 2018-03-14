// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicationFromHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::REFrom>
            , public Serialization::FabricSerializable
        {
        public:
            ReplicationFromHeader() {}

            ReplicationFromHeader(std::wstring const & address, ReplicationEndpointId const & demuxerActor) :
                address_(address), demuxerActor_(demuxerActor)
            {
            }

            ~ReplicationFromHeader() {}

            __declspec(property(get=get_Address)) std::wstring const & Address;
            std::wstring const & get_Address() const { return address_; }

            __declspec(property(get=get_DemuxerActor)) ReplicationEndpointId const & DemuxerActor;
            ReplicationEndpointId const & get_DemuxerActor() const { return demuxerActor_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write("{0}/{1}", address_, demuxerActor_);
            }

            FABRIC_FIELDS_02(address_, demuxerActor_);

        private:
            std::wstring address_;
            ReplicationEndpointId demuxerActor_;

        }; // end class ReplicationFromHeader

    } // end namespace ReplicationComponent
} // end namespace Reliability
