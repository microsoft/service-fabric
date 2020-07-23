// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace SystemHealthReportCode
    {
        void WriteToTextWriter(__in TextWriter & w, Enum e)
        {
            switch (e)
            {
            case CM_ApplicationCreated : w << "CM_ApplicationCreated"; return;
            case CM_ApplicationUpdated : w << "CM_ApplicationUpdated"; return;
            case CM_DeleteApplication: w << "CM_DeleteApplication"; return;
            case RA_ReplicaCreated: w << "RA_ReplicaCreated"; return;
            case RA_ReplicaOpenStatusWarning: w << "RA_ReplicaOpenStatusWarning"; return;
            case RA_ReplicaOpenStatusHealthy: w << "RA_ReplicaOpenStatusHealthy"; return;
            case RA_ReplicaChangeRoleStatusError: w << "RA_ReplicaChangeRoleStatusError"; return;
            case RA_ReplicaChangeRoleStatusHealthy: w << "RA_ReplicaChangeRoleStatusHealthy"; return;
            case RA_ReplicaCloseStatusWarning: w << "RA_ReplicaCloseStatusWarning"; return;
            case RA_ReplicaCloseStatusHealthy: w << "RA_ReplicaCloseStatusHealthy"; return;
            case RA_ReplicaServiceTypeRegistrationStatusWarning: w << "RA_ReplicaServiceTypeRegistrationStatusWarning"; return;
            case RA_ReplicaServiceTypeRegistrationStatusHealthy: w << "RA_ReplicaServiceTypeRegistrationStatusHealthy"; return;
            case RA_ReconfigurationStuckWarning: w << "RA_ReconfigurationStuckWarning"; return;
            case RA_ReconfigurationHealthy: w << "RA_ReconfigurationHealthy"; return;
            case RA_StoreProviderHealthy: w << "RA_StoreProviderHealthy"; return;
            case RA_DeleteReplica: w << "RA_DeleteReplica"; return;
            case RAP_ApiOk: w << "RAP_ApiOk"; return;
            case RAP_ApiSlow: w << "RAP_ApiSlow"; return;
            case FabricNode_CertificateCloseToExpiration: w << "FabricNode_CertificateCloseToExpiration"; return;
            case FabricNode_CertificateOk: w << "FabricNode_CertificateOk"; return;
            case FabricNode_CertificateRevocationListOffline: w << "FabricNode_CertificateRevocationListOffline"; return;
            case FabricNode_CertificateRevocationListOk: w << "FabricNode_CertificateRevocationListOk"; return;
            case Federation_NeighborhoodOk: w << "Federation_NeighborhoodOk"; return;
            case Federation_NeighborhoodLost: w << "Federation_NeighborhoodLost"; return;
            case FM_PartitionHealthy: w << "FM_PartitionHealthy"; return;
            case FM_PartitionQuorumLoss: w << "FM_PartitionQuorumLoss"; return;
            case FM_PartitionBuildStuckBelowMinReplicaCount: w << "FM_PartitionBuildStuckBelowMinReplicaCount"; return;
            case FM_PartitionPlacementStuckBelowMinReplicaCount: w << "FM_PartitionPlacementStuckBelowMinReplicaCount"; return;
            case FM_PartitionPlacementStuckAtZeroReplicaCount: w << "FM_PartitionPlacementStuckAtZeroReplicaCount"; return;
            case FM_PartitionReconfigurationStuck: w << "FM_PartitionReconfigurationStuck"; return;
            case FM_PartitionPlacementStuck: w << "FM_PartitionPlacementStuck"; return;
            case FM_PartitionBuildStuck: w << "FM_PartitionBuildStuck"; return;
            case FM_PartitionRebuildStuck: w << "FM_PartitionRebuildStuck"; return;
            case FM_PartitionDeletionInProgress: w << "FM_PartitionDeletionInProgress"; return;
            case FM_ServiceCreated: w << "FM_ServiceCreated"; return;
            case FM_DeleteService: w << "FM_DeleteService"; return;
            case FM_DeleteNode: w << "FM_DeleteNode"; return;
            case FM_DeletePartition: w << "FM_DeletePartition"; return;
            case FM_NodeUp: w << "FM_NodeUp"; return;
            case FM_NodeDown: w << "FM_NodeDown"; return;
            case FM_NodeDownDuringUpgrade: w << "FMM_NodeDownDuringUpgrade"; return;
            case FM_NodeDeactivateStuck: w << "FM_NodeDeactivateStuck"; return;
            case FM_RebuildStuck: w << "FM_RebuildStuck"; return;
            case FM_RebuildHealthy: w << "FM_RebuildHealthy"; return;
            case FM_SeedNodeDown: w << "FM_SeedNodeDown"; return;
            case FMM_PartitionHealthy: w << "FMM_PartitionHealthy"; return;
            case FMM_PartitionQuorumLoss: w << "FMM_PartitionQuorumLoss"; return;
            case FMM_PartitionBuildStuckBelowMinReplicaCount: w << "FMM_PartitionBuildStuckBelowMinReplicaCount"; return;
            case FMM_PartitionPlacementStuckBelowMinReplicaCount: w << "FMM_PartitionPlacementStuckBelowMinReplicaCount"; return;
            case FMM_PartitionReconfigurationStuck: w << "FMM_PartitionReconfigurationStuck"; return;
            case FMM_PartitionPlacementStuck: w << "FMM_PartitionPlacementStuck"; return;
            case FMM_PartitionBuildStuck: w << "FMM_PartitionBuildStuck"; return;
            case FMM_PartitionRebuildStuck: w << "FMM_PartitionRebuildStuck"; return;
            case FMM_PartitionDeletionInProgress: w << "FMM_PartitionDeletionInProgress"; return;
            case FMM_ServiceCreated: w << "FMM_ServiceCreated"; return;
            case FMM_NodeDeactivateStuck: w << "FMM_NodeDeactivateStuck"; return;
            case FMM_RebuildStuck: w << "FMM_RebuildStuck"; return;
            case FMM_RebuildHealthy: w << "FMM_RebuildHealthy"; return;
            case Hosting_ActivationFailed: w << "Hosting_ActivationFailed"; return;
            case Hosting_ApplicationActivated: w << "Hosting_ApplicationActivated"; return;
            case Hosting_ServicePackageActivated: w << "Hosting_ServicePackageActivated"; return;
            case Hosting_CodePackageActivationWarning: w << "Hosting_CodePackageActivationWarning"; return;
            case Hosting_CodePackageActivationError: w << "Hosting_CodePackageActivationError"; return;
            case Hosting_CodePackageActivated: w << "Hosting_CodePackageActivated"; return;
            case Hosting_ServiceTypeRegistered: w << "Hosting_ServiceTypeRegistered"; return;
            case Hosting_ServiceTypeRegistrationTimeout: w << "Hosting_ServiceTypeRegistrationTimeout"; return;
            case Hosting_ServiceTypeUnregistered: w << "Hosting_ServiceTypeUnregistered"; return;
            case Hosting_ServiceTypeDisabled: w << "Hosting_ServiceTypeDisabled"; return;
            case Hosting_DownloadFailed: w << "Hosting_DownloadFailed"; return;
            case Hosting_DeleteEvent: w << "Hosting_DeleteEvent"; return;
            case Hosting_DeleteDeployedApplication: w << "Hosting_DeleteDeployedApplication"; return;
            case Hosting_DeleteDeployedServicePackage: w << "Hosting_DeleteDeployedServicePackage"; return;
            case Hosting_FabricUpgradeValidationFailed: w << "Hosting_FabricUpgradeValidationFailed"; return;
            case Hosting_AvailableResourceCapacityMismatch: w << "Hosting_AvailableResourceCapacityMismatch"; return;
            case Hosting_AvailableResourceCapacityNotDefined: w << "Hosting_AvailableResourceCapacityNotDefined"; return;
            case Hosting_FabricUpgradeFailed: w << "Hosting_FabricUpgradeFailed"; return;
            case Hosting_DockerHealthCheckStatusHealthy: w << "Hosting_DockerHealthCheckStatusHealthy"; return;
            case Hosting_DockerHealthCheckStatusUnhealthy: w << "Hosting_DockerHealthCheckStatusUnhealthy"; return;
            case Hosting_DockerDaemonUnhealthy: w << "Hosting_DockerDaemonUnhealthy"; return;
            case PLB_NodeCapacityViolation: w << "PLB_NodeCapacityViolation"; return;
            case PLB_NodeCapacityOK: w << "PLB_NodeCapacityOK"; return;
            case PLB_UnplacedReplicaViolation: w << "PLB_UnplacedReplicaViolation"; return;
            case PLB_ReplicaConstraintViolation: w << "PLB_ReplicaConstraintViolation"; return;
            case PLB_ReplicaSoftConstraintViolation: w << "PLB_ReplicaSoftConstraintViolation"; return;
            case RE_QueueFull: w << "RE_QueueFull"; return;
            case RE_QueueOk: w << "RE_QueueOk"; return;
            case RE_ApiSlow: w << "RE_ApiSlow"; return;
            case RE_ApiOk: w << "RE_ApiOk"; return;
            case RE_RemoteReplicatorConnectionStatusOk: w << "RE_RemoteReplicatorConnectionStatusOk"; return;
            case RE_RemoteReplicatorConnectionStatusFailed: w << "RE_RemoteReplicatorConnectionStatusFailed"; return;
            case PLB_UpgradePrimarySwapViolation: w << "PLB_UpgradePrimarySwapViolation"; return;
            case PLB_BalancingUnsuccessful: w << "PLB_BalancingUnsuccessful"; return;
            case PLB_ConstraintFixUnsuccessful: w << "PLB_ConstraintFixUnsuccessful"; return;
            case PLB_ServiceCorrelationError: w << "PLB_ServiceCorrelationError"; return;
            case PLB_ServiceDescriptionOK: w << "PLB_ServiceDescriptionOK"; return;
            case Testability_UnreliableTransportBehaviorTimeOut: w << "Testability_UnreliableTransportBehaviorTimeOut"; return;
            case REStore_Unexpected: w << "REStore_Unexpected"; return;
            case REStore_SlowCommit: w << "REStore_SlowCommit"; return;
            case REStore_PathTooLong: w << "REStore_PathTooLong"; return;
            case REStore_AutoCompaction: w << "REStore_AutoCompaction"; return;
            case PLB_MovementsDropped: w << "PLB_MovementsDropped"; return;
            case Naming_OperationSlow: w << "Naming_OperationSlow"; return;
            case Naming_OperationSlowCompleted: w << "Naming_OperationSlowCompleted"; return;
            case CRM_NodeCapacityViolation: w << "CRM_NodeCapacityViolation"; return;
            case CRM_NodeCapacityOK: w << "CRM_NodeCapacityOK"; return;
            case CRM_UnplacedReplicaViolation: w << "CRM_UnplacedReplicaViolation"; return;
            case CRM_ReplicaConstraintViolation: w << "CRM_ReplicaConstraintViolation"; return;
            case CRM_ReplicaSoftConstraintViolation: w << "CRM_ReplicaSoftConstraintViolation"; return;
            case CRM_UpgradePrimarySwapViolation: w << "CRM_UpgradePrimarySwapViolation"; return;
            case CRM_BalancingUnsuccessful: w << "CRM_BalancingUnsuccessful"; return;
            case CRM_ConstraintFixUnsuccessful: w << "CRM_ConstraintFixUnsuccessful"; return;
            case CRM_ServiceCorrelationError: w << "CRM_ServiceCorrelationError"; return;
            case CRM_ServiceDescriptionOK: w << "CRM_ServiceDescriptionOK"; return;
            case CRM_MovementsDropped: w << "CRM_MovementsDropped"; return;
            case TR_SlowIO: w << "TR_SlowIO"; return;
            default: w << "SystemHealthReportCode(" << static_cast<uint>(e) << ')'; return;
            }
        }

        BEGIN_ENUM_STRUCTURED_TRACE( SystemHealthReportCode )
            ADD_CASTED_ENUM_MAP_VALUE_RANGE( SystemHealthReportCode, CM_ApplicationCreated, LAST_STATE )
        END_ENUM_STRUCTURED_TRACE( SystemHealthReportCode )

    } // end SystemHealthReportCode namespace
}
