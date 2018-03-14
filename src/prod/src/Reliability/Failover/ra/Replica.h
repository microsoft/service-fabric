// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents a replica within RA
        class Replica : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_ASSIGNMENT(Replica)

        public:

            Replica();

            // Required for vector<Replica>
            Replica(Replica && other);

            Replica(
                Federation::NodeInstance rendezvousNodeInstance, 
                int64 replicaId,
                int64 instance);

            Replica(Replica const & other);

            static Replica InvalidReplica;

            __declspec(property(get=get_FederationNodeInstance, put=set_FederationNodeInstance)) Federation::NodeInstance FederationNodeInstance;
            Federation::NodeInstance const & get_FederationNodeInstance() const { return desc_.FederationNodeInstance; }
            void set_FederationNodeInstance(Federation::NodeInstance const & value) { desc_.FederationNodeInstance = value; }

            __declspec(property(get=get_FederationNodeId)) Federation::NodeId FederationNodeId;
            Federation::NodeId get_FederationNodeId() const { return desc_.FederationNodeId; }

            __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const { return desc_.ReplicaId; }

            __declspec(property(get=get_InstanceId, put=set_InstanceId)) int64 InstanceId;
            int64 get_InstanceId() const { return desc_.InstanceId; }
            void set_InstanceId(int64 value) { desc_.InstanceId = value; }

            __declspec(property(get=get_CurrentConfigurationRole, put=set_CurrentConfigurationRole)) ReplicaRole::Enum CurrentConfigurationRole;
            ReplicaRole::Enum get_CurrentConfigurationRole() const { return desc_.CurrentConfigurationRole; }
            void set_CurrentConfigurationRole(ReplicaRole::Enum value) { desc_.CurrentConfigurationRole = value; }

            __declspec(property(get=get_PreviousConfigurationRole, put=set_PreviousConfigurationRole)) ReplicaRole::Enum PreviousConfigurationRole;
            ReplicaRole::Enum get_PreviousConfigurationRole() const { return desc_.PreviousConfigurationRole; }
            void set_PreviousConfigurationRole(ReplicaRole::Enum value) { desc_.PreviousConfigurationRole = value; }

            __declspec(property(get=get_IntermediateConfigurationRole, put=set_IntermediateConfigurationRole)) ReplicaRole::Enum IntermediateConfigurationRole;
            ReplicaRole::Enum get_IntermediateConfigurationRole() const { return intermediateConfigurationRole_; }
            void set_IntermediateConfigurationRole(ReplicaRole::Enum value) { intermediateConfigurationRole_ = value; }

            __declspec(property(get=get_State, put=set_State)) ReplicaStates::Enum State;
            ReplicaStates::Enum get_State() const { return state_; }
            void set_State(ReplicaStates::Enum value);

            __declspec(property(get=get_MessageStage, put=set_MessageStage)) ReplicaMessageStage::Enum MessageStage;
            ReplicaMessageStage::Enum get_MessageStage() const { return messageStage_; }
            void set_MessageStage(ReplicaMessageStage::Enum value) { messageStage_ = value; }

            __declspec(property(get=get_ToBeDeactivated, put=set_ToBeDeactivated)) bool ToBeDeactivated;
            bool get_ToBeDeactivated() const { return toBeDeactivated_; }
            void set_ToBeDeactivated(bool value) { toBeDeactivated_ = value; }

            __declspec(property(get=get_ToBeActivated, put=set_ToBeActivated)) bool ToBeActivated;
            bool get_ToBeActivated() const { return toBeActivated_; }
            void set_ToBeActivated(bool value) { toBeActivated_ = value; }

            __declspec(property(get = get_ToBeRestarted, put = set_ToBeRestarted)) bool ToBeRestarted;
            bool get_ToBeRestarted() const { return toBeRestarted_; }
            void set_ToBeRestarted(bool value) { toBeRestarted_ = value; }

            __declspec(property(get=get_ReplicatorRemovePending, put=set_ReplicatorRemovePending)) bool ReplicatorRemovePending;
            bool get_ReplicatorRemovePending() const { return replicatorRemovePending_; }
            void set_ReplicatorRemovePending(bool value) { replicatorRemovePending_ = value; }

            __declspec (property(get=get_ServiceLocation, put=set_ServiceLocation)) std::wstring const& ServiceLocation;
            std::wstring const& get_ServiceLocation() const { return desc_.ServiceLocation; }
            void set_ServiceLocation(std::wstring const & value) { desc_.ServiceLocation = value; }

            __declspec (property(get=get_ReplicationEndpoint, put=set_ReplicationEndpoint)) std::wstring const& ReplicationEndpoint;
            std::wstring const& get_ReplicationEndpoint() const { return desc_.ReplicationEndpoint; }
            void set_ReplicationEndpoint(std::wstring const & value) { desc_.ReplicationEndpoint = value; }

            __declspec(property(get=get_LastUpdated, put=set_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { lastUpdated_; }
            void set_LastUpdated(Common::DateTime value) { lastUpdated_ = value; }

            __declspec (property(get=get_IsInCurrentConfiguration)) bool IsInCurrentConfiguration;
            bool get_IsInCurrentConfiguration() const { return desc_.CurrentConfigurationRole >= ReplicaRole::Secondary; }
            
            __declspec (property(get=get_IsInPreviousConfiguration)) bool IsInPreviousConfiguration;
            bool get_IsInPreviousConfiguration() const { return desc_.PreviousConfigurationRole >= ReplicaRole::Secondary; }

            __declspec (property(get=get_IsInConfiguration)) bool IsInConfiguration;
            bool get_IsInConfiguration() const { return (IsInCurrentConfiguration || IsInPreviousConfiguration); }

            __declspec (property(get=get_IsInvalid)) bool IsInvalid;
            bool get_IsInvalid() const { return lastUpdated_ == Common::DateTime::Zero; }

            __declspec (property(get=get_IsUp, put=set_IsUp)) bool IsUp;
            bool get_IsUp() const { return desc_.IsUp; }
            void set_IsUp(bool value) { desc_.IsUp = value; }
            
            __declspec (property(get=get_IsReady)) bool IsReady;
            bool get_IsReady() const { return state_ == ReplicaStates::Ready; }
            
            __declspec (property(get=get_IsDropped)) bool IsDropped;
            bool get_IsDropped() const { return state_ == ReplicaStates::Dropped; }
            
            __declspec (property(get=get_IsStandBy)) bool IsStandBy;
            bool get_IsStandBy() const { return state_ == ReplicaStates::StandBy; }
            
            __declspec (property(get=get_IsOffline)) bool IsOffline;
            bool get_IsOffline() const { return (!IsUp && !IsDropped); }
            
            __declspec (property(get=get_IsInDrop)) bool IsInDrop;
            bool get_IsInDrop() const { return state_ == ReplicaStates::InDrop; }
            
            __declspec (property(get=get_IsInCreate)) bool IsInCreate;
            bool get_IsInCreate() const { return state_ == ReplicaStates::InCreate; }
            
            __declspec (property(get=get_IsInBuild)) bool IsInBuild;
            bool get_IsInBuild() const { return state_ == ReplicaStates::InBuild; }

            __declspec (property(get=get_IsAvailable)) bool IsAvailable;
            bool get_IsAvailable() const { return state_ == ReplicaStates::InBuild || state_ == ReplicaStates::Ready; }
            
            __declspec (property(get=get_IsBuildInProgress)) bool IsBuildInProgress;
            bool get_IsBuildInProgress() const { return state_ == ReplicaStates::InCreate || state_ == ReplicaStates::InBuild; }

            __declspec (property(get=get_ReplicaDescription)) Reliability::ReplicaDescription const & ReplicaDescription;
            Reliability::ReplicaDescription const & get_ReplicaDescription() const { return desc_; }
            Reliability::ReplicaDescription & get_ReplicaDescription() { return desc_; }

            __declspec(property(get = get_ReplicaDeactivationInfo)) Reliability::ReplicaDeactivationInfo const & ReplicaDeactivationInfo;
            Reliability::ReplicaDeactivationInfo const & get_ReplicaDeactivationInfo() const { return deactivationInfo_; } 

            __declspec(property(get=get_PackageVersionInstance, put=set_PackageVersionInstance)) ServiceModel::ServicePackageVersionInstance const & PackageVersionInstance;
            ServiceModel::ServicePackageVersionInstance const &  get_PackageVersionInstance() const { return desc_.PackageVersionInstance; }
            void set_PackageVersionInstance(ServiceModel::ServicePackageVersionInstance const & value) { desc_.PackageVersionInstance = value; }

            __declspec(property(get = get_QueryStatus)) FABRIC_QUERY_SERVICE_REPLICA_STATUS QueryStatus;
            FABRIC_QUERY_SERVICE_REPLICA_STATUS get_QueryStatus() const;

            void MarkAsDropped();

            void UpdateStateOnLocalReplicaDown(bool isLocalReplica);
            void UpdateStateOnReopen(bool isLocalReplica);
            void UpdateInstance(Reliability::ReplicaDescription const & newReplicaDescription, bool resetLSN);
            bool IsStale(Reliability::ReplicaDescription const & replicaInMessage) const;

            Reliability::ReplicaDescription GetCurrentConfigurationDescription() const;
            Reliability::ReplicaDescription GetConfigurationDescriptionForRebuildAndReplicaUp() const;
            Reliability::ReplicaDescription GetConfigurationDescriptionForReplicator() const;
            Reliability::ReplicaDescription GetConfigurationDescriptionForDeactivateOrActivate(bool treatInBuildAsReady) const;

            // Required for vector<Replica>
            Replica & operator = (Replica && other);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            int64 GetLastAcknowledgedLSN() const;
            int64 GetFirstAcknowledgedLSN() const;
            bool IsLSNUnknown() const;
            bool IsLSNSet() const;

            void SetFirstAndLastLSN(int64 firstLSN, int64 lastLSN);
            void SetFirstAndLastLSN(Reliability::ReplicaDescription const & replicaDescription);
            
            void SetProgress(
                Reliability::ReplicaDescription const & replicaDescription,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo);                
            
            void SetProgress(
                int64 firstLSN,
                int64 lastLSN,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo);

            void Test_SetProgress(
                int64 firstLSN,
                int64 lastLSN,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo);

            bool TrySetFirstAndLastLSNIfPresentInParameter(Reliability::ReplicaDescription const & replicaDescription);
            void SetLastLSN(int64 lastLSN);

            void SetLSNToUnknown(Reliability::ReplicaDescription const & incomingReplicaDescription);

            void ClearLSN();
            bool TryClearUnknownLSN();

            void AssertInvariants(FailoverUnit const & ft) const;

            FABRIC_FIELDS_04(
                desc_,
                intermediateConfigurationRole_,
                state_,
                lastUpdated_);

            class TraceWriter
            {
                DENY_COPY(TraceWriter);
            public:
                TraceWriter(Replica const & replica);

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

                void FillEventData(Common::TraceEventContext & context) const;
                void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            private:
                Replica const & replica_;
            };

        private:
            void Initialize();
            void Reset(bool resetLSN);
            void SetFirstAndLastLSNWithoutValidation(int64 firstLSN, int64 lastLSN);

            Reliability::ReplicaDescription GetDescription(ReplicaRole::Enum pcRole) const;

            Reliability::ReplicaDescription desc_;

            ReplicaRole::Enum intermediateConfigurationRole_;

            // States
            ReplicaStates::Enum state_;
            ReplicaMessageStage::Enum messageStage_;

            /*
                This decides whether a Deactivate message to update the deactivation epoch after
                a build is complete is pending or not
            */
            bool toBeDeactivated_;
            
            /*
                This decides whether the replica needs to be removed from the replicator view prior to any further operations
            */
            bool replicatorRemovePending_;

            /*
                This decides whether the activate message is pending for a remote replica
            */
            bool toBeActivated_;

            /*
                This decides whether a remote replica needs a restart or not
            */
            bool toBeRestarted_;

            Common::DateTime lastUpdated_;

            // Used during Phase1_GetLSN
            Reliability::ReplicaDeactivationInfo deactivationInfo_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ReconfigurationAgentComponent::Replica);
