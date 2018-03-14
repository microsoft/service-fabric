// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class OperationAckHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::OperationAck>
            , public Serialization::FabricSerializable
        {
        public:
            OperationAckHeader() {}

            OperationAckHeader(
                Common::Guid const & incarnationId)
                : incarnationId_(incarnationId)
            {
            }

            __declspec(property(get=get_IncarnationId)) Common::Guid const & IncarnationId;
            Common::Guid const & get_IncarnationId() const { return incarnationId_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write("IncarnationId:{0}", incarnationId_);
            }

            FABRIC_FIELDS_01(incarnationId_);

        private:
            Common::Guid incarnationId_;
        };
    }
}
