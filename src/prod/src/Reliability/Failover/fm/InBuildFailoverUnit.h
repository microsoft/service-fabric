// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class InBuildReplica : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(InBuildReplica)
        
        public:

            InBuildReplica();

            InBuildReplica(InBuildReplica && other);

            InBuildReplica(ReplicaDescription const & replicaDescription, bool isReportedByPrimary, bool isReportedBySecondary);

            InBuildReplica & operator=(InBuildReplica && other);

            __declspec(property(get=get_Description)) ReplicaDescription & Description;
            ReplicaDescription & get_Description() { return replicaDescription_; }
            ReplicaDescription const & get_Description() const { return replicaDescription_; }

            __declspec (property(get=get_ServiceUpdateVersion, put=set_ServiceUpdateVersion)) uint64 ServiceUpdateVersion;
            uint64 get_ServiceUpdateVersion() const { return serviceUpdateVersion_; }
            void set_ServiceUpdateVersion(uint64 value) { serviceUpdateVersion_ = value; }

            __declspec(property(get=get_Version, put=set_Version)) ServiceModel::ServicePackageVersionInstance const & Version;
            ServiceModel::ServicePackageVersionInstance const & get_Version() { return replicaDescription_.PackageVersionInstance; }
            void set_Version(ServiceModel::ServicePackageVersionInstance const & value) { replicaDescription_.PackageVersionInstance = value; }

            __declspec (property(get=get_IsReportedByPrimary, put=set_IsReportedByPrimary)) bool IsReportedByPrimary;
            bool get_IsReportedByPrimary() const { return isReportedByPrimary_; }
            void set_IsReportedByPrimary(bool value) { isReportedByPrimary_ = value; }

            __declspec (property(get=get_IsReportedBySecondary, put=set_IsReportedBySecondary)) bool IsReportedBySecondary;
            bool get_IsReportedBySecondary() const { return isReportedBySecondary_; }
            void set_IsReportedBySecondary(bool value) { isReportedBySecondary_ = value; }

            __declspec (property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return isDeleted_; }
            void set_IsDeleted(bool value) { isDeleted_ = value; }

            __declspec (property(get=get_CCEpoch, put=set_CCEpoch)) Epoch const & CCEpoch;
            Epoch const& get_CCEpoch() const { return ccEpoch_; }
            void set_CCEpoch(Epoch const& value) { ccEpoch_ = value; }

            __declspec (property(get=get_ICEpoch, put=set_ICEpoch)) Epoch const & ICEpoch;
            Epoch const& get_ICEpoch() const { return icEpoch_; }
            void set_ICEpoch(Epoch const& value) { icEpoch_ = value; }

            __declspec (property(get=get_PCEpoch, put=set_PCEpoch)) Epoch const & PCEpoch;
            Epoch const& get_PCEpoch() const { return pcEpoch_; }
            void set_PCEpoch(Epoch const& value) { pcEpoch_ = value; }

            __declspec (property(get=get_IsSelfReported)) bool IsSelfReported;
            bool get_IsSelfReported() const { return ccEpoch_ != Epoch::InvalidEpoch(); }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_08(replicaDescription_, isReportedByPrimary_, isReportedBySecondary_, isDeleted_, ccEpoch_, icEpoch_, pcEpoch_, serviceUpdateVersion_);

        private:

            ReplicaDescription replicaDescription_;

            uint64 serviceUpdateVersion_;

            bool isReportedByPrimary_;
            bool isReportedBySecondary_;
            bool isDeleted_;

            // The Epochs reported by the node hosting this replica.
            // Note that if the node hosting this replica reports then
            // ccEpoch must be valid; otherwise, all the epochs are invalid.
            // This can be used to determine if the node hosting this 
            // replica has reported or not.
            Epoch ccEpoch_;
            Epoch icEpoch_;
            Epoch pcEpoch_;
        };

        class InBuildFailoverUnit : public StoreData
        {
        public:

            InBuildFailoverUnit() { }

            InBuildFailoverUnit(InBuildFailoverUnit const& other);

            InBuildFailoverUnit(
                FailoverUnitId const& failoverUnitId,
                ConsistencyUnitDescription const& consistencyUnitDescription,
                ServiceDescription const& serviceDescription);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            bool Add(FailoverUnitInfo const& report, Federation::NodeInstance const& from, FailoverManager& fm);

            bool InBuildReplicaExistsForNode(Federation::NodeId const& nodeId) const;

            FailoverUnitUPtr Generate(NodeCache const& nodeCache, bool forceRecovery);

            bool GenerateDeleteReplicaActions(NodeCache const& nodeCache, std::vector<StateMachineActionUPtr> & actions) const;

            bool OnReplicaDropped(Federation::NodeId const& nodeId, bool isDeleted);

            bool IsServiceUpdateNeeded() const;

            __declspec (property(get=get_FUId)) FailoverUnitId const& Id;
            FailoverUnitId const& get_FUId() const { return failoverUnitId_; }

            __declspec (property(get=get_IdString)) std::wstring const& IdString;
            std::wstring const& get_IdString() const;

            __declspec (property(get=get_Description)) ServiceDescription const& Description;
            ServiceDescription const& get_Description() const { return serviceDescription_; }

            __declspec (property(get=get_IsToBeDeleted, put=set_IsToBeDeleted)) bool IsToBeDeleted;
            bool get_IsToBeDeleted() const { return isToBeDeleted_; }
            void set_IsToBeDeleted(bool value) { isToBeDeleted_ = value; } 

            __declspec (property(get=get_MaxEpoch)) Reliability::Epoch const& MaxEpoch;
            Reliability::Epoch const& get_MaxEpoch() const { return maxEpoch_; }

            __declspec (property(get = get_HealthSequence, put = set_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const { return healthSequence_; }
            void set_HealthSequence(FABRIC_SEQUENCE_NUMBER value) { healthSequence_ = value; }

            __declspec (property(get = get_IsHealthReported, put = set_IsHealthReported)) bool IsHealthReported;
            bool get_IsHealthReported() const { return isHealthReported_; }
            void set_IsHealthReported(bool value) { isHealthReported_ = value; }

            __declspec (property(get=get_CreatedTime)) Common::DateTime CreatedTime;
            Common::DateTime get_CreatedTime() const { return createdTime_; }

            __declspec (property(get=get_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { return lastUpdated_; }

            void PostUpdate(Common::DateTime updateTime);

            void PostRead(int64 operationLSN);

            FABRIC_FIELDS_17(
                failoverUnitId_,
                consistencyUnitDescription_,
                serviceDescription_,
                cc_,
                pc_,
                reportingNodes_,
                replicas_,
                maxEpoch_,
                isCCActivated_,
                isPrimaryAvailable_,
                isSecondaryAvailable_,
                isSwapPrimary_,
                pcEpochByReadyPrimary_,
                createdTime_,
                lastUpdated_,
                healthSequence_,
                isHealthReported_);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        private:

            typedef enum { PC, IC, CC } Type;

            class ReportedConfiguration : public Serialization::FabricSerializable
            {
            public:

                ReportedConfiguration();

                ReportedConfiguration(FailoverUnitInfo const& report, Federation::NodeInstance const& from, Type type);

                __declspec (property(get=get_Epoch)) Reliability::Epoch const& Epoch;
                Reliability::Epoch const& get_Epoch() const { return epoch_; }

                bool IsEmpty() const { return isEmpty_; }
                bool IsPrimary(Federation::NodeId nodeId) const;
                bool IsSecondary(Federation::NodeId nodeId) const;
                bool Contains(Federation::NodeId nodeId) const;
                bool Contains(ReportedConfiguration const& configuration) const;

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

                FABRIC_FIELDS_04(epoch_, primary_, secondaries_, isEmpty_);

            private:

                Reliability::Epoch epoch_;

                Federation::NodeId primary_;
                std::vector<Federation::NodeId> secondaries_;
                bool isEmpty_;
            };

            static bool IsIdle(ReplicaRole::Enum pcRole, ReplicaRole::Enum ccOrICRole);

            static ReplicaRole::Enum GetRole(std::unique_ptr<ReportedConfiguration> const& configuration, ReplicaDescription const& replica);

            bool IsDuplicate(FailoverUnitInfo const& report, Federation::NodeInstance const& from);
            bool IsStale(FailoverUnitInfo const& report, Federation::NodeInstance const& from) const;

            void CheckReplica(
                ReplicaInfo const & replicaInfo, 
                FailoverUnitInfo const& report, 
                Federation::NodeInstance const& from, 
                bool fromReadyPrimary,
                bool fromReadySecondary,
                FailoverManager & fm,
                __out bool & discardPC);

            void ConsiderCC(std::unique_ptr<ReportedConfiguration> & cc, bool isCCActivated);
            void ConsiderPC(std::unique_ptr<ReportedConfiguration> & pc);

            void GenerateReplica(FailoverUnit & failoverUnit, InBuildReplica & inBuildReplica);

            bool DropOfflineReplicas(FailoverUnit & failoverUnit, NodeCache const& nodeCache);

            bool IsConfigurationReported(Epoch const & epoch,  Federation::NodeId from) const;
            size_t GetReportedCount(Epoch const & epoch, FailoverUnitConfiguration const& configuration) const;

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return serviceDescription_.IsStateful; }

            __declspec (property(get=get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const { return serviceDescription_.HasPersistedState; }

            FailoverUnitId failoverUnitId_;
            ConsistencyUnitDescription consistencyUnitDescription_;
            ServiceDescription serviceDescription_;

            std::unique_ptr<ReportedConfiguration> cc_;
            std::unique_ptr<ReportedConfiguration> pc_;

            // The nodes from which a report has been received.
            std::vector<Federation::NodeId> reportingNodes_;

            // The list of replicas we know about.
            std::vector<InBuildReplica> replicas_;

            // The max epoch reported a replica.
            Epoch maxEpoch_;

            // Indicates if the CC has been successfully activated.
            bool isCCActivated_;

            // Indicates if the primary is Up and Ready.
            bool isPrimaryAvailable_;

            // Indicates if a secondary is Up and Ready.
            bool isSecondaryAvailable_;

            // Whether a swap primary is in-progress.
            bool isSwapPrimary_;

            // Delete the FailoverUnit if its service instance is stale.
            bool isToBeDeleted_;

            // The PC epoch reported by a Ready primary.
            Epoch pcEpochByReadyPrimary_;

            // The time it is created.
            Common::DateTime createdTime_;

            // The last time this InBuildFailoverUnit was updated.
            Common::DateTime lastUpdated_;

            mutable std::wstring idString_;

            FABRIC_SEQUENCE_NUMBER healthSequence_;
            bool isHealthReported_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverManagerComponent::InBuildReplica);

