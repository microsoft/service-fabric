// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        struct InduceFaultMessageBody : public Serialization::FabricSerializable
        {
        public:
            InduceFaultMessageBody() {}
            InduceFaultMessageBody(
                FABRIC_REPLICA_ID targetReplicaId,
                Common::Guid const & targetReplicaIncarnationId,
                std::wstring const & reason) 
                : targetReplicaId_(targetReplicaId)
                , targetReplicaIncarnationId_(targetReplicaIncarnationId)
                , reason_(reason)
            {
            }

            ~InduceFaultMessageBody() {}

            __declspec(property(get=get_Reason)) std::wstring Reason;
            std::wstring get_Reason() const { return reason_; }

            __declspec(property(get=get_TargetReplicaId)) FABRIC_REPLICA_ID TargetReplicaId;
            FABRIC_REPLICA_ID get_TargetReplicaId() const { return targetReplicaId_; }

            __declspec(property(get=get_TargetReplicaIncarnationId)) Common::Guid TargetReplicaIncarnationId;
            Common::Guid get_TargetReplicaIncarnationId() const { return targetReplicaIncarnationId_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
            {
                w << targetReplicaId_ << ";" << targetReplicaIncarnationId_ << ";" << reason_;
            }

            FABRIC_FIELDS_03(targetReplicaId_, targetReplicaIncarnationId_, reason_);

        private:
            FABRIC_REPLICA_ID targetReplicaId_;
            Common::Guid targetReplicaIncarnationId_;
            std::wstring reason_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
