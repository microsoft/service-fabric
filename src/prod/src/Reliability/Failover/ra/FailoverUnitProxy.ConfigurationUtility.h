// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitProxy::ConfigurationUtility
        {
        public:
            bool IsConfigurationMessageBodyStale(
                int64 localReplicaId,
                std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
                ConfigurationReplicaStore const & configurationReplicas) const;

            bool CheckConfigurationMessageBodyForUpdates(
                int64 localReplicaId,
                std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
                bool shouldApply, 
                ConfigurationReplicaStore & configurationReplicas) const;

            void GetReplicaCountForPCAndCC(
                std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
                int & ccCount,
                int & ccNonDroppedCount,
                int & pcCount,
                int & pcNonDroppedCount) const;

            void CreateReplicatorConfigurationList(
                ConfigurationReplicaStore const & store,
                bool isPrimaryChangeBetweenPCAndCC,
                bool isUpdateCatchupConfiguration,
                Common::ScopedHeap & heap,
                __out std::vector<::FABRIC_REPLICA_INFORMATION> & cc,
                __out std::vector<::FABRIC_REPLICA_INFORMATION> & pc,
                __out std::vector<Reliability::ReplicaDescription const *> & tombstoneReplicas) const;

            FABRIC_REPLICA_INFORMATION CreateReplicaInformation(
                Common::ScopedHeap & heap,
                Reliability::ReplicaDescription const & description,
                ReplicaRole::Enum role,
                bool setProgress,
                bool mustCatchup) const;

        private:
            bool TryAddCCReplicaToList(
                Common::ScopedHeap & heap,
                Reliability::ReplicaDescription const & description,
                bool isPrimaryChangeBetweenPCAndCC,
                bool setMustCatchupFlagIfRoleIsPrimary,
                __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const;

            bool TryAddPCReplicaToList(
                Common::ScopedHeap & heap,
                Reliability::ReplicaDescription const & description,
                bool isPrimaryChangeBetweenPCAndCC,
                bool setMustCatchupFlagIfRoleIsPrimary,
                __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const;

            bool TryAddReplicaToList(
                Common::ScopedHeap & heap,
                Reliability::ReplicaDescription const & description,
                ReplicaRole::Enum role,
                bool setMustCatchupFlagIfRoleIsPrimary,
                __inout std::vector<::FABRIC_REPLICA_INFORMATION> & v) const;
        };
    }
}
