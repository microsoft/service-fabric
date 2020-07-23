// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace SystemHealthReportCode
    {
        enum Enum
        {
            // Source = CM
            CM_ApplicationCreated = 0,
            CM_ApplicationUpdated = 1,
            CM_DeleteApplication = 2,

            // Source = RA
            RA_ReplicaCreated = 3,
            RA_ReplicaOpenStatusWarning = 4,
            RA_ReplicaOpenStatusHealthy = 5,
            RA_DeleteReplica = 6,

            // Source = RAP
            RAP_ApiOk = 7,
            RAP_ApiSlow = 8,

            // Source = FabricNode
            FabricNode_CertificateCloseToExpiration = 9,
            FabricNode_CertificateOk = 10,
            FabricNode_CertificateRevocationListOffline = 104,
            FabricNode_CertificateRevocationListOk = 105,
            FabricNode_SecurityApiSlow = 108,

            // Source = Federation
            Federation_NeighborhoodOk = 11,
            Federation_NeighborhoodLost = 12,
            Federation_LeaseTransportDelay = LEASE_TRANSPORT_DELAY_REPORT_CODE,

            // Source = FM
            FM_PartitionHealthy = 13,
            FM_PartitionQuorumLoss = 14,
            FM_PartitionBuildStuckBelowMinReplicaCount = 15,
            FM_PartitionPlacementStuckBelowMinReplicaCount = 16,
            FM_PartitionPlacementStuckAtZeroReplicaCount = 17,
            FM_PartitionReconfigurationStuck = 18,
            FM_PartitionPlacementStuck = 19,
            FM_PartitionBuildStuck = 20,
            FM_PartitionDeletionInProgress = 21,
            FM_ServiceCreated = 22,
            FM_DeleteService = 23,
            FM_DeleteNode = 24,
            FM_DeletePartition = 25,
            FM_NodeUp = 26,
            FM_NodeDown = 27,
            FM_NodeDownDuringUpgrade = 28,
            FM_NodeDeactivateStuck = 106,
            FM_RebuildStuck = 133,
            FM_RebuildHealthy = 132,
            FM_SeedNodeDown = 141,

            // Source = FMM
            FMM_PartitionHealthy = 29,
            FMM_PartitionQuorumLoss = 30,
            FMM_PartitionBuildStuckBelowMinReplicaCount = 31,
            FMM_PartitionPlacementStuckBelowMinReplicaCount = 32,
            FMM_PartitionReconfigurationStuck = 33,
            FMM_PartitionPlacementStuck = 34,
            FMM_PartitionBuildStuck = 35,
            FMM_PartitionDeletionInProgress = 36,
            FMM_DeletePartition = 37,
            FMM_ServiceCreated = 38,
            FMM_NodeDeactivateStuck = 107,
            FMM_RebuildStuck = 134,
            FMM_RebuildHealthy = 135,

            // Source = PLB
            PLB_NodeCapacityViolation = 39,
            PLB_NodeCapacityOK = 40,

            // Source = Hosting
            Hosting_ActivationFailed = 41,
            Hosting_ApplicationActivated = 42,
            Hosting_ServicePackageActivated = 43,
            Hosting_CodePackageActivationWarning = 44,
            Hosting_CodePackageActivationError = 45,
            Hosting_CodePackageActivated = 46,
            Hosting_ServiceTypeRegistered = 47,
            Hosting_ServiceTypeRegistrationTimeout = 48,
            Hosting_ServiceTypeUnregistered = 49,
            Hosting_ServiceTypeDisabled = 50,
            Hosting_DownloadFailed = 51,
            Hosting_DeleteEvent = 52,
            Hosting_DeleteDeployedApplication = 53,
            Hosting_DeleteDeployedServicePackage = 54,
            Hosting_FabricUpgradeValidationFailed = 55,
            Hosting_FabricUpgradeFailed = 56,

            // Warnings when node capacity are either not defined 
            // or we have mismatch with actual capacities for resource governance metrics
            Hosting_AvailableResourceCapacityMismatch = 125,
            Hosting_AvailableResourceCapacityNotDefined = 126,

            Hosting_DockerHealthCheckStatusHealthy = 130,
            Hosting_DockerHealthCheckStatusUnhealthy = 131,
            Hosting_DockerDaemonUnhealthy = 140,

            // Source - Replicator
            RE_QueueFull = 57,
            RE_QueueOk = 58,
            RE_ApiSlow = 59,
            RE_ApiOk = 60,
            RE_RemoteReplicatorConnectionStatusOk = 136,
            RE_RemoteReplicatorConnectionStatusFailed = 137,

            // Source = PLB
            PLB_UnplacedReplicaViolation = 61,
            PLB_ReplicaConstraintViolation = 62,
            PLB_ReplicaSoftConstraintViolation = 63,
            PLB_UpgradePrimarySwapViolation = 66,
            PLB_BalancingUnsuccessful = 67,
            PLB_ConstraintFixUnsuccessful = 68,
            PLB_ServiceCorrelationError = 69,
            PLB_ServiceDescriptionOK = 70,

            // Source = FM
            FM_PartitionRebuildStuck = 64,

            // Source = FMM
            FMM_PartitionRebuildStuck = 65,

            // Source = Testability
            Testability_UnreliableTransportBehaviorTimeOut = 71,

            // Source = ReplicatedStore
            REStore_Unexpected = 80,
            REStore_SlowCommit = 81,
            REStore_PathTooLong = 82,
            REStore_AutoCompaction = 83,

            // Source = PLB
            PLB_MovementsDropped = 90,

            // Source = NamingStore
            Naming_OperationSlow = 100,
            Naming_OperationSlowCompleted = 101,

            // Source = RA
            RA_ReplicaChangeRoleStatusError = 102,
            RA_ReplicaChangeRoleStatusHealthy = 103,
            RA_ReplicaCloseStatusWarning = 121,
            RA_ReplicaCloseStatusHealthy = 122,
            RA_ReplicaServiceTypeRegistrationStatusWarning = 123,
            RA_ReplicaServiceTypeRegistrationStatusHealthy = 124,
            RA_ReconfigurationStuckWarning = 127,
            RA_ReconfigurationHealthy = 128,
            RA_StoreProviderHealthy = 138,
            RA_StoreProviderUnhealthy = 139,

            // Source = CRM
            CRM_NodeCapacityViolation = 109,
            CRM_NodeCapacityOK = 110,
            CRM_UnplacedReplicaViolation = 111,
            CRM_ReplicaConstraintViolation = 112,
            CRM_ReplicaSoftConstraintViolation = 113,
            CRM_UpgradePrimarySwapViolation = 114,
            CRM_BalancingUnsuccessful = 115,
            CRM_ConstraintFixUnsuccessful = 116,
            CRM_ServiceCorrelationError = 117,
            CRM_ServiceDescriptionOK = 118,
            CRM_MovementsDropped = 119,

            // Source = Native TransactionalReplicator
            TR_SlowIO = 129,

            LAST_STATE = FM_SeedNodeDown
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

        DECLARE_ENUM_STRUCTURED_TRACE( SystemHealthReportCode )
    }
}
