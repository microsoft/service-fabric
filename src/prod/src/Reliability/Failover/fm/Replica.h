// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class Replica : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(Replica)
        
        public:
            friend class FailoverUnit;

            // properties
            __declspec (property(get=get_IsValid)) bool const IsValid;
            bool const get_IsValid() const { return failoverUnit_ != nullptr; }

            __declspec (property(get=get_FailoverUnit)) FailoverUnit const& FailoverUnitObj;
            FailoverUnit const& get_FailoverUnit() const { return *failoverUnit_; }

            __declspec (property(get=get_NodeInfo, put=set_NodeInfo)) NodeInfoSPtr & NodeInfoObj;
            NodeInfoSPtr const& get_NodeInfo() const { return nodeInfo_; }
            void set_NodeInfo(NodeInfoSPtr const& nodeInfo) const { nodeInfo_ = nodeInfo; }

            __declspec (property(get=get_FederationNodeInstance, put=set_FederationNodeInstance)) Federation::NodeInstance FederationNodeInstance;
            Federation::NodeInstance const & get_FederationNodeInstance() const { return replicaDesc_.FederationNodeInstance; }
            void set_FederationNodeInstance(Federation::NodeInstance const & value);

            __declspec (property(get=get_NodeId)) Federation::NodeId FederationNodeId;
            Federation::NodeId get_NodeId() const { return replicaDesc_.FederationNodeId; }

            __declspec (property(get=get_ReplicaId, put=set_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const { return replicaDesc_.ReplicaId; }
            void set_ReplicaId(int64 value);

            __declspec (property(get=get_InstanceId, put=set_InstanceId)) int64 InstanceId;
            int64 get_InstanceId() const { return replicaDesc_.InstanceId; }
            void set_InstanceId(int64 value);

            __declspec (property(get = get_Flags)) ReplicaFlags::Enum Flags;
            ReplicaFlags::Enum get_Flags() const { return flags_; }

            __declspec (property(get=get_State, put=set_State)) ReplicaStates::Enum ReplicaState;
            ReplicaStates::Enum get_State() const { return replicaDesc_.State; }
            void set_State(ReplicaStates::Enum value);

            __declspec (property(get=get_ReplicaStatus)) FABRIC_QUERY_SERVICE_REPLICA_STATUS ReplicaStatus;
            FABRIC_QUERY_SERVICE_REPLICA_STATUS get_ReplicaStatus() const;

            __declspec (property(get=get_IsUp, put=set_IsUp)) bool IsUp;
            bool get_IsUp() const { return replicaDesc_.IsUp; }
            void set_IsUp(bool isUp);

            __declspec (property(get=get_IsAvailable)) bool IsAvailable;
            bool get_IsAvailable() const { return replicaDesc_.IsAvailable; }

            __declspec (property(get=get_PreviousConfigurationRole, put=set_PreviousConfigurationRole)) ReplicaRole::Enum PreviousConfigurationRole;
            ReplicaRole::Enum get_PreviousConfigurationRole() const { return replicaDesc_.PreviousConfigurationRole; }
            void set_PreviousConfigurationRole(ReplicaRole::Enum role);

            __declspec (property(get=get_CurrentConfigurationRole, put=set_CurrentConfigurationRole)) ReplicaRole::Enum CurrentConfigurationRole;
            ReplicaRole::Enum get_CurrentConfigurationRole() const { return replicaDesc_.CurrentConfigurationRole; }
            void set_CurrentConfigurationRole(ReplicaRole::Enum role);

            __declspec (property(get=get_IsCurrentConfigurationPrimary)) bool IsCurrentConfigurationPrimary;
            bool get_IsCurrentConfigurationPrimary() const { return (CurrentConfigurationRole == ReplicaRole::Primary); }

            __declspec (property(get=get_IsCurrentConfigurationSecondary)) bool IsCurrentConfigurationSecondary;
            bool get_IsCurrentConfigurationSecondary() const { return (CurrentConfigurationRole == ReplicaRole::Secondary); }

            __declspec (property(get=get_IsPreviousConfigurationPrimary)) bool IsPreviousConfigurationPrimary;
            bool get_IsPreviousConfigurationPrimary() const { return (PreviousConfigurationRole == ReplicaRole::Primary); }

            __declspec (property(get=get_IsPreviousConfigurationSecondary)) bool IsPreviousConfigurationSecondary;
            bool get_IsPreviousConfigurationSecondary() const { return (PreviousConfigurationRole == ReplicaRole::Secondary); }

            __declspec (property(get=get_IsInCurrentConfiguration)) bool IsInCurrentConfiguration;
            bool get_IsInCurrentConfiguration() const { return (CurrentConfigurationRole >= ReplicaRole::Secondary); }

            __declspec (property(get=get_IsInPreviousConfiguration)) bool IsInPreviousConfiguration;
            bool get_IsInPreviousConfiguration() const { return (PreviousConfigurationRole >= ReplicaRole::Secondary); }

            __declspec (property(get=get_IsInConfiguration)) bool IsInConfiguration;
            bool get_IsInConfiguration() const { return (IsInCurrentConfiguration || IsInPreviousConfiguration); }

            __declspec (property(get=get_IsNodeUp)) bool IsNodeUp;
            bool get_IsNodeUp() const { return (nodeInfo_->IsUp); }

            __declspec (property(get=get_IsOffline)) bool IsOffline;
            bool get_IsOffline() const { return (!IsUp && !IsDropped); }

            __declspec (property(get = get_IsToBeDropped)) bool IsToBeDropped;
            bool get_IsToBeDropped() const { return IsToBeDroppedByFM || IsToBeDroppedByPLB || IsToBeDroppedForNodeDeactivation; }

            __declspec (property(get = get_IsToBeDroppedByFM, put = set_IsToBeDroppedByFM)) bool IsToBeDroppedByFM;
            bool get_IsToBeDroppedByFM() const { return (flags_ & ReplicaFlags::ToBeDroppedByFM) != 0; }
            void set_IsToBeDroppedByFM(bool value);

            __declspec (property(get = get_IsToBeDroppedByPLB, put = set_IsToBeDroppedByPLB)) bool IsToBeDroppedByPLB;
            bool get_IsToBeDroppedByPLB() const { return (flags_ & ReplicaFlags::ToBeDroppedByPLB) != 0; }
            void set_IsToBeDroppedByPLB(bool value);

            __declspec (property(get = get_IsToBeDroppedForNodeDeactivation, put = set_IsToBeDroppedForNodeDeactivation)) bool IsToBeDroppedForNodeDeactivation;
            bool get_IsToBeDroppedForNodeDeactivation() const { return (flags_ & ReplicaFlags::ToBeDroppedForNodeDeactivation) != 0; }
            void set_IsToBeDroppedForNodeDeactivation(bool value);

            __declspec (property(get = get_IsToBePromoted, put = set_IsToBePromoted)) bool IsToBePromoted;
            bool get_IsToBePromoted() const { return (flags_ & ReplicaFlags::ToBePromoted) != 0; }
            void set_IsToBePromoted(bool value);

            __declspec (property(get = get_IsPendingRemove, put = set_IsPendingRemove)) bool IsPendingRemove;
            bool get_IsPendingRemove() const { return (flags_ & ReplicaFlags::PendingRemove) != 0; }
            void set_IsPendingRemove(bool value);

            __declspec (property(get = get_IsDeleted, put = set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return (flags_ & ReplicaFlags::Deleted) != 0; }
            void set_IsDeleted(bool value);

            __declspec (property(get = get_IsPreferredPrimaryLocation, put = set_IsPreferredPrimaryLocation)) bool IsPreferredPrimaryLocation;
            bool get_IsPreferredPrimaryLocation() const { return (flags_ & ReplicaFlags::PreferredPrimaryLocation) != 0; }
            void set_IsPreferredPrimaryLocation(bool value);

            __declspec (property(get = get_IsPreferredReplicaLocation, put = set_IsPreferredReplicaLocation)) bool IsPreferredReplicaLocation;
            bool get_IsPreferredReplicaLocation() const { return (flags_ & ReplicaFlags::PreferredReplicaLocation) != 0; }
            void set_IsPreferredReplicaLocation(bool value);

            __declspec (property(get = get_IsPrimaryToBeSwappedOut, put = set_IsPrimaryToBeSwappedOut)) bool IsPrimaryToBeSwappedOut;
            bool get_IsPrimaryToBeSwappedOut() const { return (flags_ & ReplicaFlags::PrimaryToBeSwappedOut) != 0; }
            void set_IsPrimaryToBeSwappedOut(bool value);

            __declspec (property(get = get_IsPrimaryToBePlaced, put = set_IsPrimaryToBePlaced)) bool IsPrimaryToBePlaced;
            bool get_IsPrimaryToBePlaced() const { return (flags_ & ReplicaFlags::PrimaryToBePlaced) != 0; }
            void set_IsPrimaryToBePlaced(bool value);

            __declspec (property(get = get_IsReplicaToBePlaced, put = set_IsReplicaToBePlaced)) bool IsReplicaToBePlaced;
            bool get_IsReplicaToBePlaced() const { return (flags_ & ReplicaFlags::ReplicaToBePlaced) != 0; }
            void set_IsReplicaToBePlaced(bool value);

            __declspec (property(get = get_IsMoveInProgress, put = set_IsMoveInProgress)) bool IsMoveInProgress;
            bool get_IsMoveInProgress() const { return (flags_ & ReplicaFlags::MoveInProgress) != 0; }
            void set_IsMoveInProgress(bool value);

            __declspec (property(get = get_IsEndpointAvailable, put = set_IsEndpointAvailable)) bool IsEndpointAvailable;
            bool get_IsEndpointAvailable() const { return (flags_ & ReplicaFlags::EndpointAvailable) != 0; }
            void set_IsEndpointAvailable(bool value);

            __declspec (property(get=get_IsCreating)) bool IsCreating;
            bool get_IsCreating() const { return (VersionInstance == ServiceModel::ServicePackageVersionInstance::Invalid); }

            __declspec (property(get=get_IsStandBy)) bool IsStandBy;
            bool get_IsStandBy() const { return ReplicaState == Reliability::ReplicaStates::StandBy; }

            __declspec (property(get=get_IsInBuild)) bool IsInBuild;
            bool get_IsInBuild() const { return ReplicaState == Reliability::ReplicaStates::InBuild; }

            __declspec (property(get=get_IsReady)) bool IsReady;
            bool get_IsReady() const { return ReplicaState == Reliability::ReplicaStates::Ready; }

            __declspec (property(get=get_IsDropped)) bool IsDropped;
            bool get_IsDropped() const { return ReplicaState == Reliability::ReplicaStates::Dropped; }

            __declspec (property(get=get_IsStable)) bool IsStable;
            bool get_IsStable() const;

            __declspec (property(get=get_VersionInstance, put=set_VersionInstance)) ServiceModel::ServicePackageVersionInstance & VersionInstance;
            ServiceModel::ServicePackageVersionInstance const& get_VersionInstance() const { return replicaDesc_.PackageVersionInstance; }
            void set_VersionInstance(ServiceModel::ServicePackageVersionInstance const& value);

            __declspec (property(get=get_LastActionTime, put=set_LastActionTime)) Common::StopwatchTime LastActionTime;
            Common::StopwatchTime get_LastActionTime() const { return lastActionTime_; }
            void set_LastActionTime(Common::StopwatchTime value) { lastActionTime_ = value; }

            __declspec (property(get=get_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { return lastUpdated_; }
            Common::StopwatchTime GetUpdateTime() const;

            __declspec (property(get = get_LastUpTime)) Common::DateTime LastUpTime;
            Common::DateTime get_LastUpTime() const;

            __declspec (property(get = get_LastDownTime)) Common::DateTime LastDownTime;
            Common::DateTime get_LastDownTime() const;

            __declspec (property(get = get_LastBuildDuration)) Common::TimeSpan LastBuildDuration;
            Common::TimeSpan get_LastBuildDuration() const;

            __declspec (property(get=get_PersistenceState, put=set_PersistenceState)) PersistenceState::Enum PersistenceState;
            PersistenceState::Enum get_PersistenceState() const { return persistenceState_; }
            void set_PersistenceState(PersistenceState::Enum value);

            __declspec (property(get=get_ServiceLocation, put=set_ServiceLocation)) std::wstring const& ServiceLocation;
            std::wstring const& get_ServiceLocation() const { return replicaDesc_.ServiceLocation; }
            void set_ServiceLocation(std::wstring const& value);

            __declspec (property(get=get_ReplicationEndpoint, put=set_ReplicationEndpoint)) std::wstring & ReplicationEndpoint;
            std::wstring const& get_ReplicationEndpoint() const { return replicaDesc_.ReplicationEndpoint; }
            void set_ReplicationEndpoint(std::wstring const& value);

            __declspec (property(get=get_ReplicaDescription)) Reliability::ReplicaDescription & ReplicaDescription;
            Reliability::ReplicaDescription const& get_ReplicaDescription() const { return replicaDesc_; }
            Reliability::ReplicaDescription & get_ReplicaDescription() { return replicaDesc_; }

            __declspec (property(get=get_ServiceUpdateVersion, put=set_ServiceUpdateVersion)) uint64 ServiceUpdateVersion;
            uint64 get_ServiceUpdateVersion() const { return serviceUpdateVersion_; }
            void set_ServiceUpdateVersion(uint64 value);

            void UpdateFailoverUnitPtr(FailoverUnit * failoverUnit);

            void VerifyConsistency() const;

            FABRIC_FIELDS_06(replicaDesc_, flags_, lastUpdated_, serviceUpdateVersion_, lastBuildTime_, lastUpDownTime_);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            Replica(); //empty constructor used for deserialization and invalid replica
            Replica & operator = (Replica && other); //used for vector of replicas to work

            Replica(Replica && other);

            virtual ~Replica();

            ReplicaMessageBody CreateReplicaMessageBody(bool isNewReplica) const;
            DeleteReplicaMessageBody CreateDeleteReplicaMessageBody() const;

            bool CanGenerateAction() const;

            bool ShouldBePrinted() const;

            template<class T, class t0>
            void GenerateAction(std::vector<StateMachineActionUPtr> & actions, t0 && a0)
            {
                if (CanGenerateAction())
                {
                    actions.push_back(Common::make_unique<T>(std::forward<t0>(a0)));
                    lastActionTime_ = Common::Stopwatch::Now();
                }
            }

            template<class T, class t0, class t1>
            void GenerateAction(std::vector<StateMachineActionUPtr> & actions, t0 && a0, t1 && a1)
            {
                if (CanGenerateAction())
                {
                    actions.push_back(Common::make_unique<T>(std::forward<t0>(a0), std::forward<t1>(a1)));
                    lastActionTime_ = Common::Stopwatch::Now();
                }
            }

        private:
            // create a new replica in a failoverUnit
            Replica(FailoverUnit * failoverUnit, NodeInfoSPtr && nodeInfo);
            
            // create an existing replica in a failoverUnit
            Replica(FailoverUnit * failoverUnit,
                    NodeInfoSPtr && nodeInfo,
                    int64 replicaId,
                    int64 instanceId,
                    ReplicaStates::Enum state,
                    ReplicaFlags::Enum flags,
                    ReplicaRole::Enum previousConfigurationRole,
                    ReplicaRole::Enum currentConfigurationRole,
                    bool isUp,
                    ServiceModel::ServicePackageVersionInstance const & version,
                    PersistenceState::Enum persistenceState,
                    Common::DateTime lastUpdated);

            // Create a Replica from a ReplicaDescription.
            Replica(
                FailoverUnit * failoverUnit,
                Reliability::ReplicaDescription const& replicaDescription,
                uint64 serviceUpdateVersion,
                ReplicaFlags::Enum flags,
                PersistenceState::Enum persistenceState,
                Common::DateTime lastUpdated);

            // clone a replica of one failoverUnit to another failoverUnit
            Replica(FailoverUnit * failoverUnit, Replica const& other);

            std::wstring GetStateString() const;

            void OnSetToBeDropped();

            void Reuse(NodeInfoSPtr && node);

            void PostUpdate(Common::DateTime updatedTime);

            FailoverUnit * failoverUnit_;
            mutable NodeInfoSPtr nodeInfo_;
            Reliability::ReplicaDescription replicaDesc_;
            ReplicaFlags::Enum flags_;
            PersistenceState::Enum persistenceState_;
            Common::StopwatchTime lastActionTime_;
            Common::DateTime lastUpdated_;

            // If the replica is InBuild, this is the start time in ticks.
            // Otherwise, this is the last build duration in ticks.
            int64 lastBuildTime_;

            // If the replica is up, this is the time it came up; otherwise,
            // it is the time the replica went down.
            Common::DateTime lastUpDownTime_;

            uint64 serviceUpdateVersion_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverManagerComponent::Replica);
