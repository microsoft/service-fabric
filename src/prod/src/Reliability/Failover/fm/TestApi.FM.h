// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/common/FailoverConfig.h"
#include "Reliability/Failover/fm/NodeDeactivationInfo.h"
#include "Reliability/Failover/common/ReplicaRole.h"
#include "Reliability/Failover/common/ReplicaStates.h"
#include "Reliability/Failover/fm/FailoverUnitHealthState.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingPublic.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingTestHelper.h"
#include "Reliability/Failover/TestApi.FailoverPointers.h"

namespace ReliabilityTestApi
{
    namespace FailoverManagerComponentTestApi
    {
        class FailoverManagerTestHelper
        {
            DENY_COPY(FailoverManagerTestHelper);
        public:
            FailoverManagerTestHelper(Reliability::FailoverManagerComponent::IFailoverManagerSPtr const &);

            __declspec(property(get=get_IsReady)) bool IsReady;
            bool get_IsReady() const;

            __declspec(property(get = get_HighestLookupVersion)) int64 HighestLookupVersion;
            int64 get_HighestLookupVersion() const;

            __declspec(property(get = get_SavedLookupVersion)) int64 SavedLookupVersion;
            int64 get_SavedLookupVersion() const;

            __declspec(property(get = get_FabricUpgradeTargetVersion)) Common::FabricVersion FabricUpgradeTargetVersion;
            Common::FabricVersion get_FabricUpgradeTargetVersion() const;

            __declspec(property(get = get_PLB)) Reliability::LoadBalancingComponent::IPlacementAndLoadBalancing & PLB;
            Reliability::LoadBalancingComponent::IPlacementAndLoadBalancing & get_PLB() const;

            __declspec(property(get = get_PLBTestHelper)) Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelperUPtr PLBTestHelper;
            Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelperUPtr get_PLBTestHelper() const;

            __declspec(property(get=get_FederationNodeId)) Federation::NodeId FederationNodeId;
            Federation::NodeId get_FederationNodeId() const;

            __declspec (property(get = get_HealthClient)) Client::HealthReportingComponentSPtr const& HealthClient;
            Client::HealthReportingComponentSPtr const& get_HealthClient() const;

            std::vector<FailoverUnitSnapshot> SnapshotGfum();
            std::vector<NodeSnapshot> SnapshotNodeCache();
            std::vector<NodeSnapshot> SnapshotNodeCache(size_t & upCountOut);
            std::vector<ServiceSnapshot> SnapshotServiceCache();
            std::vector<LoadSnapshot> SnapshotLoadCache();
            std::vector<InBuildFailoverUnitSnapshot> SnapshotInBuildFailoverUnits();
            std::vector<ServiceTypeSnapshot> SnapshotServiceTypes();
            std::vector<ApplicationSnapshot> SnapshotApplications();

            FailoverUnitSnapshotUPtr FindFTByPartitionId(Reliability::FailoverUnitId const & ftId);
            FailoverUnitSnapshotUPtr FindFTByPartitionId(Common::Guid const & ftId);

            std::vector<FailoverUnitSnapshot> FindFTByServiceName(std::wstring const & serviceName);
            ServiceSnapshotUPtr FindServiceByServiceName(std::wstring const & serviceName);

            ApplicationSnapshotUPtr FindApplicationById(ServiceModel::ApplicationIdentifier const& appId);

            NodeSnapshotUPtr FindNodeByNodeId(Federation::NodeId const & nodeId);

            bool InBuildFailoverUnitExistsForService(std::wstring const & serviceName) const;

            Common::FabricVersionInstance GetCurrentFabricVersionInstance();

            void SetPackageVersionInstance(
                Reliability::FailoverUnitId const & ftId, 
                Federation::NodeId const & nodeId, 
                ServiceModel::ServicePackageVersionInstance const & version);

            static Common::ErrorCode DeleteAllStoreData();
        private:
            Reliability::FailoverManagerComponent::FailoverManagerSPtr mutable fm_;
        };

        class FailoverUnitSnapshot
        {
        public:
            FailoverUnitSnapshot(Reliability::FailoverManagerComponent::FailoverUnit const & ft);

            __declspec(property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const;

            __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const;

            __declspec(property(get=get_FUID)) Reliability::FailoverUnitId const & Id;
            Reliability::FailoverUnitId const & get_FUID() const;

            __declspec(property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const;

            __declspec(property(get=get_IsChangingConfiguration)) bool IsChangingConfiguration;
            bool get_IsChangingConfiguration() const;

            __declspec(property(get=get_IsInReconfiguration)) bool IsInReconfiguration;
            bool get_IsInReconfiguration() const;

            __declspec(property(get=get_ReconfigurationPrimary)) ReplicaSnapshotUPtr ReconfigurationPrimary;
            ReplicaSnapshotUPtr get_ReconfigurationPrimary() const;

            __declspec(property(get=get_CurrentConfigurationEpoch)) Reliability::Epoch const & CurrentConfigurationEpoch;
            Reliability::Epoch const & get_CurrentConfigurationEpoch() const;

            __declspec(property(get=get_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const;

            __declspec(property(get=get_MinReplicaSetSize)) int MinReplicaSetSize;
            int get_MinReplicaSetSize() const;

            __declspec(property(get=get_IsToBeDeleted)) bool IsToBeDeleted;
            bool get_IsToBeDeleted() const;

            __declspec(property(get=get_IsOrphaned)) bool IsOrphaned;
            bool get_IsOrphaned() const;

            __declspec(property(get = get_IsCreatingPrimary)) bool IsCreatingPrimary;
            bool get_IsCreatingPrimary() const;

            __declspec(property(get=get_ReplicaCount)) size_t ReplicaCount;
            size_t get_ReplicaCount() const;

            __declspec (property(get=get_PartitionStatus)) FABRIC_QUERY_SERVICE_PARTITION_STATUS PartitionStatus;
            FABRIC_QUERY_SERVICE_PARTITION_STATUS get_PartitionStatus() const;

            __declspec (property(get=get_CurrentHealthState)) Reliability::FailoverManagerComponent::FailoverUnitHealthState::Enum CurrentHealthState;
            Reliability::FailoverManagerComponent::FailoverUnitHealthState::Enum get_CurrentHealthState() const;

            __declspec(property(get=get_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const;

            std::vector<ReplicaSnapshot> GetReplicas() const;
            std::vector<ReplicaSnapshot> GetCurrentConfigurationReplicas() const;

            ReplicaSnapshotUPtr GetReplica(Federation::NodeId const & nodeId) const;

            ServiceSnapshot GetServiceInfo() const;

            bool IsQuorumLost() const;


            FailoverUnitSnapshotUPtr CloneToUPtr() const;
            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::FailoverUnitSPtr ft_;
        };

        class InBuildFailoverUnitSnapshot
        {
        public:
            InBuildFailoverUnitSnapshot(Reliability::FailoverManagerComponent::InBuildFailoverUnitUPtr const & ft);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::InBuildFailoverUnitSPtr ft_;
        };

        class ReplicaSnapshot
        {
        public:
            ReplicaSnapshot(Reliability::FailoverManagerComponent::Replica const & replica);

            __declspec(property(get=get_IsCurrentConfigurationPrimary)) bool IsCurrentConfigurationPrimary;
            bool get_IsCurrentConfigurationPrimary() const;

            __declspec(property(get=get_IsCurrentConfigurationSecondary)) bool IsCurrentConfigurationSecondary;
            bool get_IsCurrentConfigurationSecondary() const;

            __declspec(property(get=get_FederationNodeId)) Federation::NodeId FederationNodeId;
            Federation::NodeId get_FederationNodeId() const;

            __declspec(property(get=get_CurrentConfigurationRole)) Reliability::ReplicaRole::Enum CurrentConfigurationRole;
            Reliability::ReplicaRole::Enum get_CurrentConfigurationRole() const;

            __declspec(property(get=get_IsDropped)) bool IsDropped;
            bool get_IsDropped() const;

            __declspec(property(get=get_IsInBuild)) bool IsInBuild;
            bool get_IsInBuild() const;

            __declspec(property(get=get_IsStandBy)) bool IsStandBy;
            bool get_IsStandBy() const;

            __declspec(property(get=get_IsOffline)) bool IsOffline;
            bool get_IsOffline() const;

            __declspec(property(get=get_IsCreating)) bool IsCreating;
            bool get_IsCreating() const;

            __declspec(property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const;

            __declspec(property(get=get_InstanceId)) int64 InstanceId;
            int64 get_InstanceId() const;

            __declspec(property(get = get_IsToBeDropped)) bool IsToBeDropped;
            bool get_IsToBeDropped() const;

            __declspec(property(get = get_Version)) ServiceModel::ServicePackageVersionInstance const & Version;
            ServiceModel::ServicePackageVersionInstance const & get_Version() const;

            __declspec(property(get=get_ServiceLocation)) std::wstring const & ServiceLocation;
            std::wstring const & get_ServiceLocation() const;

            __declspec (property(get=get_State, put=set_State)) Reliability::ReplicaStates::Enum ReplicaState;
            Reliability::ReplicaStates::Enum get_State() const;

            __declspec(property(get=get_ReplicaStatus)) Reliability::ReplicaStatus::Enum ReplicaStatus;
            Reliability::ReplicaStatus::Enum get_ReplicaStatus() const;

            __declspec (property(get=get_ReplicaId, put=set_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const;

            __declspec(property(get=get_IsInCurrentConfiguration)) bool IsInCurrentConfiguration;
            bool get_IsInCurrentConfiguration() const;

            __declspec(property(get = get_IsInPreviousConfiguration)) bool IsInPreviousConfiguration;
            bool get_IsInPreviousConfiguration() const;

            NodeSnapshot GetNode() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::ReplicaSPtr replica_;
        };

        class NodeSnapshot
        {
        public:
            NodeSnapshot(Reliability::FailoverManagerComponent::NodeInfoSPtr const & node);

            __declspec(property(get=get_NodeInstance)) Federation::NodeInstance NodeInstance;
            Federation::NodeInstance get_NodeInstance() const;

            __declspec(property(get=get_Id)) Federation::NodeId Id;
            Federation::NodeId get_Id() const;

            __declspec(property(get=get_DeactivationInfo)) Reliability::FailoverManagerComponent::NodeDeactivationInfo const & DeactivationInfo;
            Reliability::FailoverManagerComponent::NodeDeactivationInfo const & get_DeactivationInfo() const;

            __declspec(property(get=get_IsReplicaUploaded)) bool IsReplicaUploaded;
            bool get_IsReplicaUploaded() const;

            __declspec(property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const;

            __declspec(property(get=get_IsPendingFabricUpgrade)) bool IsPendingFabricUpgrade;
            bool get_IsPendingFabricUpgrade() const;

            __declspec(property(get=get_NodeStatus)) FABRIC_QUERY_NODE_STATUS NodeStatus;
            FABRIC_QUERY_NODE_STATUS get_NodeStatus() const;

            __declspec(property(get=get_VersionInstance)) Common::FabricVersionInstance const & VersionInstance;
            Common::FabricVersionInstance const & get_VersionInstance() const;

            __declspec(property(get=get_ActualUpgradeDomainId)) std::wstring const & ActualUpgradeDomainId;
            std::wstring const & get_ActualUpgradeDomainId() const;

            __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
            std::wstring const& get_NodeName() const;

            __declspec (property(get=get_NodeType)) std::wstring const& NodeType;
            std::wstring const& get_NodeType() const;

            __declspec (property(get=get_IpAddressOrFQDN)) std::wstring const& IpAddressOrFQDN;
            std::wstring const& get_IpAddressOrFQDN() const;

            __declspec (property(get=get_FaultDomainId)) std::wstring FaultDomainId;
            std::wstring get_FaultDomainId() const;

            __declspec(property(get=get_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const;

            __declspec (property(get=get_FaultDomainIds)) std::vector<Common::Uri> const & FaultDomainIds;
            std::vector<Common::Uri> const & get_FaultDomainIds() const;

            __declspec(property(get = get_IsNodeStateRemoved)) bool IsNodeStateRemoved;
            bool get_IsNodeStateRemoved() const;

            __declspec(property(get = get_IsUnknown)) bool IsUnknown;
            bool get_IsUnknown() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::NodeInfoSPtr node_;
        };

        class ServiceSnapshot
        {
        public:
            ServiceSnapshot(Reliability::FailoverManagerComponent::ServiceInfoSPtr const & serviceInfo);

            __declspec(property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const;

            __declspec(property(get=get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const;

            __declspec(property(get=get_IsStale)) bool IsStale;
            bool get_IsStale() const;

            __declspec(property(get=get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const;

            __declspec(property(get=get_IsToBeDeleted)) bool IsToBeDeleted;
            bool get_IsToBeDeleted() const;

            __declspec(property(get=get_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const;

            __declspec(property(get=get_Status)) FABRIC_QUERY_SERVICE_STATUS Status;
            FABRIC_QUERY_SERVICE_STATUS get_Status() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            ServiceTypeSnapshot GetServiceType() const;
            ServiceModel::ServicePackageVersionInstance GetTargetVersion(std::wstring const & upgradeDomain) const;
            Reliability::LoadBalancingComponent::ServiceDescription GetPLBServiceDescription() const;
        private:
            Reliability::FailoverManagerComponent::ServiceInfoSPtr service_;
        };

        class LoadSnapshot
        {
        public:
            LoadSnapshot(Reliability::FailoverManagerComponent::LoadInfoSPtr load);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::LoadInfoSPtr load_;
        };

        class ServiceTypeSnapshot
        {
        public:
            ServiceTypeSnapshot(Reliability::FailoverManagerComponent::ServiceTypeSPtr const & st);

            __declspec(property(get=get_Type)) ServiceModel::ServiceTypeIdentifier const & Type;
            ServiceModel::ServiceTypeIdentifier const & get_Type() const;

            ApplicationSnapshot GetApplication() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::ServiceTypeSPtr st_;
        };

        class ApplicationSnapshot
        {
        public:
            ApplicationSnapshot(Reliability::FailoverManagerComponent::ApplicationInfoSPtr const & app);

            __declspec(property(get = get_AppId)) ServiceModel::ApplicationIdentifier const & AppId;
            ServiceModel::ApplicationIdentifier const & get_AppId() const;

            __declspec(property(get = get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const;

            __declspec(property(get = get_IsUpgradeCompleted)) bool IsUpgradeCompleted;
            bool get_IsUpgradeCompleted() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::FailoverManagerComponent::ApplicationInfoSPtr app_;            
        };
    }    
}

