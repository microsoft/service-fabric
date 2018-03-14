// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        struct ReplicationAckMessageBody : public Serialization::FabricSerializable
        {
        public:
            ReplicationAckMessageBody() {}
            ReplicationAckMessageBody(
                FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
                FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
                FABRIC_SEQUENCE_NUMBER copyReceivedLSN = Constants::NonInitializedLSN,
                FABRIC_SEQUENCE_NUMBER copyQuorumLSN = Constants::NonInitializedLSN) 
                :   replicationReceivedLSN_(replicationReceivedLSN), 
                    replicationQuorumLSN_(replicationQuorumLSN),
                    copyReceivedLSN_(copyReceivedLSN),
                    copyQuorumLSN_(copyQuorumLSN)
            {
            }

            ~ReplicationAckMessageBody() {}

            __declspec (property(get=get_ReplicationReceivedLSN)) FABRIC_SEQUENCE_NUMBER ReplicationReceivedLSN;
            FABRIC_SEQUENCE_NUMBER get_ReplicationReceivedLSN() const { return replicationReceivedLSN_; }

            __declspec (property(get=get_ReplicationQuorumLSN)) FABRIC_SEQUENCE_NUMBER ReplicationQuorumLSN;
            FABRIC_SEQUENCE_NUMBER get_ReplicationQuorumLSN() const { return replicationQuorumLSN_; }

            __declspec (property(get=get_CopyReceivedLSN)) FABRIC_SEQUENCE_NUMBER CopyReceivedLSN;
            FABRIC_SEQUENCE_NUMBER get_CopyReceivedLSN() const { return copyReceivedLSN_; }

            __declspec (property(get=get_CopyQuorumLSN)) FABRIC_SEQUENCE_NUMBER CopyQuorumLSN;
            FABRIC_SEQUENCE_NUMBER get_CopyQuorumLSN() const { return copyQuorumLSN_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
            {
                w << replicationReceivedLSN_ << "," << replicationQuorumLSN_;
                if(copyReceivedLSN_ != Constants::NonInitializedLSN)
                {
                    w << ":" << copyReceivedLSN_ << "," << copyQuorumLSN_;
                }
            }

            FABRIC_FIELDS_04(replicationReceivedLSN_, replicationQuorumLSN_, copyReceivedLSN_, copyQuorumLSN_);

        private:
            FABRIC_SEQUENCE_NUMBER replicationReceivedLSN_;
            FABRIC_SEQUENCE_NUMBER replicationQuorumLSN_;
            FABRIC_SEQUENCE_NUMBER copyReceivedLSN_;
            FABRIC_SEQUENCE_NUMBER copyQuorumLSN_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
