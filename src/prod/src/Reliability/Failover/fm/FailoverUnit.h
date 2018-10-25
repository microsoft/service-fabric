// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        /// <summary>
        /// The base class for stateful and stateless failoverUnits
        /// Asssume that the concurrent read and write on the same failoverUnit
        /// is under the same lock
        /// </summary>
        class FailoverUnit : public StoreData
        {
        public:
            // Empty constructor. Only used for deserialization.
            FailoverUnit(); 

            // Constructor for a new empty FailoverUnit.
            FailoverUnit(
                FailoverUnitId failoverUnitId,
                ConsistencyUnitDescription const& consistencyUnitDescription,
                bool isStateful,
                bool hasPersistedState,
                std::wstring const& serviceName,
                ServiceInfoSPtr const& serviceInfo);

            // Constructor for creating a FailoverUnit with specified values. Only used in unit tests.
            FailoverUnit(
                FailoverUnitId failoverUnitId,
                ConsistencyUnitDescription const& consistencyUnitDescription,
                ServiceInfoSPtr const& serviceInfo,
                FailoverUnitFlags::Flags flags,
                Reliability::Epoch previousConfigurationEpoch,
                Reliability::Epoch currentConfigurationEpoch,
                int64 lookupVersion,
                Common::DateTime lastUpdated,
                int64 updateVersion,
                int64 operationLSN,
                PersistenceState::Enum persistenceState);

            virtual ~FailoverUnit();

            // Constructor for making a deep copy of the given FailoverUnit.
            explicit FailoverUnit(FailoverUnit const& other);

            FailoverUnit(FailoverUnit && other);

            FailoverUnit & operator=(FailoverUnit && other);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            __declspec (property(get=get_Id)) FailoverUnitId const& Id;
            FailoverUnitId const& get_Id() const { return failoverUnitDesc_.FailoverUnitId; }

            __declspec (property(get=get_IdString)) std::wstring const& IdString;
            std::wstring const& get_IdString() const;

            __declspec (property(get = get_ExtraDescription)) std::wstring const & ExtraDescription;
            std::wstring const& get_ExtraDescription() const { return extraDescription_; }

            __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const& FailoverUnitDescription;
            Reliability::FailoverUnitDescription const& get_FailoverUnitDescription() const { return failoverUnitDesc_; }

            __declspec (property(get=get_TargetReplicaSetSize, put=set_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const;
            void set_TargetReplicaSetSize(int value);

            __declspec (property(get=get_MinReplicaSetSize)) int MinReplicaSetSize;
            int get_MinReplicaSetSize() const { return serviceInfo_->ServiceDescription.MinReplicaSetSize; }

            __declspec (property(get = get_Flags)) FailoverUnitFlags::Flags FailoverUnitFlags;
            FailoverUnitFlags::Flags get_Flags() const { return flags_; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return flags_.IsStateful(); }

            __declspec(property(get = get_IsVolatile)) bool IsVolatile;
            bool get_IsVolatile() const { return (IsStateful && !HasPersistedState); }

            __declspec (property(get=get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const { return flags_.HasPersistedState(); }

            __declspec (property(get=get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }

            __declspec (property(get=get_ServiceInfo, put=set_ServiceInfo)) ServiceInfoSPtr & ServiceInfoObj;
            ServiceInfoSPtr const & get_ServiceInfo() const { return serviceInfo_; }
            void set_ServiceInfo(ServiceInfoSPtr const& serviceInfo) const { serviceInfo_ = serviceInfo; }

            __declspec (property(get=get_IsToBeDeleted)) bool IsToBeDeleted;
            bool get_IsToBeDeleted() const { return flags_.IsToBeDeleted(); }
            void SetToBeDeleted();

            __declspec (property(get=get_IsSwappingPrimary, put=set_IsSwappingPrimary)) bool IsSwappingPrimary;
            bool get_IsSwappingPrimary() const { return flags_.IsSwappingPrimary(); }
            void set_IsSwappingPrimary(bool value);

            __declspec (property(get=get_IsOrphaned)) bool IsOrphaned;
            bool get_IsOrphaned() const { return flags_.IsOrphaned(); }
            void SetOrphaned(FailoverManager & fm);

            __declspec (property(get=get_IsUpgrading, put=set_IsUpgrading)) bool IsUpgrading;
            bool get_IsUpgrading() const { return flags_.IsUpgrading(); }
            void set_IsUpgrading(bool value);

            __declspec (property(get=get_ToBePromotedReplicaExists)) bool ToBePromotedReplicaExists;
            bool get_ToBePromotedReplicaExists() const;

            __declspec (property(get=get_ToBePromotedSecondaryExists)) bool ToBePromotedSecondaryExists;
            bool get_ToBePromotedSecondaryExists() const;

            __declspec (property(get=get_PreferredPrimaryLocationExists)) bool PreferredPrimaryLocationExists;
            bool get_PreferredPrimaryLocationExists() const;

            __declspec (property(get=get_NoData, put=set_NoData)) bool NoData;
            bool get_NoData() const { return flags_.IsNoData(); }
            void set_NoData(bool value);

            __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
            size_t get_ReplicaCount() const { return replicas_.size(); }

            __declspec (property(get=get_UpReplicaCount)) size_t UpReplicaCount;
            size_t get_UpReplicaCount() const;

            __declspec (property(get = get_AvailableReplicaCount)) size_t AvailableReplicaCount;
            size_t get_AvailableReplicaCount() const;

            __declspec (property(get=get_InBuildReplicaCount)) size_t InBuildReplicaCount;
            size_t get_InBuildReplicaCount() const;

            __declspec (property(get=get_IdleReadyReplicaCount)) size_t IdleReadyReplicaCount;
            size_t get_IdleReadyReplicaCount() const;

            __declspec (property(get=get_PotentialReplicaCount)) size_t PotentialReplicaCount;
            size_t get_PotentialReplicaCount() const;

            __declspec (property(get=get_OfflineReplicaCount)) size_t OfflineReplicaCount;
            size_t get_OfflineReplicaCount() const;

            __declspec (property(get=get_StandByReplicaCount)) size_t StandByReplicaCount;
            size_t get_StandByReplicaCount() const;

            __declspec (property(get=get_DroppedReplicaCount)) size_t DroppedReplicaCount;
            size_t get_DroppedReplicaCount() const;

            __declspec (property(get=get_DeletedReplicaCount)) size_t DeletedReplicaCount;
            size_t get_DeletedReplicaCount() const;

            __declspec (property(get=get_PreviousConfigurationEpoch, put=set_PreviousConfigurationEpoch)) Reliability::Epoch PreviousConfigurationEpoch;
            Reliability::Epoch const & get_PreviousConfigurationEpoch() const { return failoverUnitDesc_.PreviousConfigurationEpoch; }
            void set_PreviousConfigurationEpoch(Reliability::Epoch const& value) { failoverUnitDesc_.PreviousConfigurationEpoch = value; }

            __declspec (property(get=get_CurrentConfigurationEpoch, put=set_CurrentConfigurationEpoch)) Reliability::Epoch CurrentConfigurationEpoch;
            Reliability::Epoch const & get_CurrentConfigurationEpoch() const { return failoverUnitDesc_.CurrentConfigurationEpoch; }
            void set_CurrentConfigurationEpoch(Reliability::Epoch const& value) { failoverUnitDesc_.CurrentConfigurationEpoch = value; }

            __declspec (property(get=get_PreviousConfigurationVersion)) int64 PreviousConfigurationVersion;
            int64 get_PreviousConfigurationVersion() const { return failoverUnitDesc_.PreviousConfigurationEpoch.ConfigurationVersion; }

            __declspec (property(get=get_CurrentConfigurationVersion)) int64 CurrentConfigurationVersion;
            int64 get_CurrentConfigurationVersion() const { return failoverUnitDesc_.CurrentConfigurationEpoch.ConfigurationVersion; }

            __declspec (property(get=get_PreviousConfiguration)) FailoverUnitConfiguration const& PreviousConfiguration;
            FailoverUnitConfiguration const& get_PreviousConfiguration() const;

            __declspec (property(get=get_CurrentConfiguration)) FailoverUnitConfiguration const& CurrentConfiguration;
            FailoverUnitConfiguration const& get_CurrentConfiguration() const;

            __declspec (property(get=get_Primary)) ReplicaIterator Primary;
            ReplicaConstIterator get_Primary() const { return CurrentConfiguration.Primary; } // the const version
            ReplicaIterator get_Primary() { return ConvertIterator(CurrentConfiguration.Primary); } // non-const version

            __declspec (property(get=get_ReconfigurationPrimary)) ReplicaIterator ReconfigurationPrimary;
            ReplicaConstIterator get_ReconfigurationPrimary() const;
            ReplicaIterator get_ReconfigurationPrimary();

            bool IsReconfigurationPrimaryActive() const;
            bool IsReconfigurationPrimaryAvailable() const;

            __declspec (property(get=get_BeginIterator)) ReplicaIterator BeginIterator;
            ReplicaConstIterator get_BeginIterator() const { return replicas_.begin(); } // the const version
            ReplicaIterator get_BeginIterator() { return replicas_.begin(); } // non-const version

            __declspec (property(get=get_EndIterator)) ReplicaIterator EndIterator;
            ReplicaConstIterator get_EndIterator() const { return replicas_.end(); } // the const version
            ReplicaIterator get_EndIterator() { return replicas_.end(); } // non-const version

            __declspec (property(get=get_IsCreatingPrimary)) bool IsCreatingPrimary;
            bool get_IsCreatingPrimary() const;

            __declspec (property(get=get_IsDroppingPrimary)) bool IsDroppingPrimary;
            bool get_IsDroppingPrimary() const;

            __declspec (property(get=get_IsChangingConfiguration)) bool IsChangingConfiguration;
            bool get_IsChangingConfiguration() const { return (PreviousConfigurationVersion != 0); }

            __declspec (property(get=get_IsInReconfiguration)) bool IsInReconfiguration;
            bool get_IsInReconfiguration() const { return IsChangingConfiguration && !IsCreatingPrimary && !IsDroppingPrimary; }

            __declspec (property(get=get_IsUnhealthy)) bool IsUnhealthy;
            bool get_IsUnhealthy() const;

            __declspec (property(get=get_IsStable)) bool IsStable;
            bool get_IsStable() const;

            __declspec (property(get=get_PartitionStatus)) FABRIC_QUERY_SERVICE_PARTITION_STATUS PartitionStatus;
            FABRIC_QUERY_SERVICE_PARTITION_STATUS get_PartitionStatus() const;

            __declspec (property(get=get_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { return lastUpdated_; }
            Common::StopwatchTime GetUpdateTime() const;

            __declspec (property(get=get_LookupVersion, put=set_LookupVersion)) int64 LookupVersion;
            int64 get_LookupVersion() const { return lookupVersion_; }
            void set_LookupVersion(int64 value) { lookupVersion_ = value; }

            __declspec (property(get=get_UpdateVersion)) int64 UpdateVersion;
            int64 get_UpdateVersion() const { return updateVersion_; }

            __declspec(property(get = get_IsPersistencePending, put=set_IsPersistencePending)) bool IsPersistencePending;
            bool get_IsPersistencePending() const { return isPersistencePending_; }
            void set_IsPersistencePending(bool value) { isPersistencePending_ = value; }

            __declspec (property(get=get_HealthSequence, put=set_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const { return healthSequence_; }
            void set_HealthSequence(FABRIC_SEQUENCE_NUMBER value) { healthSequence_ = value; }

            __declspec (property(get=get_CurrentHealthState)) FailoverUnitHealthState::Enum CurrentHealthState;
            FailoverUnitHealthState::Enum get_CurrentHealthState() const { return currentHealthState_; }

            __declspec (property(get=get_UpdateForHealthReport, put=set_UpdateForHealthReport)) bool UpdateForHealthReport;
            bool get_UpdateForHealthReport() const { return updateForHealthReport_; }
            void set_UpdateForHealthReport(bool value);

            virtual void set_PersistenceState(PersistenceState::Enum value);

            __declspec (property(get=get_ReplicaDifference, put=set_ReplicaDifference)) int ReplicaDifference;
            int get_ReplicaDifference() const { return replicaDifference_; }
            void set_ReplicaDifference(int value) { replicaDifference_ = value; }

            __declspec (property(get=get_LastQuorumLossDuration)) Common::TimeSpan LastQuorumLossDuration;
            Common::TimeSpan get_LastQuorumLossDuration() const;
            void SetQuorumLossTime(int64 ticks);

            __declspec(property(get=get_StartProcessingTime, put=set_StartProcessingTime)) Common::StopwatchTime ProcessingStartTime;
            Common::StopwatchTime get_StartProcessingTime() const { return processingStartTime_; }
            void set_StartProcessingTime(Common::StopwatchTime value) { processingStartTime_ = value; }

            __declspec(property(get=get_PlacementStartTime, put=set_PlacementStartTime)) Common::DateTime PlacementStartTime;
            Common::DateTime get_PlacementStartTime() const { return placementStartTime_; }
            void set_PlacementStartTime(Common::DateTime value) { placementStartTime_ = value; }

            __declspec(property(get=get_ReconfigurationStartTime)) Common::DateTime ReconfigurationStartTime;
            Common::DateTime get_ReconfigurationStartTime() const { return reconfigurationStartTime_; }
			void Test_SetReconfigurationStartTime(Common::DateTime time) { reconfigurationStartTime_ = time; }

            __declspec(property(get=get_IsPlacementNeeded)) bool IsPlacementNeeded;
            bool get_IsPlacementNeeded() const;

            __declspec(property(get = get_HasPendingUpgradeOrDeactivateNodeReplica)) bool HasPendingUpgradeOrDeactivateNodeReplica;
            bool get_HasPendingUpgradeOrDeactivateNodeReplica() const;

			int GetInitialReplicaPlacementCount() const;

            bool IsBelowMinReplicaSetSize() const;

            bool IsReplicaMoveNeededDuringUpgrade(Replica const& replica) const;

            bool IsReplicaMoveNeededDuringDeactivateNode(Replica const& replica) const;

            void TraceState() const;

            FABRIC_FIELDS_13(
                failoverUnitDesc_,
                serviceName_,
                flags_,
                lookupVersion_,
                lastUpdated_,
                replicas_,
                updateVersion_,
                healthSequence_,
                currentHealthState_,
                extraDescription_,
                reconfigurationStartTime_,
                placementStartTime_,
                lastQuorumLossTime_);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            // get a specific replica
            Replica const* GetReplica(Federation::NodeId id) const;
            Replica * GetReplica(Federation::NodeId id);
            ReplicaIterator GetReplicaIterator(Federation::NodeId id);
            ReplicaConstIterator GetReplicaIterator(Federation::NodeId id) const;

            // convert const iterator to non-const iterator
            ReplicaIterator ConvertIterator(ReplicaConstIterator it) { return replicas_.begin() + (it - replicas_.cbegin()); }

            // iterate and process all replicas
            void ForEachReplica(std::function<void(Replica &)> processor);
            void ForEachReplica(std::function<void(Replica const&)> processor) const;

            // iterate replicas that is not in dropped states
            void ForEachUpReplica(std::function<void(Replica &)> processor);
            void ForEachUpReplica(std::function<void(Replica const&)> processor) const;

            // create an empty replica
            ReplicaIterator CreateReplica(NodeInfoSPtr && node);

            // Add an existing replica obtained from rebuild.
            void AddReplica(
                ReplicaDescription const& replicaDescription,
                uint64 serviceUpdateVersion,
                ReplicaFlags::Enum flags,
                PersistenceState::Enum persistenceState,
                Common::DateTime lastUpdated);

            // add an existing replica, only used in a deseralization in unit test now
            ReplicaIterator AddReplica(
                NodeInfoSPtr && nodeInfo,
                int64 replicaId,
                int64 instanceId,
                ReplicaStates::Enum state,
                ReplicaFlags::Enum flags,
                ReplicaRole::Enum pcRole,
                ReplicaRole::Enum ccRole,
                bool isUp,
                uint64 serviceUpdateVersion,
                ServiceModel::ServicePackageVersionInstance const& servicePackageVersionInstance,
                PersistenceState::Enum persistenceState,
                Common::DateTime lastUpdated);

            void RemoveAllReplicas();

            void UpdateReplicaPreviousConfigurationRole(ReplicaIterator replica, ReplicaRole::Enum newRole);
            void UpdateReplicaCurrentConfigurationRole(ReplicaIterator replica, ReplicaRole::Enum newRole);

            void UpdateEpochForDataLoss();
            void UpdateEpochForConfigurationChange(bool isPrimaryChange, bool resetLSN = true);

            void PostUpdate(Common::DateTime updatedTime, bool isPersistenceDone = true);

            void PostRead(int64 operationLSN);

            ServiceReplicaSet CreateServiceReplicaSet() const;

            // used before any state machine tasks for updating the node and service and pending movement pointers
            void UpdatePointers(FailoverManager & fm, NodeCache const& nodeCache, ServiceCache & serviceCache);

            // copy current configuration to previous configuration
            void StartReconfiguration(bool isPrimaryChange);

            void CompleteReconfiguration();

            void SwapPrimary(ReplicaIterator newPrimary);

            void ClearConfiguration();
            void ClearPreviousConfiguration();
            void ClearCurrentConfiguration();

            void RecoverFromDataLoss();

            void OnReplicaDown(Replica & replica, bool isDropped);
            void OnReplicaDropped(Replica & replica);
            void OnPrimaryDown();

            bool IsQuorumLost() const;
            bool HasPendingRemoveIdleReplica() const;

            void DropOfflineReplicas();

            void VerifyConsistency() const;

            bool CanGenerateAction() const;
            bool ShouldGenerateReconfigurationAction();

            void RemoveFromCurrentConfiguration(ReplicaIterator replica);

            DoReconfigurationMessageBody CreateDoReconfigurationMessageBody() const;
            FailoverUnitMessageBody CreateFailoverUnitMessageBody() const;

            LoadBalancingComponent::FailoverUnitDescription GetPLBFailoverUnitDescription(Common::StopwatchTime processingStartTime) const;

            bool UpdateHealthState();
            bool IsHealthReportNeeded();

            int GetRandomInteger(int max) const;

            bool IsSafeToCloseReplica(Replica const& replica, __out bool & isQuorumCheckNeeded, __out UpgradeSafetyCheckKind::Enum & kind) const;
            int GetSafeReplicaCloseCount(FailoverManager const& fm, ApplicationInfo const& applicationInfo) const;

            bool IsSafeToRemove(std::wstring const& batchId) const;

            bool IsSafeToUpgrade(Common::DateTime upgradeStartTime) const;

        private:
            // methods
            void ConstructConfiguration() const;

            void ResetLSN();

            void AddDetailedHealthReportDescription(__out std::wstring & extraDescription) const;
            FailoverUnitHealthState::Enum GetCurrentHealth(__out std::wstring & extraDescription) const;
            static FailoverUnitHealthState::Enum GetEffectiveHealth(FailoverUnitHealthState::Enum state);

            bool AreAllReplicasInSameUD() const;
            bool IsReplicaMoveNeeded(Replica const& replica) const;

            mutable FailoverManager * fm_;

            // NOTE: In the future we may have multiple service descriptions in one FailoverUnit.
            // This is because the ConsistencyUnits in a FailoverUnit may come from different services.
            mutable ServiceInfoSPtr serviceInfo_;

            // fields
            Reliability::FailoverUnitDescription failoverUnitDesc_;
            std::wstring serviceName_;
            mutable std::wstring idString_;

            std::vector<Replica> replicas_;

            FailoverUnitFlags::Flags flags_;

            mutable FailoverUnitConfiguration previousConfiguration_;
            mutable FailoverUnitConfiguration currentConfiguration_;

            // if replicas are added or deleted, vector<Replica> may move replica object
            // so the pc and cc will not have valid replica pointer
            // this flag indicate PC and CC will be re-constructed next time visiting them
            mutable bool isConfigurationValid_;

            // The lookup version that is used for Service Lookup Table refresh and broadcast.
            // This changes whenever a reconfiguration starts and a reconfiguration ends.
            int64 lookupVersion_;

            Common::DateTime lastUpdated_;
            Common::DateTime reconfigurationStartTime_;
            Common::DateTime placementStartTime_;

            Common::StopwatchTime lastActionTime_;
            Common::StopwatchTime processingStartTime_;

            //
            // Maintain a version that increments on every update independently from commit sequence number
            // for the following reasons:
            //
            // - FMM does not persist, so there is no commit sequence number
            // - Debug missed updates by identifying version gaps
            //
            int64 updateVersion_;

            bool isPersistencePending_;

            // Used for health report
            std::wstring extraDescription_;
            FailoverUnitHealthState::Enum currentHealthState_;
            FABRIC_SEQUENCE_NUMBER healthSequence_;
            bool updateForHealthReport_;

            // A positive value indicates the number of replica to create;
            // while, a negative value indicates the number of replica to drop.
            int replicaDifference_;

            // The time quorum loss is detected for this FailoverUnit.
            // If the FailoverUnit is currently in quorum loss, this is the quorum loss start time in ticks.
            // Otherwise, this is the last quorum loss duration in ticks.
            int64 lastQuorumLossTime_;
        };
    }
}
