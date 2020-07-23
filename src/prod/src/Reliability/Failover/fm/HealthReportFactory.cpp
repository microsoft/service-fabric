// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;
using namespace Federation;

const wstring HealthReportFactory::documentationUrl_ = L"http://aka.ms/sfhealth";

HealthReportFactory::HealthReportFactory(const std::wstring & nodeId, bool isFMM, const Common::ConfigEntry<Common::TimeSpan> & stateTraceIntervalEntry):
    nodeId_(nodeId),
    isFMM_(isFMM),
    stateTraceIntervalEntry_(stateTraceIntervalEntry)
{
}

HealthReport HealthReportFactory::GenerateRebuildStuckHealthReport(const wstring & description)
{
    auto healthReportCode = isFMM_ ? SystemHealthReportCode::FMM_RebuildStuck : SystemHealthReportCode::FM_RebuildStuck;

    return HealthReport::CreateSystemHealthReport(
        healthReportCode,
        EntityHealthInformation::CreateClusterEntityHealthInformation(),
        description,
        SequenceNumber::GetNext(),
        FailoverConfig::GetConfig().RebuildHealthReportTTL,
        ServiceModel::AttributeList());
}

HealthReport HealthReportFactory::GenerateRebuildStuckHealthReport(Common::TimeSpan elapsedTime, Common::TimeSpan expectedTime)
{
    return GenerateRebuildStuckHealthReport(GenerateRebuildBroadcastStuckDescription(elapsedTime, expectedTime));
}

HealthReport HealthReportFactory::GenerateRebuildStuckHealthReport(vector<NodeInstance> & stuckNodes, Common::TimeSpan elapsedTime, Common::TimeSpan expectedTime)
{
    return GenerateRebuildStuckHealthReport(GenerateRebuildWaitingForNodesDescription(stuckNodes, elapsedTime, expectedTime));
}

HealthReport HealthReportFactory::GenerateClearRebuildStuckHealthReport()
{
    auto healthReportCode = isFMM_ ? SystemHealthReportCode::FMM_RebuildHealthy : SystemHealthReportCode::FM_RebuildHealthy;        
    return HealthReport::CreateSystemRemoveHealthReport(healthReportCode, EntityHealthInformation::CreateClusterEntityHealthInformation(), L"");
}

bool HealthReportFactory::ShouldIncludeDocumentationUrl(SystemHealthReportCode::Enum reportCode)
{
    return ((reportCode == SystemHealthReportCode::FM_NodeDown) ||
        (reportCode == SystemHealthReportCode::FM_SeedNodeDown));
}

HealthReport HealthReportFactory::GenerateNodeInfoHealthReport(NodeInfoSPtr nodeInfo, bool isSeedNode, bool isUpgrade)
{
    auto nodeEntityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
        nodeInfo->Id.IdValue,
        nodeInfo->NodeName,
        nodeInfo->NodeInstance.InstanceId);

    if (nodeInfo->IsNodeStateRemoved || nodeInfo->IsUnknown)
    {
        return HealthReport::CreateSystemDeleteEntityHealthReport(
            SystemHealthReportCode::FM_DeleteNode,
            move(nodeEntityInfo),
            nodeInfo->HealthSequence);
    }

    AttributeList attributeList;
    PopulateNodeAttributeList(attributeList, nodeInfo);
    
    auto reportCode = nodeInfo->IsUp ?
        SystemHealthReportCode::FM_NodeUp :
        (isUpgrade ?
            SystemHealthReportCode::FM_NodeDownDuringUpgrade :
            isSeedNode ? SystemHealthReportCode::FM_SeedNodeDown : SystemHealthReportCode::FM_NodeDown);
    return HealthReport::CreateSystemHealthReport(
        reportCode,
        move(nodeEntityInfo),
        ShouldIncludeDocumentationUrl(reportCode) ?
          documentationUrl_ :
          L"" /*extraDescription*/,
        nodeInfo->HealthSequence,
        move(attributeList));
}

HealthReport HealthReportFactory::GenerateNodeDeactivationHealthReport(NodeInfoSPtr nodeInfo, bool isSeedNode, bool isDeactivationComplete)
{
    FABRIC_SEQUENCE_NUMBER sequence = (isFMM_ ? SequenceNumber::GetNext() : nodeInfo->HealthSequence);
    TimeSpan ttl = (isFMM_ ? TimeSpan::FromTicks(stateTraceIntervalEntry_.GetValue().Ticks * 3) : TimeSpan::MaxValue);

    auto nodeEntityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
        nodeInfo->Id.IdValue,
        nodeInfo->NodeName,
        nodeInfo->NodeInstance.InstanceId);

    SystemHealthReportCode::Enum reportCode;     

    if (isDeactivationComplete)
    {
        if (isFMM_)
        {
            return HealthReport::CreateSystemRemoveHealthReport(
                SystemHealthReportCode::FMM_NodeDeactivateStuck,
                move(nodeEntityInfo),
                wstring(),
                sequence);
        }

        reportCode = nodeInfo->IsUp ?
            SystemHealthReportCode::FM_NodeUp :
            isSeedNode ? SystemHealthReportCode::FM_SeedNodeDown : SystemHealthReportCode::FM_NodeDown;
    }
    else
    {
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_NodeDeactivateStuck : SystemHealthReportCode::FM_NodeDeactivateStuck;
    }

    AttributeList attributeList;
    PopulateNodeAttributeList(attributeList, nodeInfo);

    return HealthReport::CreateSystemHealthReport(
        reportCode,
        move(nodeEntityInfo),
        ShouldIncludeDocumentationUrl(reportCode) ?
          documentationUrl_ :
          L"" /*extraDescription*/,
        sequence,
        ttl,
        move(attributeList));
}

HealthReport HealthReportFactory::GenerateServiceInfoHealthReport(ServiceInfoSPtr serviceInfo)
{
    wstring publicName = SystemServiceApplicationNameHelper::GetPublicServiceName(serviceInfo->Name);
    wstring appName = StringUtility::StartsWith<wstring>(publicName, *SystemServiceApplicationNameHelper::SystemServiceApplicationName) ? SystemServiceApplicationNameHelper::SystemServiceApplicationName : serviceInfo->ServiceDescription.ApplicationName;
    FABRIC_SEQUENCE_NUMBER sequence = serviceInfo->HealthSequence;
    auto serviceEntityInfo = EntityHealthInformation::CreateServiceEntityHealthInformation(publicName, serviceInfo->Instance);

    if (serviceInfo->IsDeleted)
    {
        return HealthReport::CreateSystemDeleteEntityHealthReport(
            SystemHealthReportCode::FM_DeleteService,
            move(serviceEntityInfo),
            sequence);
    }

    AttributeList attributes;
    attributes.AddAttribute(*HealthAttributeNames::ApplicationName, appName);
    attributes.AddAttribute(*HealthAttributeNames::ServiceTypeName, serviceInfo->ServiceDescription.Type.ServiceTypeName);

    return HealthReport::CreateSystemHealthReport(
        SystemHealthReportCode::FM_ServiceCreated,
        move(serviceEntityInfo),
        L"" /*extraDescription*/,
        sequence,
        move(attributes));
}

HealthReport HealthReportFactory::GenerateInBuildFailoverUnitHealthReport(InBuildFailoverUnit const & inBuildFailoverUnit, bool isDeleted)
{
    FABRIC_SEQUENCE_NUMBER sequence = (isFMM_ ? SequenceNumber::GetNext() : inBuildFailoverUnit.HealthSequence);
    TimeSpan ttl = (isFMM_ ? TimeSpan::FromTicks(stateTraceIntervalEntry_.GetValue().Ticks * 3) : TimeSpan::MaxValue);
    auto partitionInfo = EntityHealthInformation::CreatePartitionEntityHealthInformation(inBuildFailoverUnit.Id.Guid);

    if (isDeleted)
    {
        return HealthReport::CreateSystemDeleteEntityHealthReport(
            isFMM_ ? SystemHealthReportCode::FMM_DeletePartition : SystemHealthReportCode::FM_DeletePartition,
            move(partitionInfo),
            sequence);
    }

    AttributeList attributes;
    attributes.AddAttribute(*HealthAttributeNames::ServiceName, SystemServiceApplicationNameHelper::GetPublicServiceName(inBuildFailoverUnit.Description.Name));

    SystemHealthReportCode::Enum reportCode = (isFMM_ ? SystemHealthReportCode::FMM_PartitionRebuildStuck : SystemHealthReportCode::FM_PartitionRebuildStuck);

    return HealthReport::CreateSystemHealthReport(
        reportCode,
        move(partitionInfo),
        wstring(),
        sequence,
        ttl,
        move(attributes));
}

HealthReport HealthReportFactory::GenerateFailoverUnitHealthReport(FailoverUnit const & failoverUnit)
{
    FABRIC_SEQUENCE_NUMBER sequence = (isFMM_ ? SequenceNumber::GetNext() : failoverUnit.HealthSequence);
    TimeSpan ttl = (isFMM_ ? TimeSpan::FromTicks(stateTraceIntervalEntry_.GetValue().Ticks * 3) : TimeSpan::MaxValue);
    auto partitionInfo = EntityHealthInformation::CreatePartitionEntityHealthInformation(failoverUnit.Id.Guid);

    if (failoverUnit.IsOrphaned)
    {
        return HealthReport::CreateSystemDeleteEntityHealthReport(
            isFMM_ ? SystemHealthReportCode::FMM_DeletePartition : SystemHealthReportCode::FM_DeletePartition,
            move(partitionInfo),
            sequence);
    }

    AttributeList attributes;
    attributes.AddAttribute(*HealthAttributeNames::ServiceName, SystemServiceApplicationNameHelper::GetPublicServiceName(failoverUnit.ServiceName));

    SystemHealthReportCode::Enum reportCode;
    switch (failoverUnit.CurrentHealthState)
    {
    case FailoverUnitHealthState::QuorumLost:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionQuorumLoss : SystemHealthReportCode::FM_PartitionQuorumLoss;
        break;
    case FailoverUnitHealthState::ReconfigurationStuck:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionReconfigurationStuck : SystemHealthReportCode::FM_PartitionReconfigurationStuck;
        break;
    case FailoverUnitHealthState::BuildStuck:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionBuildStuck : SystemHealthReportCode::FM_PartitionBuildStuck;
        break;
    case FailoverUnitHealthState::BuildStuckBelowMinReplicaCount:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionBuildStuckBelowMinReplicaCount : SystemHealthReportCode::FM_PartitionBuildStuckBelowMinReplicaCount;
        break;
    case FailoverUnitHealthState::PlacementStuck:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionPlacementStuck : SystemHealthReportCode::FM_PartitionPlacementStuck;
        break;
    case FailoverUnitHealthState::PlacementStuckBelowMinReplicaCount:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionPlacementStuckBelowMinReplicaCount : SystemHealthReportCode::FM_PartitionPlacementStuckBelowMinReplicaCount;
        break;
    case FailoverUnitHealthState::PlacementStuckAtZeroReplicaCount:
        reportCode = SystemHealthReportCode::FM_PartitionPlacementStuckAtZeroReplicaCount;
        break;
    case FailoverUnitHealthState::DeletionInProgress:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionDeletionInProgress : SystemHealthReportCode::FM_PartitionDeletionInProgress;
        break;
    default:
        reportCode = isFMM_ ? SystemHealthReportCode::FMM_PartitionHealthy : SystemHealthReportCode::FM_PartitionHealthy;
        break;
    }

    return HealthReport::CreateSystemHealthReport(
        reportCode,
        move(partitionInfo),
        failoverUnit.ExtraDescription,
        sequence,
        ttl,
        move(attributes));
}


wstring HealthReportFactory::GenerateRebuildBroadcastStuckDescription(Common::TimeSpan elapsedTime, Common::TimeSpan expectedTime)
{
    wstring description;
    StringWriter writer(description);

    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_Stuck_Broadcast), nodeId_));
    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_Time_Info), elapsedTime, expectedTime));
    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_More_Info), documentationUrl_));

    return description;
}

wstring HealthReportFactory::GenerateRebuildWaitingForNodesDescription(vector<Federation::NodeInstance> & stuckNodes, Common::TimeSpan elapsedTime, Common::TimeSpan expectedTime)
{
    wstring description;    
    StringWriter writer(description);
    
    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_Stuck_Nodes), nodeId_));

    bool first = true;
    for (auto const & node : stuckNodes)
    {
        if (first)
        {
            first = false;
            writer.Write(" {0}", node.Id);
        }
        else
        {
            writer.Write(", {0}", node.Id);
        }
    }

    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_Time_Info), elapsedTime, expectedTime));
    writer.Write(wformatString(Common::StringResource::Get(IDS_FM_Rebuild_More_Info), documentationUrl_));

    return description;
}

void HealthReportFactory::PopulateNodeAttributeList(AttributeList & attributes, const NodeInfoSPtr & nodeInfo)
{
    attributes.AddAttribute(*HealthAttributeNames::IpAddressOrFqdn, nodeInfo->IpAddressOrFQDN);
    attributes.AddAttribute(*HealthAttributeNames::UpgradeDomain, nodeInfo->ActualUpgradeDomainId);
    attributes.AddAttribute(*HealthAttributeNames::FaultDomain, nodeInfo->FaultDomainId);

    if (!nodeInfo->NodeName.empty())
    {
        attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeInfo->NodeName);
    }
}
