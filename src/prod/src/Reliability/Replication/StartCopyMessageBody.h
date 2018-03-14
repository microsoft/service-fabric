// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        struct StartCopyMessageBody : public Serialization::FabricSerializable
        {
        public:
            StartCopyMessageBody() {}
            StartCopyMessageBody(
                FABRIC_EPOCH const & epoch,
                FABRIC_REPLICA_ID replicaId,
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber) 
                :   epoch_(epoch),
                    replicaId_(replicaId),
                    replicationStartSequenceNumber_(replicationStartSequenceNumber)
            {
            }

            ~StartCopyMessageBody() {}

            __declspec(property(get=get_ReplicationStartSequenceNumber)) FABRIC_SEQUENCE_NUMBER ReplicationStartSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_ReplicationStartSequenceNumber() const { return replicationStartSequenceNumber_; }

            __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec(property(get=get_Epoch)) FABRIC_EPOCH const & Epoch;
            FABRIC_EPOCH const & get_Epoch() const { return epoch_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
            {
                w << epoch_ << ";" << replicaId_ << ";" << replicationStartSequenceNumber_;
            }

            FABRIC_FIELDS_03(epoch_, replicaId_, replicationStartSequenceNumber_);

        private:
            FABRIC_EPOCH epoch_;
            FABRIC_REPLICA_ID replicaId_;
            FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
