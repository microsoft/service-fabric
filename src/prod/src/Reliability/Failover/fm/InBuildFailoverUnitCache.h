// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class InBuildFailoverUnitCache : public Common::RootedObject
        {
            DENY_COPY(InBuildFailoverUnitCache);

        public:
            InBuildFailoverUnitCache(
                FailoverManager& fm,
                std::vector<InBuildFailoverUnitUPtr> && inBuildFailoverUnits,
                Common::ComponentRoot const& root);

            __declspec(property(get=get_Count)) size_t Count;
            size_t get_Count() const;

            __declspec(property(get=get_AnyReplicaFound, put=set_AnyReplicaFound)) bool AnyReplicaFound;
            bool get_AnyReplicaFound() const { return anyReplicaFound_; }
            void set_AnyReplicaFound(bool value) { anyReplicaFound_ = value; }

            void InitializeHealthSequence(__inout FABRIC_SEQUENCE_NUMBER & healthSequence) const;

            void OnStartRebuild();
            Common::ErrorCode OnRebuildComplete();
            Common::ErrorCode AddFailoverUnitReport(FailoverUnitInfo const & report, Federation::NodeInstance const & nodeInstance);

            bool InBuildFailoverUnitExists(FailoverUnitId const& failoverUnitId) const;
            bool InBuildFailoverUnitExistsForService(std::wstring const & serviceName) const;
            bool InBuildReplicaExistsForNode(Federation::NodeId const& nodeId) const;

            Common::ErrorCode OnReplicaDropped(FailoverUnitId const& failoverUnitId, Federation::NodeId const& nodeId, bool isDeleted);

            Common::ErrorCode RecoverPartitions();
            Common::ErrorCode RecoverPartition(FailoverUnitId const& failoverUnitId);
            Common::ErrorCode RecoverServicePartitions(std::wstring const& serviceName);
            Common::ErrorCode RecoverSystemPartitions();

            Common::ErrorCode DeleteInBuildFailoverUnitsForService(std::wstring const& serviceName);
            Common::ErrorCode MarkInBuildFailoverUnitsToBeDeletedForService(std::wstring const& serviceName);

            void CheckQuorumLoss();

            void GetInBuildFailoverUnits(std::vector<InBuildFailoverUnitUPtr> & result) const;

            void Dispose();

        private:

            Common::ErrorCode CreateServiceIfNotPresent(FailoverUnitId const & failoverUnitId, InBuildFailoverUnitUPtr const& inBuildFailoverUnitUPtr, ServiceDescription const& serviceDescription);

            Common::ErrorCode ProcessFailoverUnits(bool shouldDropOfflineReplicas, std::vector<StateMachineActionUPtr> & actions);

            bool UpdateServiceInstance(std::wstring const & name, uint64 instance);

            Common::ErrorCode RecoverPartition(InBuildFailoverUnitUPtr & inBuildFailoverUnit);

            bool RecoverFMServicePartition();

            // Should be called under a lock
            Common::ErrorCode PersistGeneratedFailoverUnit(
                FailoverUnitUPtr && failoverUnit,
                InBuildFailoverUnitUPtr const& inBuildFailoverUnit,
                ServiceDescription const& serviceDescription);

            void SetFullRebuildTimer();

            void ReportHealth(InBuildFailoverUnit const& inBuildFailoverUnit, bool isDeleted, Common::ErrorCode error);

            FailoverManager& fm_;
            FailoverManagerStore& fmStore_;

            std::map<FailoverUnitId, InBuildFailoverUnitUPtr> inBuildFailoverUnits_;
            std::map<std::wstring, uint64> serviceInstances_;

            bool anyReplicaFound_;

            bool isRebuildComplete_;
            Common::TimerSPtr fullRebuildTimer_;

            MUTABLE_RWLOCK(FM.InBuildFailoverUnitCache, lock_);
        };
    }
}
