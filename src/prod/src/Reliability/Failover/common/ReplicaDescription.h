// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReplicaDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(ReplicaDescription)
        DEFAULT_COPY_ASSIGNMENT(ReplicaDescription)
    
    public:
        ReplicaDescription();

        ReplicaDescription(
            Federation::NodeInstance federationNodeInstance,
            int64 replicaId,
            int64 instanceId,
            ReplicaRole::Enum previousConfigurationRole = ReplicaRole::None,
            ReplicaRole::Enum currentConfigurationRole = ReplicaRole::None,
            ReplicaStates::Enum state = ReplicaStates::Ready,
            bool isUp = true,
            FABRIC_SEQUENCE_NUMBER lastAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER,
            FABRIC_SEQUENCE_NUMBER firstAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER,
            std::wstring const & serviceLocation = std::wstring(),
            std::wstring const & replicationEndpoint = std::wstring(),
            ServiceModel::ServicePackageVersionInstance const & version = ServiceModel::ServicePackageVersionInstance::Invalid);

        __declspec(property(get=get_FederationNodeInstance, put=set_FederationNodeInstance)) Federation::NodeInstance FederationNodeInstance;
        Federation::NodeInstance const & get_FederationNodeInstance() const { return federationNodeInstance_; }
        void set_FederationNodeInstance(Federation::NodeInstance const & value) { federationNodeInstance_ = value; }

        __declspec(property(get=get_FederationNodeId)) Federation::NodeId FederationNodeId;
        Federation::NodeId get_FederationNodeId() const { return federationNodeInstance_.Id; }

        __declspec(property(get=get_ReplicaId, put=set_ReplicaId)) int64 ReplicaId;
        int64 get_ReplicaId() const { return replicaId_; }
        void set_ReplicaId(int64 value) { replicaId_ = value; }

        __declspec(property(get=get_InstanceId, put=set_InstanceId)) int64 InstanceId;
        int64 get_InstanceId() const { return instanceId_; }
        void set_InstanceId(int64 value) { instanceId_ = value; }

        __declspec(property(get=get_CurrentConfigurationRole, put=set_CurrentConfigurationRole)) ReplicaRole::Enum CurrentConfigurationRole;
        ReplicaRole::Enum get_CurrentConfigurationRole() const { return currentConfigurationRole_; }
        void set_CurrentConfigurationRole(ReplicaRole::Enum value) { currentConfigurationRole_ = value; }

        __declspec(property(get=get_PreviousConfigurationRole, put=set_PreviousConfigurationRole)) ReplicaRole::Enum PreviousConfigurationRole;
        ReplicaRole::Enum get_PreviousConfigurationRole() const { return previousConfigurationRole_; }
        void set_PreviousConfigurationRole(ReplicaRole::Enum value) { previousConfigurationRole_ = value; }

        __declspec(property(get = get_IsInCurrentConfiguration)) bool IsInCurrentConfiguration;
        bool get_IsInCurrentConfiguration() const { return currentConfigurationRole_ >= ReplicaRole::Secondary; }      

        __declspec(property(get = get_IsInPreviousConfiguration)) bool IsInPreviousConfiguration;
        bool get_IsInPreviousConfiguration() const { return previousConfigurationRole_ >= ReplicaRole::Secondary; }

        __declspec(property(get=get_LastAcknowledgedLSN, put=set_LastAcknowledgedLSN)) FABRIC_SEQUENCE_NUMBER LastAcknowledgedLSN;
        FABRIC_SEQUENCE_NUMBER get_LastAcknowledgedLSN() const { return lastAcknowledgedLSN_; }
        void set_LastAcknowledgedLSN(FABRIC_SEQUENCE_NUMBER value) { lastAcknowledgedLSN_ = value; }

        __declspec(property(get=get_FirstAcknowledgedLSN, put=set_FirstAcknowledgedLSN)) FABRIC_SEQUENCE_NUMBER FirstAcknowledgedLSN;
        FABRIC_SEQUENCE_NUMBER get_FirstAcknowledgedLSN() const { return firstAcknowledgedLSN_; }
        void set_FirstAcknowledgedLSN(FABRIC_SEQUENCE_NUMBER value) { firstAcknowledgedLSN_ = value; }

        __declspec(property(get=get_State, put=set_State)) ReplicaStates::Enum State;
        ReplicaStates::Enum get_State() const { return state_; }
        void set_State(ReplicaStates::Enum value) { state_ = value; }

        __declspec(property(get=get_IsUp, put=set_IsUp)) bool IsUp;
        bool get_IsUp() const { return isUp_; }
        void set_IsUp(bool isUp) { isUp_ = isUp; }

        __declspec (property(get=get_IsDropped)) bool IsDropped;
        bool get_IsDropped() const { return state_ == ReplicaStates::Dropped; }

        __declspec (property(get=get_IsAvailable)) bool IsAvailable;
        bool get_IsAvailable() const { return isUp_ && state_ == ReplicaStates::Ready; }

        __declspec (property(get=get_IsStandBy)) bool IsStandBy;
        bool get_IsStandBy() const { return state_ == ReplicaStates::StandBy; }

        __declspec(property(get = get_IsInBuild)) bool IsInBuild;
        bool get_IsInBuild() const { return state_ == ReplicaStates::InBuild; } 

        __declspec(property(get = get_IsReady)) bool IsReady;
        bool get_IsReady() const { return state_ == Reliability::ReplicaStates::Ready; }

        __declspec(property(get = get_IsFirstAcknowledgedLSNValid)) bool IsFirstAcknowledgedLSNValid;
        bool get_IsFirstAcknowledgedLSNValid() const { return firstAcknowledgedLSN_ != FABRIC_INVALID_SEQUENCE_NUMBER; }

        __declspec(property(get = get_IsLastAcknowledgedLSNValid)) bool IsLastAcknowledgedLSNValid;
        bool get_IsLastAcknowledgedLSNValid() const { return firstAcknowledgedLSN_ != FABRIC_INVALID_SEQUENCE_NUMBER; }

        __declspec (property(get=get_ReplicaStatus)) ReplicaStatus::Enum ReplicaStatus;
        ReplicaStatus::Enum get_ReplicaStatus() const { return isUp_ ? ReplicaStatus::Up : ReplicaStatus::Down; }

        __declspec(property(get=get_ServiceLocation, put=set_ServiceLocation)) std::wstring & ServiceLocation;
        std::wstring const & get_ServiceLocation() const { return serviceLocation_; }
        void set_ServiceLocation(std::wstring const & value) { serviceLocation_ = value; }

        __declspec(property(get=get_ReplicationEndpoint, put=set_ReplicationEndpoint)) std::wstring & ReplicationEndpoint;
        std::wstring const & get_ReplicationEndpoint() const { return replicationEndpoint_; }
        void set_ReplicationEndpoint(std::wstring const & value) { replicationEndpoint_ = value; }

        __declspec (property(get=get_PackageVersionInstance, put=put_PackageVersionInstance)) ServiceModel::ServicePackageVersionInstance & PackageVersionInstance;
        ServiceModel::ServicePackageVersionInstance const & get_PackageVersionInstance() const { return packageVersionInstance_; }
        void put_PackageVersionInstance(ServiceModel::ServicePackageVersionInstance const & value) { packageVersionInstance_ = value; }

        ReplicaDescription & operator = (ReplicaDescription && other);
        ReplicaDescription(ReplicaDescription && other);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_12(
            federationNodeInstance_,
            replicaId_,
            instanceId_,
            currentConfigurationRole_,
            previousConfigurationRole_,
            isUp_,
            lastAcknowledgedLSN_,
            firstAcknowledgedLSN_,
            state_,
            packageVersionInstance_,
            serviceLocation_,
            replicationEndpoint_);

        static ReplicaDescription CreateDroppedReplicaDescription(
            Federation::NodeInstance const & nodeInstance,
            int64 replicaId,
            int64 instanceId);

        void AssertLastAckLSNIsValid() const;
        void AssertFirstAckLSNIsValid() const;
        void AssertLSNsAreValid() const;
        void AssertLastAckLSNIsInvalid() const;
        void AssertFirstAckLSNIsInvalid() const;
        void AssertLSNsAreInvalid() const;
        void AssertLSNAreNotUnknown() const;
        
    private:
        Federation::NodeInstance federationNodeInstance_;
        int64 replicaId_;
        int64 instanceId_;

        ReplicaRole::Enum currentConfigurationRole_;
        ReplicaRole::Enum previousConfigurationRole_;

        bool isUp_;

        FABRIC_SEQUENCE_NUMBER lastAcknowledgedLSN_;
        FABRIC_SEQUENCE_NUMBER firstAcknowledgedLSN_;

        ReplicaStates::Enum state_;

        ServiceModel::ServicePackageVersionInstance packageVersionInstance_;

        std::wstring serviceLocation_;
        std::wstring replicationEndpoint_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ReplicaDescription);
