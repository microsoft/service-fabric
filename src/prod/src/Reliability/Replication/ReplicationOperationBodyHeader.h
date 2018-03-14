// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicationOperationBodyHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::ReplicationOperationBody>
            , public Serialization::FabricSerializable
        {
        public:
            ReplicationOperationBodyHeader() :
                replicationOperationBodyHeaderSize_(0)
            {}

            ReplicationOperationBodyHeader(
                ULONG replicationOperationBodyHeaderSize)
                : replicationOperationBodyHeaderSize_(replicationOperationBodyHeaderSize)
            {
            }

            __declspec(property(get = get_ReplicationOperationBodyHeaderSize)) ULONG ReplicationOperationBodyHeaderSize;
            ULONG get_ReplicationOperationBodyHeaderSize() const { return replicationOperationBodyHeaderSize_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w.Write("ReplicationOperationBodyHeaderSize = {0}", replicationOperationBodyHeaderSize_);
            }

            FABRIC_FIELDS_01(replicationOperationBodyHeaderSize_);

        private:
            ULONG replicationOperationBodyHeaderSize_;
        };
    }
}
