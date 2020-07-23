// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("HealthReport");

INITIALIZE_SIZE_ESTIMATION(HealthReport)

GlobalWString HealthReport::Delimiter = make_global<wstring>(L"+");
Common::Global<Common::TimeSpan> HealthReport::DefaultDeleteTTL = make_global<TimeSpan>(TimeSpan::FromMinutes(120));

Priority::Enum HealthReport::get_ReportPriority() const
{
    if (priority_ == Priority::NotAssigned)
    {
        if (this->IsForDelete)
        {
            priority_ = Priority::Critical;
        }
        else
        {
            priority_ = GetPriority(entityInformation_->Kind, sourceId_, property_);
        }
    }

    return priority_;
}

Common::StringCollection HealthReport::GetAuthoritySources(FABRIC_HEALTH_REPORT_KIND kind)
{
    StringCollection authoritySources;
    switch (kind)
    {
    case FABRIC_HEALTH_REPORT_KIND_NODE:
    case FABRIC_HEALTH_REPORT_KIND_SERVICE:
    case FABRIC_HEALTH_REPORT_KIND_PARTITION:
        authoritySources.push_back(*Constants::HealthReportFMSource);
        authoritySources.push_back(*Constants::HealthReportFMMSource);
        break;

    case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
        authoritySources.push_back(*Constants::HealthReportCMSource);
        break;

    case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
    case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
        authoritySources.push_back(*Constants::HealthReportRASource);
        break;

    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
        authoritySources.push_back(*Constants::HealthReportHostingSource);
        break;

    default:
        // do nothing
        break;
    }

    return move(authoritySources);
}

Priority::Enum HealthReport::GetPriority(FABRIC_HEALTH_REPORT_KIND kind, std::wstring const & sourceId, std::wstring const & property)
{
    if (!StringUtility::StartsWith<wstring>(sourceId, *Constants::EventSystemSourcePrefix))
    {
        // User report
        return Priority::Normal;
    }

    // System report: if the report is authority report, return Critical;
    // If it's from an authority source, return Higher. Otherwise, return High.
    switch (kind)
    {
    case FABRIC_HEALTH_REPORT_KIND_NODE:
    case FABRIC_HEALTH_REPORT_KIND_SERVICE:
    case FABRIC_HEALTH_REPORT_KIND_PARTITION:
        if (sourceId == *Constants::HealthReportFMSource || sourceId == *Constants::HealthReportFMMSource)
        {
            return (property == *Constants::HealthStateProperty) ? Priority::Critical : Priority::Higher;
        }

        return Priority::High;

    case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
        if (sourceId == *Constants::HealthReportCMSource)
        {
            return (property == *Constants::HealthStateProperty) ? Priority::Critical : Priority::Higher;
        }

        return Priority::High;

    case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
    case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
        if (sourceId == *Constants::HealthReportRASource)
        {
            return (property == *Constants::HealthStateProperty) ? Priority::Critical : Priority::Higher;
        }

        return Priority::High;

    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
    case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
        if (sourceId == *Constants::HealthReportHostingSource)
        {
            return (property == *Constants::HealthActivationProperty) ? Priority::Critical : Priority::Higher;
        }

        return Priority::High;

    default:
        return Priority::High;
    }
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::TimeSpan const timeToLive,
    AttributeList && attributeList)
{
    wstring sourceId;
    wstring property;
    FABRIC_HEALTH_STATE healthState;
    bool removeWhenExpired = false;
    wstring description;
    switch(reportCode)
    {
    case SystemHealthReportCode::CM_ApplicationCreated:
        sourceId = *Constants::HealthReportCMSource;
        property = *Constants::HealthStateProperty;
        description = HMResource::GetResources().ApplicationCreated;
        healthState = FABRIC_HEALTH_STATE_OK;
        break;

    case SystemHealthReportCode::CM_ApplicationUpdated:
        sourceId = *Constants::HealthReportCMSource;
        property = *Constants::HealthStateProperty;
        description = HMResource::GetResources().ApplicationUpdated;
        healthState = FABRIC_HEALTH_STATE_OK;
        break;

    case SystemHealthReportCode::RA_ReconfigurationStuckWarning:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::ReconfigurationProperty;
        description = HMResource::GetResources().HealthReconfigurationStatusWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::RA_ReconfigurationHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::ReconfigurationProperty;
        description = HMResource::GetResources().HealthReconfigurationStatusHealthy;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RA_ReplicaCreated:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthStateProperty;
        description = HMResource::GetResources().HealthReplicaCreated;
        healthState = FABRIC_HEALTH_STATE_OK;
        break;

    case SystemHealthReportCode::RA_ReplicaOpenStatusWarning:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaOpenStatusProperty;
        description = HMResource::GetResources().HealthReplicaOpenStatusWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::RA_ReplicaOpenStatusHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaOpenStatusProperty;
        description = HMResource::GetResources().HealthReplicaOpenStatusHealthy;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RA_ReplicaCloseStatusWarning:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaCloseStatusProperty;
        description = HMResource::GetResources().HealthReplicaCloseStatusWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::RA_ReplicaCloseStatusHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaCloseStatusProperty;
        description = HMResource::GetResources().HealthReplicaCloseStatusHealthy;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusWarning:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaServiceTypeRegistrationStatusProperty;
        description = HMResource::GetResources().HealthReplicaServiceTypeRegistrationStatusWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaServiceTypeRegistrationStatusProperty;
        description = HMResource::GetResources().HealthReplicaServiceTypeRegistrationStatusHealthy;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RA_StoreProviderHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthRAStoreProvider;
        healthState = FABRIC_HEALTH_STATE_OK;
        break;

    case SystemHealthReportCode::RA_StoreProviderUnhealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthRAStoreProvider;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::RA_ReplicaChangeRoleStatusError:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaChangeRoleStatusProperty;
        description = HMResource::GetResources().HealthReplicaChangeRoleStatusError;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        break;

    case SystemHealthReportCode::RA_ReplicaChangeRoleStatusHealthy:
        sourceId = *Constants::HealthReportRASource;
        property = *Constants::HealthReplicaChangeRoleStatusProperty;
        description = HMResource::GetResources().HealthReplicaChangeStatusHealthy;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FMM_NodeDeactivateStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().NodeDeactivateStuck;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FMM_PartitionQuorumLoss:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionQuorumLoss;
        break;

    case SystemHealthReportCode::FMM_PartitionBuildStuckBelowMinReplicaCount:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionBuildStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionPlacementStuckBelowMinReplicaCount:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionPlacementStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionReconfigurationStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionReconfigurationStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionBuildStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionBuildStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionRebuildStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionRebuildStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionPlacementStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionPlacementStuck;
        break;

    case SystemHealthReportCode::FMM_PartitionDeletionInProgress:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().DeletionInProgress;
        break;

    case SystemHealthReportCode::FMM_PartitionHealthy:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().PartitionHealthy;
        break;

    case SystemHealthReportCode::FMM_ServiceCreated:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().ServiceCreated;
        break;

    case SystemHealthReportCode::FMM_RebuildStuck:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::RebuildProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().FMMRebuildStuck;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FMM_RebuildHealthy:
        sourceId = *Constants::HealthReportFMMSource;
        property = *Constants::RebuildProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().FMMRebuildHealthy;
        removeWhenExpired = true;
        break;        

    case SystemHealthReportCode::FM_PartitionQuorumLoss:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionQuorumLoss;
        break;

    case SystemHealthReportCode::FM_PartitionBuildStuckBelowMinReplicaCount:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionBuildStuck;
        break;

    case SystemHealthReportCode::FM_PartitionPlacementStuckBelowMinReplicaCount:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionPlacementStuck;
        break;

    case SystemHealthReportCode::FM_PartitionPlacementStuckAtZeroReplicaCount:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().PartitionPlacementStuck;
        break;

    case SystemHealthReportCode::FM_PartitionReconfigurationStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionReconfigurationStuck;
        break;

    case SystemHealthReportCode::FM_PartitionBuildStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionBuildStuck;
        break;

    case SystemHealthReportCode::FM_PartitionRebuildStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionRebuildStuck;
        break;

    case SystemHealthReportCode::FM_PartitionPlacementStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().PartitionPlacementStuck;
        break;

    case SystemHealthReportCode::FM_ServiceCreated:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().ServiceCreated;
        break;

    case SystemHealthReportCode::FM_NodeDeactivateStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().NodeDeactivateStuck;
        break;

    case SystemHealthReportCode::FM_NodeUp:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().NodeUp;
        break;

    case SystemHealthReportCode::FM_PartitionHealthy:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().PartitionHealthy;
        break;

    case SystemHealthReportCode::FM_PartitionDeletionInProgress:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().DeletionInProgress;
        break;

    case SystemHealthReportCode::FM_NodeDown:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().NodeDown;
        break;

    case SystemHealthReportCode::FM_SeedNodeDown:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().SeedNodeDown;
        break;

    case SystemHealthReportCode::FM_NodeDownDuringUpgrade:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().NodeDownDuringUpgrade;
        break;

    case SystemHealthReportCode::FM_RebuildStuck:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::RebuildProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().FMRebuildStuck;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FM_RebuildHealthy:
        sourceId = *Constants::HealthReportFMSource;
        property = *Constants::RebuildProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().FMRebuildHealthy;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RAP_ApiOk:
        sourceId = *Constants::HealthReportRAPSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().HealthStartTime;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RAP_ApiSlow:
        sourceId = *Constants::HealthReportRAPSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().HealthStartTime;
        break;

    case SystemHealthReportCode::FabricNode_CertificateOk:
        sourceId = *Constants::HealthReportFabricNodeSource;
        property = *Constants::FabricCertificateProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().CertificateExpiration;
        break;

    case SystemHealthReportCode::FabricNode_CertificateCloseToExpiration:
        sourceId = *Constants::HealthReportFabricNodeSource;
        property = *Constants::FabricCertificateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().CertificateExpiration;
        break;

    case SystemHealthReportCode::FabricNode_CertificateRevocationListOffline:
        sourceId = *Constants::HealthReportFabricNodeSource;
        property = *Constants::FabricCertificateRevocationListProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().CertificateRevocationList;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FabricNode_CertificateRevocationListOk:
        sourceId = *Constants::HealthReportFabricNodeSource;
        property = *Constants::FabricCertificateRevocationListProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().CertificateRevocationList;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::FabricNode_SecurityApiSlow:
        sourceId = *Constants::HealthReportFabricNodeSource;
        property = *Constants::SecurityApiProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().SecurityApi;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::Federation_NeighborhoodLost:
        sourceId = *Constants::HealthReportFederationSource;
        property = *Constants::NeighborhoodProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().NeighborHoodLost;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::Federation_NeighborhoodOk:
        sourceId = *Constants::HealthReportFederationSource;
        property = *Constants::NeighborhoodProperty;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().NeighborHoodLost;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::Hosting_DeleteEvent:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        // No description for delete event
        break;

    case SystemHealthReportCode::Hosting_ServiceTypeRegistered:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().ServiceTypeRegistered;
        break;

    case SystemHealthReportCode::Hosting_CodePackageActivated:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().CodePackageActivated;
        break;

    case SystemHealthReportCode::Hosting_ApplicationActivated:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().ApplicationActivated;
        break;

    case SystemHealthReportCode::Hosting_ServicePackageActivated:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        description = HMResource::GetResources().ServicePackageActivated;
        break;

    case SystemHealthReportCode::Hosting_ServiceTypeRegistrationTimeout:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().ServiceTypeRegistrationTimeout;
        break;

    case SystemHealthReportCode::Hosting_ServiceTypeDisabled:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().ServiceTypeDisabled;
        break;

    case SystemHealthReportCode::Hosting_ActivationFailed:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().ActivationFailed;
        break;

    case SystemHealthReportCode::Hosting_CodePackageActivationError:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().CodePackageActivationFailed;
        break;

    case SystemHealthReportCode::Hosting_DownloadFailed:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().DownloadFailed;
        break;

    case SystemHealthReportCode::Hosting_ServiceTypeUnregistered:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().ServiceTypeUnregistered;
        break;

    case SystemHealthReportCode::Hosting_CodePackageActivationWarning:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().CodePackageActivationFailed;
        break;

    case SystemHealthReportCode::Hosting_FabricUpgradeValidationFailed:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().FabricUpgradeValidationFailed;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::Hosting_FabricUpgradeFailed:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        description = HMResource::GetResources().FabricUpgradeFailed;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::Hosting_AvailableResourceCapacityMismatch:
        sourceId = *Constants::HealthReportHostingSource;
        property = *Constants::ResourceGovernanceReportProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().AvailableResourceCapacityMismatch;
        break;

    case SystemHealthReportCode::Hosting_AvailableResourceCapacityNotDefined:
        sourceId = *Constants::HealthReportHostingSource;
        property = *Constants::ResourceGovernanceReportProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        description = HMResource::GetResources().AvailableResourceCapacityNotDefined;
        break;

    case SystemHealthReportCode::Hosting_DockerHealthCheckStatusHealthy:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        break;

    case SystemHealthReportCode::Hosting_DockerHealthCheckStatusUnhealthy:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        break;

    case SystemHealthReportCode::Hosting_DockerDaemonUnhealthy:
        sourceId = *Constants::HealthReportHostingSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_NodeCapacityViolation:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::HealthCapacityProperty;
        description = HMResource::GetResources().PLBNodeCapacityViolationWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_NodeCapacityOK:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::HealthCapacityProperty;
        description = HMResource::GetResources().PLBNodeCapacityOK;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_UnplacedReplicaViolation:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ServiceReplicaUnplacedHealthProperty;
        description = HMResource::GetResources().PLBUnplacedReplicaViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_ReplicaConstraintViolation:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ReplicaConstraintViolationProperty;
        description = HMResource::GetResources().PLBReplicaConstraintViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_ReplicaSoftConstraintViolation:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ReplicaConstraintViolationProperty;
        description = HMResource::GetResources().PLBReplicaSoftConstraintViolation;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_UpgradePrimarySwapViolation:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::UpgradePrimarySwapHealthProperty;
        description = HMResource::GetResources().PLBUpgradePrimarySwapViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_BalancingUnsuccessful:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::BalancingUnsuccessfulProperty;
        description = HMResource::GetResources().PLBBalancingUnsuccessful;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_ConstraintFixUnsuccessful:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ConstraintFixUnsuccessfulProperty;
        description = HMResource::GetResources().PLBConstraintFixUnsuccessful;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_MovementsDropped:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::MovementEfficacyProperty;
        description = HMResource::GetResources().PLBMovementsDropped;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::PLB_ServiceCorrelationError:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ServiceDescriptionHealthProperty;
        description = HMResource::GetResources().PLBServiceCorrelationError;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        removeWhenExpired = false;
        break;

    case SystemHealthReportCode::PLB_ServiceDescriptionOK:
        sourceId = *Constants::HealthReportPLBSource;
        property = *Constants::ServiceDescriptionHealthProperty;
        description = HMResource::GetResources().PLBServiceDescriptionOK;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_NodeCapacityViolation:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::HealthCapacityProperty;
        description = HMResource::GetResources().PLBNodeCapacityViolationWarning;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_NodeCapacityOK:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::HealthCapacityProperty;
        description = HMResource::GetResources().PLBNodeCapacityOK;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_UnplacedReplicaViolation:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ServiceReplicaUnplacedHealthProperty;
        description = HMResource::GetResources().PLBUnplacedReplicaViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_ReplicaConstraintViolation:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ReplicaConstraintViolationProperty;
        description = HMResource::GetResources().PLBReplicaConstraintViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_ReplicaSoftConstraintViolation:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ReplicaConstraintViolationProperty;
        description = HMResource::GetResources().PLBReplicaSoftConstraintViolation;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_UpgradePrimarySwapViolation:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::UpgradePrimarySwapHealthProperty;
        description = HMResource::GetResources().PLBUpgradePrimarySwapViolation;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_BalancingUnsuccessful:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::BalancingUnsuccessfulProperty;
        description = HMResource::GetResources().PLBBalancingUnsuccessful;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_ConstraintFixUnsuccessful:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ConstraintFixUnsuccessfulProperty;
        description = HMResource::GetResources().PLBConstraintFixUnsuccessful;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_MovementsDropped:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::MovementEfficacyProperty;
        description = HMResource::GetResources().PLBMovementsDropped;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::CRM_ServiceCorrelationError:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ServiceDescriptionHealthProperty;
        description = HMResource::GetResources().PLBServiceCorrelationError;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        removeWhenExpired = false;
        break;

    case SystemHealthReportCode::CRM_ServiceDescriptionOK:
        sourceId = *Constants::HealthReportCRMSource;
        property = *Constants::ServiceDescriptionHealthProperty;
        description = HMResource::GetResources().PLBServiceDescriptionOK;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::RE_QueueFull:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = false;
        ASSERT_IF(extraDescription.empty(), "Replicator heath report description cannot be empty");
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::RE_QueueOk:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;
    
    case SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusOk:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;
    
    case SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusFailed:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::RE_ApiSlow:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = false;
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::RE_ApiOk:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_OK;
        removeWhenExpired = true;
        ASSERT_IF(dynamicProperty.empty(), "Replicator heath report dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::Testability_UnreliableTransportBehaviorTimeOut:
        sourceId = *Constants::HealthReportTestabilitySource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::REStore_Unexpected:
        sourceId = *Constants::HealthReportReplicatedStoreSource;
        property = *Constants::HealthStateProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::REStore_SlowCommit:
        sourceId = *Constants::HealthReportReplicatedStoreSource;
        property = *Constants::CommitPerformanceHealthProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::REStore_PathTooLong:
        sourceId = *Constants::HealthReportReplicatedStoreSource;
        property = *Constants::HealthReplicaOpenStatusProperty;
        healthState = FABRIC_HEALTH_STATE_ERROR;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::REStore_AutoCompaction:
        sourceId = *Constants::HealthReportReplicatedStoreSource;
        property = *Constants::HealthReplicaOpenStatusProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = (timeToLive != TimeSpan::MaxValue);
        break;

    case SystemHealthReportCode::Naming_OperationSlow:
        sourceId = *Constants::HealthReportNamingServiceSource;
        property = *Constants::DurationHealthProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        ASSERT_IF(extraDescription.empty(), "Naming_OperationSlow extra description cannot be empty");
        ASSERT_IF(dynamicProperty.empty(), "Naming_OperationSlow dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::Naming_OperationSlowCompleted:
        sourceId = *Constants::HealthReportNamingServiceSource;
        property = *Constants::DurationHealthProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        ASSERT_IF(extraDescription.empty(), "Naming_OperationSlowCompleted extra description cannot be empty");
        ASSERT_IF(dynamicProperty.empty(), "Naming_OperationSlowCompleted dynamic property cannot be empty");
        break;

    case SystemHealthReportCode::Federation_LeaseTransportDelay:
        sourceId = *Constants::HealthReportFederationSource;
        property = *Constants::DurationHealthProperty;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        break;

    case SystemHealthReportCode::TR_SlowIO:
        sourceId = *Constants::HealthReportReplicatorSource;
        healthState = FABRIC_HEALTH_STATE_WARNING;
        removeWhenExpired = true;
        ASSERT_IF(dynamicProperty.empty(), "Transactional Replicator heath report dynamic property cannot be empty");
        break;

    default:
        Assert::CodingError("Unsupported reportCode {0}", reportCode);
    }

    if (!extraDescription.empty())
    {
        description += extraDescription;
    }

    if (!dynamicProperty.empty())
    {
        property += dynamicProperty;
    }

    HealthReport report(
        move(entityInformation),
        move(sourceId),
        move(property),
        timeToLive,
        healthState,
        move(description),
        sequenceNumber,
        removeWhenExpired,
        move(attributeList));
    auto error = report.TryAcceptUserOrSystemReport();
    ASSERT_IFNOT(error.IsSuccess(), "CreateSystemHealthReport: error validating the error report: {0} {1}", error, error.Message);

    return report;
}

HealthReport HealthReport::CreateSystemDeleteEntityHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    wstring sourceId;
    switch(reportCode)
    {
    case SystemHealthReportCode::FM_DeleteService:
    case SystemHealthReportCode::FM_DeleteNode:
    case SystemHealthReportCode::FM_DeletePartition:
        sourceId = *Constants::HealthReportFMSource;
        break;

    case SystemHealthReportCode::FMM_DeletePartition:
        sourceId = *Constants::HealthReportFMMSource;
        break;

    case SystemHealthReportCode::CM_DeleteApplication:
        sourceId = *Constants::HealthReportCMSource;
        break;

    case SystemHealthReportCode::RA_DeleteReplica:
        sourceId = *Constants::HealthReportRASource;
        break;

    case SystemHealthReportCode::Hosting_DeleteDeployedApplication:
    case SystemHealthReportCode::Hosting_DeleteDeployedServicePackage:
        sourceId = *Constants::HealthReportHostingSource;
        break;

    default:
        Assert::CodingError("Unsupported reportCode {0}", reportCode);
    }

    return HealthReport::CreateHealthInformationForDelete(
        move(entityInformation),
        move(sourceId),
        sequenceNumber);
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & extraDescription,
    AttributeList && attributeList)
{
    FABRIC_SEQUENCE_NUMBER sequenceNumber = static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext());
    return CreateSystemHealthReport(reportCode, move(entityInformation), L""/*dynamicProperty*/, extraDescription, sequenceNumber, TimeSpan::MaxValue, move(attributeList));
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    AttributeList && attributeList)
{
    return CreateSystemHealthReport(reportCode, move(entityInformation), L"" /*dynamicProperty*/, extraDescription, sequenceNumber, TimeSpan::MaxValue, move(attributeList));
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::TimeSpan const timeToLive,
    AttributeList && attributeList)
{
    return CreateSystemHealthReport(reportCode, move(entityInformation), L"" /*dynamicProperty*/, extraDescription, sequenceNumber, timeToLive, move(attributeList));
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty,
    std::wstring const & extraDescription,
    AttributeList && attributeList)
{
    FABRIC_SEQUENCE_NUMBER sequenceNumber = static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext());
    return CreateSystemHealthReport(reportCode, move(entityInformation), dynamicProperty, extraDescription, sequenceNumber, TimeSpan::MaxValue, move(attributeList));
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty,
    std::wstring const & extraDescription,
    Common::TimeSpan const timeToLive,
    AttributeList && attributeList)
{
    FABRIC_SEQUENCE_NUMBER sequenceNumber = static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext());
    return CreateSystemHealthReport(reportCode, move(entityInformation), dynamicProperty, extraDescription, sequenceNumber, timeToLive, move(attributeList));
}

HealthReport HealthReport::CreateSystemHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    AttributeList && attributeList)
{
    return CreateSystemHealthReport(reportCode, move(entityInformation), dynamicProperty, extraDescription, sequenceNumber, TimeSpan::MaxValue, move(attributeList));
}

HealthReport HealthReport::CreateSystemRemoveHealthReport(
    Common::SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty)
{
    AttributeList attributeList;
    FABRIC_SEQUENCE_NUMBER sequenceNumber = static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext());
    TimeSpan ttl = TimeSpan::FromSeconds(1);
    return CreateSystemHealthReport(reportCode, move(entityInformation), dynamicProperty, L"", sequenceNumber, ttl, move(attributeList));
}

HealthReport HealthReport::CreateSystemRemoveHealthReport(
    Common::SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & dynamicProperty,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    AttributeList attributeList;
    TimeSpan ttl = TimeSpan::FromMilliseconds(100);
    return CreateSystemHealthReport(reportCode, move(entityInformation), dynamicProperty, L"", sequenceNumber, ttl, move(attributeList));
}

HealthReport HealthReport::CreateSystemDeleteEntityHealthReport(
    SystemHealthReportCode::Enum reportCode,
    EntityHealthInformationSPtr && entityInformation)
{
    FABRIC_SEQUENCE_NUMBER sequenceNumber = static_cast<FABRIC_SEQUENCE_NUMBER>(SequenceNumber::GetNext());
    return CreateSystemDeleteEntityHealthReport(reportCode, move(entityInformation), sequenceNumber);
}

HealthReport::HealthReport()
    : HealthInformation()
    , entityInformation_()
    , attributeList_()
    , entityPropertyId_()
    , priority_(Priority::NotAssigned)
{
}

HealthReport::HealthReport(
    EntityHealthInformationSPtr && entityInformation,
    std::wstring const & sourceId,
    std::wstring const & property,
    Common::TimeSpan const & timeToLive,
    FABRIC_HEALTH_STATE state,
    std::wstring const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    bool removeWhenExpired,
    AttributeList && attributes)
    : HealthInformation(sourceId, property, timeToLive, state, description, sequenceNumber, DateTime::Now(), removeWhenExpired)
    , entityInformation_(move(entityInformation))
    , attributeList_(move(attributes))
    , entityPropertyId_()
    , priority_(Priority::NotAssigned)
{
}

HealthReport::HealthReport(
    EntityHealthInformationSPtr && entityInformation,
    std::wstring && sourceId,
    std::wstring && property,
    Common::TimeSpan const & timeToLive,
    FABRIC_HEALTH_STATE state,
    std::wstring const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    bool removeWhenExpired,
    AttributeList && attributes)
    : HealthInformation(move(sourceId), move(property), timeToLive, state, description, sequenceNumber, DateTime::Now(), removeWhenExpired)
    , entityInformation_(move(entityInformation))
    , attributeList_(move(attributes))
    , entityPropertyId_()
    , priority_(Priority::NotAssigned)
{
}

HealthReport::HealthReport(
    EntityHealthInformationSPtr && entityInformation,
    HealthInformation && healthInfo,
    AttributeList && attributes)
    : HealthInformation(move(healthInfo))
    , entityInformation_(move(entityInformation))
    , attributeList_(move(attributes))
    , entityPropertyId_()
    , priority_(Priority::NotAssigned)
{
}

HealthReport HealthReport::CreateHealthInformationForDelete(
    EntityHealthInformationSPtr && entityInformation,
    std::wstring && sourceId,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    wstring property = *HealthInformation::DeleteProperty;
    return HealthReport(
        move(entityInformation),
        move(sourceId),
        move(property),
        *DefaultDeleteTTL,
        FABRIC_HEALTH_STATE_OK,
        L"",
        sequenceNumber,
        false /*removeWhenExpired*/,
        AttributeList());
}

ErrorCode HealthReport::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto commonHealthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    auto error = this->ToCommonPublicApi(heap, *commonHealthInformation);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error getting health information from ToPublicApi: {0}. {1}", error, error.Message);
        return error;
    }

    if (!entityInformation_)
    {
        Trace.WriteInfo(TraceSource, "Error getting entity health information from ToPublicApi: {0}. {1}", error, error.Message);
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    error = entityInformation_->ToPublicApi(heap, commonHealthInformation.GetRawPointer(), healthReport);
    return error;
}

ErrorCode HealthReport::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    FABRIC_STRING_PAIR_LIST const & attributeList)
{
    AttributeList attributeListResult;
    ErrorCode error = attributeListResult.FromPublicApi(attributeList);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing attributes from PublicAPI: {0}. {1}", error, error.Message);
        return error;
    }

    attributeList_ = move(attributeListResult);

    // More attributes can be added here.
    error = FromPublicApi(healthReport);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing health information from PublicAPI: {0}. {1}", error, error.Message);
        return error;
    }

    return error;
}

ErrorCode HealthReport::FromPublicApi(FABRIC_HEALTH_REPORT const & healthReport)
{
    ErrorCode error = ErrorCodeValue::Success;

    entityInformation_ = EntityHealthInformation::CreateSPtr(healthReport.Kind);
    error = entityInformation_->FromPublicApi(healthReport, *this, attributeList_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing health entity information from PublicAPI: {0}. {1}", error, error.Message);
        return error;
    }

    return error;
}

wstring const& HealthReport::get_EntityPropertyId() const
{
    if (entityPropertyId_.empty())
    {
        CheckEntityExists();
        entityPropertyId_ = wformatString("{0}{1}{2}", entityInformation_->EntityId, HealthReport::Delimiter, property_);
    }

    return entityPropertyId_;
}

void HealthReport::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    CheckEntityExists();
    writer.Write("HealthReport({0} instance={1} {2} {3} {4} ttl={5} sn={6} {7} removeWhenExpired={8} {9} priority {10})",
        entityInformation_->EntityId,
        entityInformation_->EntityInstance,
        sourceId_,
        property_,
        state_,
        timeToLive_,
        sequenceNumber_,
        description_,
        removeWhenExpired_,
        sourceUtcTimestamp_,
        priority_);
}

wstring HealthReport::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

std::string HealthReport::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "HealthReport({0} instance={1} {2} {3} {4} ttl={5} sn={6} {7} removeWhenExpired={8} {9} priority {10})";
    size_t index = 0;

    traceEvent.AddEventField<std::wstring>(format, name + ".entityId", index);
    traceEvent.AddEventField<FABRIC_INSTANCE_ID>(format, name + ".entityInstance", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".sourceId", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".property", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".state", index);
    traceEvent.AddEventField<Common::TimeSpan>(format, name + ".ttl", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".sn", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".description", index);
    traceEvent.AddEventField<bool>(format, name + ".removeWhenExpired", index);
    traceEvent.AddEventField<Common::DateTime>(format, name + ".sourceUtc", index);
    traceEvent.AddEventField<wstring>(format, name + ".priority", index);

    return format;
}

void HealthReport::FillEventData(Common::TraceEventContext & context) const
{
    CheckEntityExists();
    context.Write<std::wstring>(entityInformation_->EntityId);
    context.WriteCopy<FABRIC_INSTANCE_ID>(entityInformation_->EntityInstance);
    context.Write<std::wstring>(sourceId_);
    context.Write<std::wstring>(property_);
    context.WriteCopy<std::wstring>(wformatString(state_));
    context.Write<Common::TimeSpan>(timeToLive_);
    context.Write<FABRIC_SEQUENCE_NUMBER>(sequenceNumber_);
    context.Write<std::wstring>(description_);
    context.Write<bool>(removeWhenExpired_);
    context.Write<Common::DateTime>(sourceUtcTimestamp_);
    context.WriteCopy<wstring>(wformatString(this->ReportPriority));
}

void HealthReport::CheckEntityExists() const
{
    ASSERT_IFNOT(entityInformation_, "entityInformation should exist");
    ASSERT_IF(entityInformation_->Kind == FABRIC_HEALTH_REPORT_KIND_INVALID, "entityInformation is not valid: {0}", *entityInformation_);
}

HealthReport HealthReport::GenerateInstanceHealthReport(
    HealthInformation && healthInformation,
    Guid &partitionId,
    FABRIC_REPLICA_ID & instanceId)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(partitionId, instanceId);
    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}

HealthReport HealthReport::GenerateReplicaHealthReport(
    HealthInformation && healthInformation,
    Guid &partitionId,
    FABRIC_REPLICA_ID & replicaId)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(partitionId, replicaId, FABRIC_INVALID_REPLICA_ID);
    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}

HealthReport HealthReport::GeneratePartitionHealthReport(
    HealthInformation && healthInformation,
    Guid &partitionId)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionId);
    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}

HealthReport HealthReport::GenerateApplicationHealthReport(
    HealthInformation && healthInformation,
    wstring const & applicationName)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreateApplicationEntityHealthInformation(applicationName, FABRIC_INVALID_SEQUENCE_NUMBER);
    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}

HealthReport HealthReport::GenerateDeployedApplicationHealthReport(
    HealthInformation && healthInformation,
    wstring const & applicationName,
    LargeInteger const & nodeId,
    wstring const & nodeName)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(applicationName, nodeId, nodeName, FABRIC_INVALID_SEQUENCE_NUMBER);
    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}

HealthReport HealthReport::GenerateDeployedServicePackageHealthReport(
    HealthInformation && healthInformation,
    wstring const & applicationName,
    wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    LargeInteger const & nodeId,
    wstring const & nodeName)
{
    EntityHealthInformationSPtr entityHealthInformation;
    entityHealthInformation = EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(
        applicationName,
        serviceManifestName,
        servicePackageActivationId,
        nodeId,
        nodeName,
        FABRIC_INVALID_SEQUENCE_NUMBER);

    AttributeList attributes;
    return HealthReport(move(entityHealthInformation),
        move(healthInformation),
        move(attributes));
}
