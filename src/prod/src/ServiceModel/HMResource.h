// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
// Defines a resource string based on "IDS_HM_ResourceName/ResourceProperty"
#define HM_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( ManagementResource, ResourceProperty, HM, ResourceName ) \

#define CM_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( ManagementResource, ResourceProperty, CM, ResourceName ) \

#define RA_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( RAResource, ResourceProperty, RA, ResourceName ) \

#define FABRIC_NODE_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FabricNodeResources, ResourceProperty, FABRIC_NODE, ResourceName ) \

#define FEDERATION_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FederationResources, ResourceProperty, FEDERATION, ResourceName ) \

#define HOSTING_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( HostingResources, ResourceProperty, HOSTING, ResourceName ) \

#define FAILOVER_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FailoverResources, ResourceProperty, FAILOVER, ResourceName ) \

#define RAP_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( RAPResource, ResourceProperty, RAP, ResourceName ) \

#define PLB_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( PLBResources, ResourceProperty, PLB, ResourceName) \

#define NAMING_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( NamingResources, ResourceProperty, NAMING, ResourceName) \

#define FM_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FMResource, ResourceProperty, FM, ResourceName ) \

#define FMM_HEALTH_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE( FMMResource, ResourceProperty, FMM, ResourceName ) \

    class HMResource
    {
        DECLARE_SINGLETON_RESOURCE( HMResource )

        HM_RESOURCE( HealthEvaluationUnhealthyEvent, Health_Evaluation_Unhealthy_Event )
        HM_RESOURCE( HealthEvaluationExpiredEvent, Health_Evaluation_Expired_Event )
        HM_RESOURCE( HealthEvaluationUnhealthyEventPerPolicy, Health_Evaluation_Unhealthy_EventPerPolicy )
        HM_RESOURCE( HealthEvaluationUnhealthyUpgradeDomainDeltaNodesCheck, Health_Evaluation_Unhealthy_UpgradeDomainDeltaNodesCheck )
        HM_RESOURCE( HealthEvaluationUnhealthyDeltaNodesCheck, Health_Evaluation_Unhealthy_DeltaNodesCheck )
        HM_RESOURCE( HealthEvaluationUnhealthyReplicasPerPolicy, Health_Evaluation_Unhealthy_Replicas )
        HM_RESOURCE( HealthEvaluationUnhealthyPartitionsPerPolicy, Health_Evaluation_Unhealthy_Partitions )
        HM_RESOURCE( HealthEvaluationUnhealthyDeployedServicePackages, Health_Evaluation_Unhealthy_Deployed_Service_Packages )
        HM_RESOURCE( HealthEvaluationUnhealthyDeployedApplicationsPerPolicy, Health_Evaluation_Unhealthy_DeployedApplications )
        HM_RESOURCE( HealthEvaluationUnhealthyServicesPerServiceTypePolicy, Health_Evaluation_Unhealthy_ServiceType_Services )
        HM_RESOURCE( HealthEvaluationUnhealthyNodesPerPolicy, Health_Evaluation_Unhealthy_Nodes )
        HM_RESOURCE( HealthEvaluationUnhealthyApplicationsPerPolicy, Health_Evaluation_Unhealthy_Applications )
        HM_RESOURCE( HealthEvaluationUnhealthyDeployedApplicationsPerUDPolicy, Health_Evaluation_Unhealthy_UD_Deployed_Applications )
        HM_RESOURCE( HealthEvaluationUnhealthyNodesPerUDPolicy, Health_Evaluation_Unhealthy_UD_Nodes )
        HM_RESOURCE( HealthEvaluationUnhealthySystemApplication, Health_Evaluation_Unhealthy_System_Application )
        HM_RESOURCE( HealthReportInvalidEmptyInput, Health_Report_Invalid_Empty_Input )
        HM_RESOURCE( HealthReportInvalidSourceId, Health_Report_Invalid_SourceId )
        HM_RESOURCE( HealthReportInvalidProperty, Health_Report_Invalid_Property )
        HM_RESOURCE( HealthReportInvalidDescription, Health_Report_Invalid_Description )
        HM_RESOURCE( HealthReportInvalidHealthState, Health_Report_Invalid_Health_State )
        HM_RESOURCE( HealthReportReservedSourceId, Health_Report_Reserved_SourceId )
        HM_RESOURCE( HealthReportReservedProperty, Health_Report_Reserved_Property)
        HM_RESOURCE( HealthReportInvalidSequenceNumber, Health_Report_Invalid_Sequence_Number )
        HM_RESOURCE( HealthEvaluationUnhealthyNode, Health_Evaluation_Unhealthy_Node )
        HM_RESOURCE( HealthEvaluationUnhealthyService, Health_Evaluation_Unhealthy_Service )
        HM_RESOURCE( HealthEvaluationUnhealthyReplica, Health_Evaluation_Unhealthy_Replica )
        HM_RESOURCE( HealthEvaluationUnhealthyPartition, Health_Evaluation_Unhealthy_Partition )
        HM_RESOURCE( HealthEvaluationUnhealthyApplication, Health_Evaluation_Unhealthy_Application )
        HM_RESOURCE( HealthEvaluationUnhealthyDeployedApplication, Health_Evaluation_Unhealthy_DeployedApplication )
        HM_RESOURCE( HealthEvaluationUnhealthyDeployedServicePackage, Health_Evaluation_Unhealthy_DeployedServicePackage )
        HM_RESOURCE( HealthEvaluationUnhealthyApplicationTypeApplications, Health_Evaluation_Unhealthy_ApplicationTypeApplications )
        HM_RESOURCE( HealthStaleReportDueToSequenceNumber, Health_Stale_Report_SequenceNumber)
        HM_RESOURCE( HealthStaleReportDueToInstance, Health_Stale_Report_Instance)
        HM_RESOURCE( HealthStaleReportDueToEntityDeleted, Health_Stale_Report_Entity_Deleted)
        HM_RESOURCE( EntityMissingRequiredReport, MissingRequiredReport)
        HM_RESOURCE( InvalidApplicationTypeHealthPolicyMapItem, Invalid_ApplicationTypeHealthPolicyMapItem)
        HM_RESOURCE( DuplicateNodeFilters, DuplicateNodeFilters)
        HM_RESOURCE( DuplicateApplicationFilters, DuplicateApplicationFilters)
        HM_RESOURCE( DuplicateServiceFilters, DuplicateServiceFilters)
        HM_RESOURCE( DuplicatePartitionFilters, DuplicatePartitionFilters)
        HM_RESOURCE( DuplicateReplicaFilters, DuplicateReplicaFilters)
        HM_RESOURCE( DuplicateDeployedApplicationFilters, DuplicateDeployedApplicationFilters)
        HM_RESOURCE( DuplicateDeployedServicePackageFilters, DuplicateDeployedServicePackageFilters)
        HM_RESOURCE( EntityTooLargeNoUnhealthyEvaluations, EntityTooLarge_NoUnhealthyEvaluations )
        HM_RESOURCE( EntityTooLargeWithUnhealthyEvaluations ,EntityTooLarge_WithUnhealthyEvaluations )
        HM_RESOURCE( TooManyHealthReports, TooManyHealthReports )

        // Resource strings used in HealthReport description by system components

        // CM
        CM_HEALTH_RESOURCE( ApplicationCreated, Application_Created )
        CM_HEALTH_RESOURCE( ApplicationUpdated, Application_Updated )

        // RA
        RA_HEALTH_RESOURCE ( HealthReplicaCreated, HEALTH_REPLICA_CREATED )
        RA_HEALTH_RESOURCE ( HealthReplicaOpenStatusWarning, HEALTH_REPLICA_OPEN_STATUS_WARNING)
        RA_HEALTH_RESOURCE ( HealthReplicaOpenStatusHealthy, HEALTH_REPLICA_OPEN_STATUS_HEALTHY)
        RA_HEALTH_RESOURCE ( HealthReplicaChangeRoleStatusError, HEALTH_REPLICA_CHANGEROLE_STATUS_ERROR)
        RA_HEALTH_RESOURCE ( HealthReplicaChangeStatusHealthy, HEALTH_REPLICA_CHANGEROLE_STATUS_HEALTHY)
        RA_HEALTH_RESOURCE ( HealthReplicaCloseStatusWarning, HEALTH_REPLICA_CLOSE_STATUS_WARNING)
        RA_HEALTH_RESOURCE ( HealthReplicaCloseStatusHealthy, HEALTH_REPLICA_CLOSE_STATUS_HEALTHY)
        RA_HEALTH_RESOURCE ( HealthReconfigurationStatusWarning, HEALTH_RECONFIGURATION_STATUS_WARNING)
        RA_HEALTH_RESOURCE ( HealthReconfigurationStatusHealthy, HEALTH_RECONFIGURATION_STATUS_HEALTHY)
        RA_HEALTH_RESOURCE ( HealthReplicaServiceTypeRegistrationStatusWarning, HEALTH_REPLICA_STR_STATUS_WARNING)
        RA_HEALTH_RESOURCE ( HealthReplicaServiceTypeRegistrationStatusHealthy, HEALTH_REPLICA_STR_STATUS_HEALTHY)

        // RAP
        RAP_HEALTH_RESOURCE( HealthStartTime, HEALTH_START_TIME )

        // FabricNode
        FABRIC_NODE_HEALTH_RESOURCE(CertificateExpiration, Certificate_Expiration)
        FABRIC_NODE_HEALTH_RESOURCE(CertificateRevocationList, Certificate_Revocation_List)
        FABRIC_NODE_HEALTH_RESOURCE(SecurityApi, SecurityApi)

        // Federation
        FEDERATION_HEALTH_RESOURCE( NeighborHoodLost, Neighborhood_Lost )

        // Hosting
        HOSTING_HEALTH_RESOURCE( DownloadFailed, Download_Failed )
        HOSTING_HEALTH_RESOURCE( ActivationFailed, Activation_Failed )
        HOSTING_HEALTH_RESOURCE( ApplicationActivated, Application_Activated )
        HOSTING_HEALTH_RESOURCE( ServicePackageActivated, ServicePackage_Activated )
        HOSTING_HEALTH_RESOURCE( CodePackageActivationFailed, CodePackage_Activation_Failed )
        HOSTING_HEALTH_RESOURCE( CodePackageActivated, CodePackage_Activated )
        HOSTING_HEALTH_RESOURCE( ServiceTypeRegistered, ServiceType_Registered )
        HOSTING_HEALTH_RESOURCE( ServiceTypeRegistrationTimeout, ServiceType_Registration_Timeout )
        HOSTING_HEALTH_RESOURCE( ServiceTypeUnregistered, ServiceType_Unregistered )
        HOSTING_HEALTH_RESOURCE( ServiceTypeDisabled, ServiceType_Disabled )
        HOSTING_HEALTH_RESOURCE( FabricUpgradeValidationFailed, FabricUpgrade_Validation_Failed )
        HOSTING_HEALTH_RESOURCE( FabricUpgradeFailed, FabricUpgrade_Failed )
        HOSTING_HEALTH_RESOURCE( AvailableResourceCapacityMismatch, AvailableResourceCapacity_Mismatch )
        HOSTING_HEALTH_RESOURCE( AvailableResourceCapacityNotDefined, AvailableResourceCapacity_NotDefined )

        // Failover
        FAILOVER_HEALTH_RESOURCE( NodeUp, Node_Up )
        FAILOVER_HEALTH_RESOURCE( NodeDown, Node_Down )
        FAILOVER_HEALTH_RESOURCE( SeedNodeDown, Seed_Node_Down )
        FAILOVER_HEALTH_RESOURCE( NodeDownDuringUpgrade, Node_Down_During_Upgrade )
        FAILOVER_HEALTH_RESOURCE( PartitionHealthy, Partition_Healthy )
        FAILOVER_HEALTH_RESOURCE( PartitionPlacementStuck, Partition_PlacementStuck )
        FAILOVER_HEALTH_RESOURCE( PartitionBuildStuck, Partition_BuildStuck )
        FAILOVER_HEALTH_RESOURCE( PartitionRebuildStuck, Partition_RebuildStuck)
        FAILOVER_HEALTH_RESOURCE( PartitionReconfigurationStuck, Partition_ReconfigurationStuck )
        FAILOVER_HEALTH_RESOURCE( PartitionQuorumLoss, Partition_QuorumLoss )
        FAILOVER_HEALTH_RESOURCE( ServiceCreated, Service_Created )
        FAILOVER_HEALTH_RESOURCE( DeletionInProgress, Deletion_InProgress )
        FAILOVER_HEALTH_RESOURCE( NodeDeactivateStuck, Node_DeactivateStuck )

        //FM
        FM_HEALTH_RESOURCE( FMRebuildStuck, Rebuild_Stuck )
        FM_HEALTH_RESOURCE( FMRebuildHealthy, Rebuild_Healthy )

        //FMM
        FMM_HEALTH_RESOURCE( FMMRebuildStuck, Rebuild_Stuck )
        FMM_HEALTH_RESOURCE( FMMRebuildHealthy, Rebuild_Healthy )

        // PLB
        PLB_HEALTH_RESOURCE( PLBNodeCapacityViolationWarning, Node_Capacity_Violation)
        PLB_HEALTH_RESOURCE( PLBNodeCapacityOK, Node_Capacity_OK)
        PLB_HEALTH_RESOURCE( PLBUnplacedReplicaViolation, Unplaced_Replica_Violation)
        PLB_HEALTH_RESOURCE( PLBReplicaConstraintViolation, Replica_Constraint_Violation)
        PLB_HEALTH_RESOURCE( PLBReplicaSoftConstraintViolation, Replica_Soft_Constraint_Violation)
        PLB_HEALTH_RESOURCE( PLBUpgradePrimarySwapViolation, Upgrade_Primary_Swap_Violation)
        PLB_HEALTH_RESOURCE( PLBBalancingUnsuccessful, Balancing_Unsuccessful)
        PLB_HEALTH_RESOURCE( PLBConstraintFixUnsuccessful, ConstraintFix_Unsuccessful)
        PLB_HEALTH_RESOURCE( PLBServiceCorrelationError, Correlation_Error)
        PLB_HEALTH_RESOURCE( PLBServiceDescriptionOK, Service_Description_OK)
        PLB_HEALTH_RESOURCE( PLBMovementsDropped, Movements_Dropped)
        PLB_HEALTH_RESOURCE( PLBMovementsDroppedByFMDescription, Movements_Dropped_By_FM_Description)

        // NamingService
        NAMING_HEALTH_RESOURCE( NamingOperationSlow, Operation_Slow )
        NAMING_HEALTH_RESOURCE( NamingOperationSlowCompleted, Operation_Slow_Completed)
    };
}
