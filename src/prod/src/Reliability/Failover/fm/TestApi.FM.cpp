// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../TestApi.Failover.Internal.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingTestHelper.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace FailoverManagerComponent;
using namespace ReliabilityTestApi;
using namespace ReliabilityTestApi::FailoverManagerComponentTestApi;

ErrorCode FailoverManagerTestHelper::DeleteAllStoreData()
{
    return ErrorCode::Success();
}

FailoverManagerTestHelper::FailoverManagerTestHelper(IFailoverManagerSPtr const & fm)
: fm_(dynamic_pointer_cast<FailoverManager>(fm))
{
}

bool FailoverManagerTestHelper::get_IsReady() const
{
    return fm_->IsReady;
}

int64 FailoverManagerTestHelper::get_HighestLookupVersion() const
{
    return (fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion - 1);
}

FabricVersion FailoverManagerTestHelper::get_FabricUpgradeTargetVersion() const
{
    return fm_->FabricUpgradeManager.Upgrade->Description.Version;
}

int64 FailoverManagerTestHelper::get_SavedLookupVersion() const
{
    return fm_->FailoverUnitCacheObj.SavedLookupVersion;
}

LoadBalancingComponent::IPlacementAndLoadBalancing & FailoverManagerTestHelper::get_PLB() const
{
    InstrumentedPLB & iplb = dynamic_cast<InstrumentedPLB &>(fm_->PLB);
    return iplb.PLB;
}

Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelperUPtr FailoverManagerTestHelper::get_PLBTestHelper() const
{
    InstrumentedPLB & iplb = dynamic_cast<InstrumentedPLB &>(fm_->PLB);
    return make_unique<Reliability::LoadBalancingComponent::PlacementAndLoadBalancingTestHelper>(iplb.PLB);
}

NodeId FailoverManagerTestHelper::get_FederationNodeId() const
{
    return fm_->Federation.Id;
}

Client::HealthReportingComponentSPtr const& FailoverManagerTestHelper::get_HealthClient() const
{
    return fm_->HealthClient;
}

vector<FailoverUnitSnapshot> FailoverManagerTestHelper::SnapshotGfum()
{
    vector<FailoverUnitSnapshot> rv;

    auto visitor = fm_->FailoverUnitCacheObj.CreateVisitor();
    while (auto ft = visitor->MoveNext())
    {
        rv.push_back(FailoverUnitSnapshot(*ft));
    }

    return rv;
}

vector<NodeSnapshot> FailoverManagerTestHelper::SnapshotNodeCache()
{
    vector<NodeInfoSPtr> nodes;
    fm_->NodeCacheObj.GetNodes(nodes);
    return ConstructSnapshotObjectList<NodeInfoSPtr, NodeSnapshot>(nodes);
}

vector<NodeSnapshot> FailoverManagerTestHelper::SnapshotNodeCache(size_t & upCountOut)
{
    auto rv = SnapshotNodeCache();
    upCountOut = fm_->NodeCacheObj.UpCount;
    return rv;
}

vector<ServiceSnapshot> FailoverManagerTestHelper::SnapshotServiceCache()
{
    vector<ServiceInfoSPtr> services;
    fm_->ServiceCacheObj.GetServices(services);
    return ConstructSnapshotObjectList<ServiceInfoSPtr, ServiceSnapshot>(services);
}

vector<LoadSnapshot> FailoverManagerTestHelper::SnapshotLoadCache()
{
    vector<LoadInfoSPtr> loads;
    fm_->LoadCacheObj.GetAllLoads(loads);
    return ConstructSnapshotObjectList<LoadInfoSPtr, LoadSnapshot>(loads);
}

vector<InBuildFailoverUnitSnapshot> FailoverManagerTestHelper::SnapshotInBuildFailoverUnits()
{
    vector<InBuildFailoverUnitUPtr> inbuildFTs;
    fm_->InBuildFailoverUnitCacheObj.GetInBuildFailoverUnits(inbuildFTs);
    return ConstructSnapshotObjectList<InBuildFailoverUnitUPtr, InBuildFailoverUnitSnapshot>(inbuildFTs);
}

vector<ServiceTypeSnapshot> FailoverManagerTestHelper::SnapshotServiceTypes()
{
    vector<ServiceTypeSPtr> serviceTypes;
    fm_->ServiceCacheObj.GetServiceTypes(serviceTypes);
    return ConstructSnapshotObjectList<ServiceTypeSPtr, ServiceTypeSnapshot>(serviceTypes);
}

vector<ApplicationSnapshot> FailoverManagerTestHelper::SnapshotApplications()
{
    vector<ApplicationInfoSPtr> applications;
    fm_->ServiceCacheObj.GetApplications(applications);
    return ConstructSnapshotObjectList<ApplicationInfoSPtr, ApplicationSnapshot>(applications);
}

FailoverUnitSnapshotUPtr FailoverManagerTestHelper::FindFTByPartitionId(FailoverUnitId const & ftId)
{
    auto gfum = SnapshotGfum();
    auto ft = find_if(gfum.begin(), gfum.end(), [ftId] (FailoverUnitSnapshot const & inner)
    {
        return inner.Id == ftId;
    });

    if (ft == gfum.end())
    {
        return FailoverUnitSnapshotUPtr();
    }

    return make_unique<FailoverUnitSnapshot>(*ft);
}

FailoverUnitSnapshotUPtr FailoverManagerTestHelper::FindFTByPartitionId(Guid const & ftId)
{
    return FindFTByPartitionId(FailoverUnitId(ftId));
}

vector<FailoverUnitSnapshot> FailoverManagerTestHelper::FindFTByServiceName(wstring const & serviceName)
{
    auto gfum = SnapshotGfum();
    vector<FailoverUnitSnapshot> rv;

    for(auto it = gfum.begin(); it != gfum.end(); ++it)
    {
        if (!it->IsOrphaned && it->ServiceName == serviceName)
        {
            rv.push_back(*it);
        }
    }

    return rv;
}

ServiceSnapshotUPtr FailoverManagerTestHelper::FindServiceByServiceName(wstring const & serviceName)
{
    auto service = fm_->ServiceCacheObj.GetService(serviceName);

    if (service == nullptr)
    {
        return ServiceSnapshotUPtr();
    }

    return make_unique<ServiceSnapshot>(service);
}

ApplicationSnapshotUPtr FailoverManagerTestHelper::FindApplicationById(ServiceModel::ApplicationIdentifier const& appId)
{
    auto appInfo = fm_->ServiceCacheObj.GetApplication(appId);
    if (appInfo == nullptr)
    {
        return ApplicationSnapshotUPtr();
    }

    return make_unique<ApplicationSnapshot>(appInfo);
}

NodeSnapshotUPtr FailoverManagerTestHelper::FindNodeByNodeId(NodeId const & nodeId)
{
    auto node = fm_->NodeCacheObj.GetNode(nodeId);

    if (node == nullptr)
    {
        return NodeSnapshotUPtr();
    }

    return make_unique<NodeSnapshot>(node);
}

bool FailoverManagerTestHelper::InBuildFailoverUnitExistsForService(wstring const & serviceName) const
{
    return fm_->InBuildFailoverUnitCacheObj.InBuildFailoverUnitExistsForService(serviceName);
}

FabricVersionInstance FailoverManagerTestHelper::GetCurrentFabricVersionInstance()
{
    return fm_->FabricUpgradeManager.CurrentVersionInstance;
}

void FailoverManagerTestHelper::SetPackageVersionInstance(
    Reliability::FailoverUnitId const & ftId,
    Federation::NodeId const & nodeId,
    ServiceModel::ServicePackageVersionInstance const & version)
{
    LockedFailoverUnitPtr ft;
    if (!fm_->FailoverUnitCacheObj.TryGetLockedFailoverUnit(ftId, ft))
    {
        return;
    }

    auto replica = ft->GetReplica(nodeId);
    ASSERT_IF(replica == nullptr, "Replica not found for {0} {1}", ftId, nodeId);

    ft.EnableUpdate();
    replica->VersionInstance = version;
    ft->PersistenceState = PersistenceState::NoChange;
    ft.Submit();
}

FailoverUnitSnapshot::FailoverUnitSnapshot(FailoverUnit const & ft)
: ft_(std::make_shared<FailoverUnit>(ft))
{
}

Reliability::FailoverUnitDescription const & FailoverUnitSnapshot::get_FailoverUnitDescription() const
{
    return ft_->FailoverUnitDescription;
}

wstring const & FailoverUnitSnapshot::get_ServiceName() const
{
    return ft_->ServiceName;
}

FailoverUnitId const & FailoverUnitSnapshot::get_FUID() const
{
    return ft_->Id;
}

bool FailoverUnitSnapshot::get_IsStateful() const
{
    return ft_->IsStateful;
}

bool FailoverUnitSnapshot::get_IsChangingConfiguration() const
{
    return ft_->IsChangingConfiguration;
}

bool FailoverUnitSnapshot::get_IsInReconfiguration() const
{
    return ft_->IsInReconfiguration;
}

ReplicaSnapshotUPtr FailoverUnitSnapshot::get_ReconfigurationPrimary() const
{
    auto replica = ft_->ReconfigurationPrimary;
    if (replica == ft_->EndIterator)
    {
        return ReplicaSnapshotUPtr();
    }

    return make_unique<ReplicaSnapshot>(*replica);
}

Epoch const & FailoverUnitSnapshot::get_CurrentConfigurationEpoch() const
{
    return ft_->CurrentConfigurationEpoch;
}

int FailoverUnitSnapshot::get_TargetReplicaSetSize() const
{
    return ft_->TargetReplicaSetSize;
}

int FailoverUnitSnapshot::get_MinReplicaSetSize() const
{
    return ft_->MinReplicaSetSize;
}

bool FailoverUnitSnapshot::get_IsToBeDeleted() const
{
    return ft_->IsToBeDeleted;
}

bool FailoverUnitSnapshot::get_IsOrphaned() const
{
    return ft_->IsOrphaned;
}

bool FailoverUnitSnapshot::get_IsCreatingPrimary() const
{
    return ft_->IsCreatingPrimary;
}

size_t FailoverUnitSnapshot::get_ReplicaCount() const
{
    return ft_->ReplicaCount;
}

FABRIC_QUERY_SERVICE_PARTITION_STATUS FailoverUnitSnapshot::get_PartitionStatus() const
{
    return ft_->PartitionStatus;
}

FailoverUnitHealthState::Enum FailoverUnitSnapshot::get_CurrentHealthState() const
{
    return ft_->CurrentHealthState;
}

FABRIC_SEQUENCE_NUMBER FailoverUnitSnapshot::get_HealthSequence() const
{
    return ft_->HealthSequence;
}

vector<ReplicaSnapshot> FailoverUnitSnapshot::GetReplicas() const
{
    vector<ReplicaSnapshot> rv;

    for(auto it = ft_->BeginIterator; it != ft_->EndIterator; ++it)
    {
        rv.push_back(ReplicaSnapshot(*it));
    }

    return rv;
}

vector<ReplicaSnapshot> FailoverUnitSnapshot::GetCurrentConfigurationReplicas() const
{
    vector<ReplicaSnapshot> rv;

    for(auto it = ft_->CurrentConfiguration.Replicas.begin(); it != ft_->CurrentConfiguration.Replicas.end(); ++it)
    {
        rv.push_back(ReplicaSnapshot(**it));
    }

    return rv;
}

ReplicaSnapshotUPtr FailoverUnitSnapshot::GetReplica(Federation::NodeId const & nodeId) const
{
    auto replica = ft_->GetReplica(nodeId);

    if (replica == nullptr)
    {
        return ReplicaSnapshotUPtr();
    }

    return make_unique<ReplicaSnapshot>(*replica);
}

ServiceSnapshot FailoverUnitSnapshot::GetServiceInfo() const
{
    return ServiceSnapshot(ft_->ServiceInfoObj);
}

bool FailoverUnitSnapshot::IsQuorumLost() const
{
    return ft_->IsQuorumLost();
}

FailoverUnitSnapshotUPtr FailoverUnitSnapshot::CloneToUPtr() const
{
    return make_unique<FailoverUnitSnapshot>(*ft_);
}

void FailoverUnitSnapshot::WriteTo(TextWriter& writer, FormatOptions const & options) const
{
    ft_->WriteTo(writer, options);
}

InBuildFailoverUnitSnapshot::InBuildFailoverUnitSnapshot(InBuildFailoverUnitUPtr const & ft)
: ft_(make_shared<InBuildFailoverUnit>(*ft))
{
}

void InBuildFailoverUnitSnapshot::WriteTo(TextWriter& writer, FormatOptions const & options) const
{
    ft_->WriteTo(writer, options);
}

ReplicaSnapshot::ReplicaSnapshot(Replica const & replica)
: replica_(make_shared<Replica>(replica))
{
}

bool ReplicaSnapshot::get_IsCurrentConfigurationPrimary() const
{
    return replica_->IsCurrentConfigurationPrimary;
}

bool ReplicaSnapshot::get_IsCurrentConfigurationSecondary() const
{
    return replica_->IsCurrentConfigurationSecondary;
}

Federation::NodeId ReplicaSnapshot::get_FederationNodeId() const
{
    return replica_->FederationNodeId;
}

Reliability::ReplicaRole::Enum ReplicaSnapshot::get_CurrentConfigurationRole() const
{
    return replica_->CurrentConfigurationRole;
}

bool ReplicaSnapshot::get_IsDropped() const
{
    return replica_->IsDropped;
}

bool ReplicaSnapshot::get_IsInBuild() const
{
    return replica_->IsInBuild;
}

bool ReplicaSnapshot::get_IsStandBy() const
{
    return replica_->IsStandBy;
}

bool ReplicaSnapshot::get_IsOffline() const
{
    return replica_->IsOffline;
}

bool ReplicaSnapshot::get_IsCreating() const
{
    return replica_->IsCreating;
}

bool ReplicaSnapshot::get_IsUp() const
{
    return replica_->IsUp;
}

int64 ReplicaSnapshot::get_InstanceId() const
{
    return replica_->InstanceId;
}

bool ReplicaSnapshot::get_IsToBeDropped() const
{
    return replica_->IsToBeDropped;
}

ServiceModel::ServicePackageVersionInstance const & ReplicaSnapshot::get_Version() const
{
    return replica_->VersionInstance;
}

std::wstring const & ReplicaSnapshot::get_ServiceLocation() const
{
    return replica_->ServiceLocation;
}

Reliability::ReplicaStates::Enum ReplicaSnapshot::get_State() const
{
    return replica_->ReplicaDescription.State;
}

Reliability::ReplicaStatus::Enum ReplicaSnapshot::get_ReplicaStatus() const
{
    return replica_->ReplicaDescription.ReplicaStatus;
}

int64 ReplicaSnapshot::get_ReplicaId() const
{
    return replica_->ReplicaId;
}

bool ReplicaSnapshot::get_IsInCurrentConfiguration() const
{
    return replica_->IsInCurrentConfiguration;
}

bool ReplicaSnapshot::get_IsInPreviousConfiguration() const
{
    return replica_->IsInPreviousConfiguration;
}

NodeSnapshot ReplicaSnapshot::GetNode() const
{
    return NodeSnapshot(replica_->NodeInfoObj);
}

void ReplicaSnapshot::WriteTo(Common::TextWriter& writer, Common::FormatOptions const & options) const
{
    replica_->WriteTo(writer, options);
}

NodeSnapshot::NodeSnapshot(NodeInfoSPtr const & node)
: node_(node)
{
}

NodeInstance NodeSnapshot::get_NodeInstance() const
{
    return node_->NodeInstance;
}

NodeId NodeSnapshot::get_Id() const
{
    return node_->Id;
}

NodeDeactivationInfo const & NodeSnapshot::get_DeactivationInfo() const
{
    return node_->DeactivationInfo;
}

bool NodeSnapshot::get_IsReplicaUploaded() const
{
    return node_->IsReplicaUploaded;
}

bool NodeSnapshot::get_IsPendingFabricUpgrade() const
{
    return node_->IsPendingFabricUpgrade;
}

bool NodeSnapshot::get_IsUp() const
{
    return node_->IsUp;
}

FABRIC_QUERY_NODE_STATUS NodeSnapshot::get_NodeStatus() const
{
    return node_->NodeStatus;
}

FabricVersionInstance const & NodeSnapshot::get_VersionInstance() const
{
    return node_->VersionInstance;
}

wstring const & NodeSnapshot::get_ActualUpgradeDomainId() const
{
    return node_->ActualUpgradeDomainId;
}

wstring const & NodeSnapshot::get_NodeName() const
{
    return node_->NodeName;
}

wstring const & NodeSnapshot::get_NodeType() const
{
    return node_->NodeType;
}

wstring const & NodeSnapshot::get_IpAddressOrFQDN() const
{
    return node_->IpAddressOrFQDN;
}

wstring NodeSnapshot::get_FaultDomainId() const
{
    return node_->FaultDomainId;
}

bool NodeSnapshot::get_IsNodeStateRemoved() const
{
    return node_->IsNodeStateRemoved;
}

bool NodeSnapshot::get_IsUnknown() const
{
    return node_->IsUnknown;
}

FABRIC_SEQUENCE_NUMBER NodeSnapshot::get_HealthSequence() const
{
    return node_->HealthSequence;
}

vector<Uri> const & NodeSnapshot::get_FaultDomainIds() const
{
    return node_->FaultDomainIds;
}

void NodeSnapshot::WriteTo(TextWriter& writer, FormatOptions const & options) const
{
    node_->WriteTo(writer, options);
}

ServiceSnapshot::ServiceSnapshot(ServiceInfoSPtr const & serviceInfo)
: service_(serviceInfo)
{

}

ServiceDescription const & ServiceSnapshot::get_ServiceDescription() const
{
    return service_->ServiceDescription;
}

wstring const & ServiceSnapshot::get_Name() const
{
    return service_->Name;
}

bool ServiceSnapshot::get_IsStale() const
{
    return service_->IsStale;
}

bool ServiceSnapshot::get_IsDeleted() const
{
    return service_->IsDeleted;
}

bool ServiceSnapshot::get_IsToBeDeleted() const
{
    return service_->IsToBeDeleted;
}

FABRIC_SEQUENCE_NUMBER ServiceSnapshot::get_HealthSequence() const
{
    return service_->HealthSequence;
}

FABRIC_QUERY_SERVICE_STATUS ServiceSnapshot::get_Status() const
{
    return service_->Status;
}

void ServiceSnapshot::WriteTo(TextWriter& writer, FormatOptions const & options) const
{
    service_->WriteTo(writer, options);
}

ServiceTypeSnapshot ServiceSnapshot::GetServiceType() const
{
    return ServiceTypeSnapshot(service_->ServiceType);
}

ServiceModel::ServicePackageVersionInstance ServiceSnapshot::GetTargetVersion(wstring const & upgradeDomain) const
{
    return service_->GetTargetVersion(upgradeDomain);
}

Reliability::LoadBalancingComponent::ServiceDescription ServiceSnapshot::GetPLBServiceDescription() const
{
    return service_->GetPLBServiceDescription();
}

LoadSnapshot::LoadSnapshot(LoadInfoSPtr load)
: load_(load)
{
}

void LoadSnapshot::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &options) const
{
    load_->WriteTo(writer, options);
}

ServiceTypeSnapshot::ServiceTypeSnapshot(ServiceTypeSPtr const & st)
: st_(st)
{
}

ServiceModel::ServiceTypeIdentifier const & ServiceTypeSnapshot::get_Type() const
{
    return st_->Type;
}

ApplicationSnapshot ServiceTypeSnapshot::GetApplication() const
{
    return ApplicationSnapshot(st_->Application);
}

void ServiceTypeSnapshot::WriteTo(Common::TextWriter& writer, Common::FormatOptions const & options) const
{
    st_->WriteTo(writer, options);
}

ApplicationSnapshot::ApplicationSnapshot(ApplicationInfoSPtr const & app)
: app_(app)
{
}

ServiceModel::ApplicationIdentifier const & ApplicationSnapshot::get_AppId() const
{
    return app_->ApplicationId;
}

bool ApplicationSnapshot::get_IsDeleted() const
{
    return app_->IsDeleted;
}

bool ApplicationSnapshot::get_IsUpgradeCompleted() const
{
    return !app_->Upgrade || app_->Upgrade->IsCompleted();
}

void ApplicationSnapshot::WriteTo(Common::TextWriter& writer, Common::FormatOptions const & options) const
{
    app_->WriteTo(writer, options);
}
