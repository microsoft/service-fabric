// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class HealthReportReplica
        {

        public:
            explicit HealthReportReplica(const Replica &r);

            bool operator == (HealthReportReplica const & rhs) const;
            bool operator != (HealthReportReplica const & rhs) const;
            wstring GetStringRepresentation() const;
            int64 Test_GetReplicaID() const { return replicaId_; }

        private:
            ReplicaRole::Enum currentRole_;
            ReplicaRole::Enum previousRole_;
            ::FABRIC_QUERY_SERVICE_REPLICA_STATUS queryStatus_;
            int64 replicaId_;
            Federation::NodeId nodeId_;
        };
    }
}
