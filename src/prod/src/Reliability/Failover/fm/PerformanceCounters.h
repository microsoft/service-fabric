// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/PerformanceCounter.h"

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverUnitCounters
        {
            DENY_COPY(FailoverUnitCounters)

        public:

            BEGIN_COUNTER_SET_DEFINITION(
                L"EF4BAE59-375F-4287-A113-0A59AC8FB879",
                L"Failover Manager Component",
                L"Counters for Failover Manager Component",
                Common::PerformanceCounterSetInstanceType::Multiple)

                COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"#Unhealthy Failover Units", L"Number of failover units are currently unhealthy")
                COUNTER_DEFINITION(2, Common::PerformanceCounterType::RawData64, L"#Unprocessed Failover Units", L"Number of failover units that were not processed during background processing")
                COUNTER_DEFINITION(3, Common::PerformanceCounterType::RawData64, L"#Failover Unit Actions", L"Number of failover unit actions generated during background processing")
                COUNTER_DEFINITION(4, Common::PerformanceCounterType::AverageBase, L"Base for Failover Unit Commit Duration", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(5, 4, Common::PerformanceCounterType::AverageCount64, L"Failover Unit Commit Duration", L"Time taken for the Failover Unit to be persisted and replicated")
                COUNTER_DEFINITION(6, Common::PerformanceCounterType::AverageBase, L"Base for Failover Unit Reconfiguration Duration", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(7, 6, Common::PerformanceCounterType::AverageCount64, L"Failover Unit Reconfiguration Duration", L"Time taken for the Failover Unit to complete a reconfiguration")
                COUNTER_DEFINITION(8, Common::PerformanceCounterType::AverageBase, L"Base for Failover Unit Placement Duration", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(9, 8, Common::PerformanceCounterType::AverageCount64, L"Failover Unit Placement Duration", L"Time taken for the Failover Unit to start placing required number of replicas")
                COUNTER_DEFINITION(10, Common::PerformanceCounterType::RawData64, L"#InBuild Replicas", L"Number of InBuild replicas in the system")
                COUNTER_DEFINITION(11, Common::PerformanceCounterType::RawData64, L"#StandBy Replicas", L"Number of StandBy replicas in the system")
                COUNTER_DEFINITION(12, Common::PerformanceCounterType::RawData64, L"#Offline Replicas", L"Number of Offline replicas in the system")
                COUNTER_DEFINITION(13, Common::PerformanceCounterType::RawData64, L"#Replicas", L"Number of replicas in the system")
                COUNTER_DEFINITION(14, Common::PerformanceCounterType::RawData64, L"#Failover Units", L"Number of FailoverUnits in the system")
                COUNTER_DEFINITION(15, Common::PerformanceCounterType::RawData64, L"#InBuild Failover Units", L"Number of InBuildFailoverUnits in the system")
                COUNTER_DEFINITION(16, Common::PerformanceCounterType::RawData64, L"#Services", L"Number of services in the system")
                COUNTER_DEFINITION(17, Common::PerformanceCounterType::RawData64, L"#Service Types", L"Number of service types in the system")
                COUNTER_DEFINITION(18, Common::PerformanceCounterType::RawData64, L"#Applications", L"Number of applications in the system")
                COUNTER_DEFINITION(19, Common::PerformanceCounterType::RawData64, L"#Application Upgrades", L"Number of application upgrades in the system")

                COUNTER_DEFINITION(20, Common::PerformanceCounterType::RawData64, L"Common Queue Length", L"Number of items in the common queue")
                COUNTER_DEFINITION(21, Common::PerformanceCounterType::RawData64, L"Query Queue Length", L"Number of items in the query queue")
                COUNTER_DEFINITION(22, Common::PerformanceCounterType::RawData64, L"Failover Unit Queue Length", L"Number of items in the failover unit queue")
                COUNTER_DEFINITION(23, Common::PerformanceCounterType::RawData64, L"Commit Queue Length", L"Number of items in the commit queue")

                COUNTER_DEFINITION(24, Common::PerformanceCounterType::AverageBase, L"Base for Message Process Duration", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(25, 24, Common::PerformanceCounterType::AverageCount64, L"Message Process Duration", L"Time taken for the Failover Manager to process a message")

                COUNTER_DEFINITION(26, Common::PerformanceCounterType::AverageBase, L"Base for PLB SetMovementEnabled", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(27, 26, Common::PerformanceCounterType::AverageCount64, L"PLB SetMovementEnabled", L"Time taken for the PLB SetMovementEnabled function call")

                COUNTER_DEFINITION(28, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateNode", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(29, 28, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateNode", L"Time taken for the PLB UpdateNode function call")

                COUNTER_DEFINITION(30, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateServiceType", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(31, 30, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateServiceType", L"Time taken for the PLB UpdateServiceType function call")

                COUNTER_DEFINITION(32, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateApplication", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(33, 32, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateApplication", L"Time taken for the PLB UpdateApplication function call")

                COUNTER_DEFINITION(34, Common::PerformanceCounterType::AverageBase, L"Base for PLB DeleteApplication", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(35, 34, Common::PerformanceCounterType::AverageCount64, L"PLB DeleteApplication", L"Time taken for the PLB DeleteServiceType function call")

                COUNTER_DEFINITION(36, Common::PerformanceCounterType::AverageBase, L"Base for PLB DeleteServiceType", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(37, 36, Common::PerformanceCounterType::AverageCount64, L"PLB DeleteServiceType", L"Time taken for the PLB DeleteServiceType function call")

                COUNTER_DEFINITION(38, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateService", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(39, 38, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateService", L"Time taken for the PLB UpdateService function call")

                COUNTER_DEFINITION(40, Common::PerformanceCounterType::AverageBase, L"Base for PLB DeleteService", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(41, 40, Common::PerformanceCounterType::AverageCount64, L"PLB DeleteService", L"Time taken for the PLB DeleteService function call")

                COUNTER_DEFINITION(42, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateFailoverUnit", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(43, 42, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateFailoverUnit", L"Time taken for the PLB UpdateFailoverUnit function call")

                COUNTER_DEFINITION(44, Common::PerformanceCounterType::AverageBase, L"Base for PLB DeleteFailoverUnit", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(45, 44, Common::PerformanceCounterType::AverageCount64, L"PLB DeleteFailoverUnit", L"Time taken for the PLB DeleteFailoverUnit function call")

                COUNTER_DEFINITION(46, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateLoadOrMoveCost", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(47, 46, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateLoadOrMoveCost", L"Time taken for the PLB UpdateLoadOrMoveCost function call")

                COUNTER_DEFINITION(48, Common::PerformanceCounterType::AverageBase, L"Base for PLB ResetPartitionLoad", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(49, 48, Common::PerformanceCounterType::AverageCount64, L"PLB ResetPartitionLoad", L"Time taken for the PLB ResetPartitionLoad function call")

                COUNTER_DEFINITION(50, Common::PerformanceCounterType::AverageBase, L"Base for PLB CompareNodeForPromotion", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(51, 50, Common::PerformanceCounterType::AverageCount64, L"PLB CompareNodeForPromotion", L"Time taken for the PLB CompareNodeForPromotion function call")

                COUNTER_DEFINITION(52, Common::PerformanceCounterType::AverageBase, L"Base for PLB Dispose", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(53, 52, Common::PerformanceCounterType::AverageCount64, L"PLB Dispose", L"Time taken for the PLB Dispose function call")

                COUNTER_DEFINITION(54, Common::PerformanceCounterType::AverageBase, L"Base for PLB GetClusterLoadInformationQueryResult", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(55, 54, Common::PerformanceCounterType::AverageCount64, L"PLB GetClusterLoadInformationQueryResult", L"Time taken for the PLB GetClusterLoadInformationQueryResult function call")

                COUNTER_DEFINITION(56, Common::PerformanceCounterType::AverageBase, L"Base for PLB GetNodeLoadInformationQueryResult", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(57, 56, Common::PerformanceCounterType::AverageCount64, L"PLB GetNodeLoadInformationQueryResult", L"Time taken for the PLB GetNodeLoadInformationQueryResult function call")

                COUNTER_DEFINITION(58, Common::PerformanceCounterType::AverageBase, L"Base for PLB GetGetUnplacedReplicaInformationQueryResult", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(59, 58, Common::PerformanceCounterType::AverageCount64, L"PLB GetGetUnplacedReplicaInformationQueryResult", L"Time taken for the PLB GetGetUnplacedReplicaInformationQueryResult function call")

                COUNTER_DEFINITION(60, Common::PerformanceCounterType::AverageBase, L"Base for PLB TriggerPromoteToPrimary", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(61, 60, Common::PerformanceCounterType::AverageCount64, L"PLB TriggerPromoteToPrimary", L"Time taken for the PLB TriggerPromoteToPrimary function call")

                COUNTER_DEFINITION(62, Common::PerformanceCounterType::AverageBase, L"Base for PLB GetApplicationLoadInformationResult", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(63, 62, Common::PerformanceCounterType::AverageCount64, L"PLB GetApplicationLoadInformationResult", L"Time taken for the PLB GetApplicationLoadInformationResult function call")

                COUNTER_DEFINITION(64, Common::PerformanceCounterType::AverageBase, L"Base for PLB TriggerSwapPrimary", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(65, 64, Common::PerformanceCounterType::AverageCount64, L"PLB TriggerSwapPrimary", L"Time taken for the PLB TriggerSwapPrimary function call")

                COUNTER_DEFINITION(66, Common::PerformanceCounterType::AverageBase, L"Base for PLB TriggerMoveSecondary", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(67, 66, Common::PerformanceCounterType::AverageCount64, L"PLB TriggerMoveSecondary", L"Time taken for the PLB TriggerMoveSecondary function call")

                COUNTER_DEFINITION(68, Common::PerformanceCounterType::AverageBase, L"Base for PLB OnDroppedPLBMovement", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(69, 68, Common::PerformanceCounterType::AverageCount64, L"PLB OnDroppedPLBMovement", L"Time taken for the PLB OnDroppedPLBMovement function call")

                COUNTER_DEFINITION(70, Common::PerformanceCounterType::AverageBase, L"Base for PLB ToggleVerboseServicePlacementHealthReporting", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(71, 70, Common::PerformanceCounterType::AverageCount64, L"PLB ToggleVerboseServicePlacementHealthReporting", L"Time taken for the PLB ToggleVerboseServicePlacementHealthReporting function call")

                COUNTER_DEFINITION(72, Common::PerformanceCounterType::AverageBase, L"Base for PLB OnDroppedPLBMovements", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(73, 72, Common::PerformanceCounterType::AverageCount64, L"PLB OnDroppedPLBMovements", L"Time taken for the PLB OnDroppedPLBMovements function call")

                COUNTER_DEFINITION(74, Common::PerformanceCounterType::AverageBase, L"Base for PLB OnExecutePLBMovement", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(75, 74, Common::PerformanceCounterType::AverageCount64, L"PLB OnExecutePLBMovement", L"Time taken for the PLB OnExecutePLBMovement function call")

                COUNTER_DEFINITION(76, Common::PerformanceCounterType::AverageBase, L"Base for PLB UpdateClusterUpgrade", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(77, 76, Common::PerformanceCounterType::AverageCount64, L"PLB UpdateClusterUpgrade", L"Time taken for the PLB UpdateClusterUpgrade function call")

                COUNTER_DEFINITION(78, Common::PerformanceCounterType::AverageBase, L"Base for PLB OnFMBusy", L"", noDisplay)
                COUNTER_DEFINITION_WITH_BASE(79, 78, Common::PerformanceCounterType::AverageCount64, L"PLB OnFMBusy", L"Time taken for the PLB OnFMBusy function call")

                END_COUNTER_SET_DEFINITION()

            DECLARE_COUNTER_INSTANCE(NumberOfUnhealthyFailoverUnits)
            DECLARE_COUNTER_INSTANCE(NumberOfUnprocessedFailoverUnits)
            DECLARE_COUNTER_INSTANCE(NumberOfFailoverUnitActions)
            DECLARE_COUNTER_INSTANCE(FailoverUnitCommitDurationBase)
            DECLARE_COUNTER_INSTANCE(FailoverUnitCommitDuration)
            DECLARE_COUNTER_INSTANCE(FailoverUnitReconfigurationDurationBase)
            DECLARE_COUNTER_INSTANCE(FailoverUnitReconfigurationDuration)
            DECLARE_COUNTER_INSTANCE(FailoverUnitPlacementDurationBase)
            DECLARE_COUNTER_INSTANCE(FailoverUnitPlacementDuration)
            DECLARE_COUNTER_INSTANCE(NumberOfInBuildReplicas)
            DECLARE_COUNTER_INSTANCE(NumberOfStandByReplicas)
            DECLARE_COUNTER_INSTANCE(NumberOfOfflineReplicas)
            DECLARE_COUNTER_INSTANCE(NumberOfReplicas)
            DECLARE_COUNTER_INSTANCE(NumberOfFailoverUnits)
            DECLARE_COUNTER_INSTANCE(NumberOfInBuildFailoverUnits)
            DECLARE_COUNTER_INSTANCE(NumberOfServices)
            DECLARE_COUNTER_INSTANCE(NumberOfServiceTypes)
            DECLARE_COUNTER_INSTANCE(NumberOfApplications)
            DECLARE_COUNTER_INSTANCE(NumberOfApplicationUpgrades)
            DECLARE_COUNTER_INSTANCE(CommonQueueLength)
            DECLARE_COUNTER_INSTANCE(QueryQueueLength)
            DECLARE_COUNTER_INSTANCE(FailoverUnitQueueLength)
            DECLARE_COUNTER_INSTANCE(CommitQueueLength)
            DECLARE_COUNTER_INSTANCE(MessageProcessDurationBase)
            DECLARE_COUNTER_INSTANCE(MessageProcessDuration)
            DECLARE_COUNTER_INSTANCE(PlbSetMovementEnabledBase)
            DECLARE_COUNTER_INSTANCE(PlbSetMovementEnabled)
            DECLARE_COUNTER_INSTANCE(PlbUpdateNodeBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateNode)
            DECLARE_COUNTER_INSTANCE(PlbUpdateServiceTypeBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateServiceType)
            DECLARE_COUNTER_INSTANCE(PlbUpdateApplicationBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateApplication)
            DECLARE_COUNTER_INSTANCE(PlbDeleteApplicationBase)
            DECLARE_COUNTER_INSTANCE(PlbDeleteApplication)
            DECLARE_COUNTER_INSTANCE(PlbDeleteServiceTypeBase)
            DECLARE_COUNTER_INSTANCE(PlbDeleteServiceType)
            DECLARE_COUNTER_INSTANCE(PlbUpdateServiceBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateService)
            DECLARE_COUNTER_INSTANCE(PlbDeleteServiceBase)
            DECLARE_COUNTER_INSTANCE(PlbDeleteService)
            DECLARE_COUNTER_INSTANCE(PlbUpdateFailoverUnitBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateFailoverUnit)
            DECLARE_COUNTER_INSTANCE(PlbDeleteFailoverUnitBase)
            DECLARE_COUNTER_INSTANCE(PlbDeleteFailoverUnit)
            DECLARE_COUNTER_INSTANCE(PlbUpdateLoadOrMoveCostBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateLoadOrMoveCost)
            DECLARE_COUNTER_INSTANCE(PlbResetPartitionLoadBase)
            DECLARE_COUNTER_INSTANCE(PlbResetPartitionLoad)
            DECLARE_COUNTER_INSTANCE(PlbCompareNodeForPromotionBase)
            DECLARE_COUNTER_INSTANCE(PlbCompareNodeForPromotion)
            DECLARE_COUNTER_INSTANCE(PlbDisposeBase)
            DECLARE_COUNTER_INSTANCE(PlbDispose)
            DECLARE_COUNTER_INSTANCE(PlbGetClusterLoadInformationQueryResultBase)
            DECLARE_COUNTER_INSTANCE(PlbGetClusterLoadInformationQueryResult)
            DECLARE_COUNTER_INSTANCE(PlbGetNodeLoadInformationQueryResultBase)
            DECLARE_COUNTER_INSTANCE(PlbGetNodeLoadInformationQueryResult)
            DECLARE_COUNTER_INSTANCE(PlbGetUnplacedReplicaInformationQueryResultBase)
            DECLARE_COUNTER_INSTANCE(PlbGetUnplacedReplicaInformationQueryResult)
            DECLARE_COUNTER_INSTANCE(PlbTriggerPromoteToPrimaryBase)
            DECLARE_COUNTER_INSTANCE(PlbTriggerPromoteToPrimary)
            DECLARE_COUNTER_INSTANCE(PlbGetApplicationLoadInformationResultBase)
            DECLARE_COUNTER_INSTANCE(PlbGetApplicationLoadInformationResult)
            DECLARE_COUNTER_INSTANCE(PlbTriggerSwapPrimaryBase)
            DECLARE_COUNTER_INSTANCE(PlbTriggerSwapPrimary)
            DECLARE_COUNTER_INSTANCE(PlbTriggerMoveSecondaryBase)
            DECLARE_COUNTER_INSTANCE(PlbTriggerMoveSecondary)
            DECLARE_COUNTER_INSTANCE(PlbOnDroppedPLBMovementBase)
            DECLARE_COUNTER_INSTANCE(PlbOnDroppedPLBMovement)
            DECLARE_COUNTER_INSTANCE(PlbToggleVerboseServicePlacementHealthReportingBase)
            DECLARE_COUNTER_INSTANCE(PlbToggleVerboseServicePlacementHealthReporting)
            DECLARE_COUNTER_INSTANCE(PlbOnDroppedPLBMovementsBase)
            DECLARE_COUNTER_INSTANCE(PlbOnDroppedPLBMovements)
            DECLARE_COUNTER_INSTANCE(PlbOnExecutePLBMovementBase)
            DECLARE_COUNTER_INSTANCE(PlbOnExecutePLBMovement)
            DECLARE_COUNTER_INSTANCE(PlbUpdateClusterUpgradeBase)
            DECLARE_COUNTER_INSTANCE(PlbUpdateClusterUpgrade)
            DECLARE_COUNTER_INSTANCE(PlbOnFMBusyBase)
            DECLARE_COUNTER_INSTANCE(PlbOnFMBusy)

            BEGIN_COUNTER_SET_INSTANCE(FailoverUnitCounters)
                DEFINE_COUNTER_INSTANCE(NumberOfUnhealthyFailoverUnits, 1)
                DEFINE_COUNTER_INSTANCE(NumberOfUnprocessedFailoverUnits, 2)
                DEFINE_COUNTER_INSTANCE(NumberOfFailoverUnitActions, 3)
                DEFINE_COUNTER_INSTANCE(FailoverUnitCommitDurationBase, 4)
                DEFINE_COUNTER_INSTANCE(FailoverUnitCommitDuration, 5)
                DEFINE_COUNTER_INSTANCE(FailoverUnitReconfigurationDurationBase, 6)
                DEFINE_COUNTER_INSTANCE(FailoverUnitReconfigurationDuration, 7)
                DEFINE_COUNTER_INSTANCE(FailoverUnitPlacementDurationBase, 8)
                DEFINE_COUNTER_INSTANCE(FailoverUnitPlacementDuration, 9)
                DEFINE_COUNTER_INSTANCE(NumberOfInBuildReplicas, 10)
                DEFINE_COUNTER_INSTANCE(NumberOfStandByReplicas, 11)
                DEFINE_COUNTER_INSTANCE(NumberOfOfflineReplicas, 12)
                DEFINE_COUNTER_INSTANCE(NumberOfReplicas, 13)
                DEFINE_COUNTER_INSTANCE(NumberOfFailoverUnits, 14)
                DEFINE_COUNTER_INSTANCE(NumberOfInBuildFailoverUnits, 15)
                DEFINE_COUNTER_INSTANCE(NumberOfServices, 16)
                DEFINE_COUNTER_INSTANCE(NumberOfServiceTypes, 17)
                DEFINE_COUNTER_INSTANCE(NumberOfApplications, 18)
                DEFINE_COUNTER_INSTANCE(NumberOfApplicationUpgrades, 19)
                DEFINE_COUNTER_INSTANCE(CommonQueueLength, 20)
                DEFINE_COUNTER_INSTANCE(QueryQueueLength, 21)
                DEFINE_COUNTER_INSTANCE(FailoverUnitQueueLength, 22)
                DEFINE_COUNTER_INSTANCE(CommitQueueLength, 23)
                DEFINE_COUNTER_INSTANCE(MessageProcessDurationBase, 24)
                DEFINE_COUNTER_INSTANCE(MessageProcessDuration, 25)
                DEFINE_COUNTER_INSTANCE(PlbSetMovementEnabledBase, 26)
                DEFINE_COUNTER_INSTANCE(PlbSetMovementEnabled, 27)
                DEFINE_COUNTER_INSTANCE(PlbUpdateNodeBase, 28)
                DEFINE_COUNTER_INSTANCE(PlbUpdateNode, 29)
                DEFINE_COUNTER_INSTANCE(PlbUpdateServiceTypeBase, 30)
                DEFINE_COUNTER_INSTANCE(PlbUpdateServiceType, 31)
                DEFINE_COUNTER_INSTANCE(PlbUpdateApplicationBase, 32)
                DEFINE_COUNTER_INSTANCE(PlbUpdateApplication, 33)
                DEFINE_COUNTER_INSTANCE(PlbDeleteApplicationBase, 34)
                DEFINE_COUNTER_INSTANCE(PlbDeleteApplication, 35)
                DEFINE_COUNTER_INSTANCE(PlbDeleteServiceTypeBase, 36)
                DEFINE_COUNTER_INSTANCE(PlbDeleteServiceType, 37)
                DEFINE_COUNTER_INSTANCE(PlbUpdateServiceBase, 38)
                DEFINE_COUNTER_INSTANCE(PlbUpdateService, 39)
                DEFINE_COUNTER_INSTANCE(PlbDeleteServiceBase, 40)
                DEFINE_COUNTER_INSTANCE(PlbDeleteService, 41)
                DEFINE_COUNTER_INSTANCE(PlbUpdateFailoverUnitBase, 42)
                DEFINE_COUNTER_INSTANCE(PlbUpdateFailoverUnit, 43)
                DEFINE_COUNTER_INSTANCE(PlbDeleteFailoverUnitBase, 44)
                DEFINE_COUNTER_INSTANCE(PlbDeleteFailoverUnit, 45)
                DEFINE_COUNTER_INSTANCE(PlbUpdateLoadOrMoveCostBase, 46)
                DEFINE_COUNTER_INSTANCE(PlbUpdateLoadOrMoveCost, 47)
                DEFINE_COUNTER_INSTANCE(PlbResetPartitionLoadBase, 48)
                DEFINE_COUNTER_INSTANCE(PlbResetPartitionLoad, 49)
                DEFINE_COUNTER_INSTANCE(PlbCompareNodeForPromotionBase, 50)
                DEFINE_COUNTER_INSTANCE(PlbCompareNodeForPromotion, 51)
                DEFINE_COUNTER_INSTANCE(PlbDisposeBase, 52)
                DEFINE_COUNTER_INSTANCE(PlbDispose, 53)
                DEFINE_COUNTER_INSTANCE(PlbGetClusterLoadInformationQueryResultBase, 54)
                DEFINE_COUNTER_INSTANCE(PlbGetClusterLoadInformationQueryResult, 55)
                DEFINE_COUNTER_INSTANCE(PlbGetNodeLoadInformationQueryResultBase, 56)
                DEFINE_COUNTER_INSTANCE(PlbGetNodeLoadInformationQueryResult, 57)
                DEFINE_COUNTER_INSTANCE(PlbGetUnplacedReplicaInformationQueryResultBase, 58)
                DEFINE_COUNTER_INSTANCE(PlbGetUnplacedReplicaInformationQueryResult, 59)
                DEFINE_COUNTER_INSTANCE(PlbTriggerPromoteToPrimaryBase, 60)
                DEFINE_COUNTER_INSTANCE(PlbTriggerPromoteToPrimary, 61)
                DEFINE_COUNTER_INSTANCE(PlbGetApplicationLoadInformationResultBase, 62)
                DEFINE_COUNTER_INSTANCE(PlbGetApplicationLoadInformationResult, 63)
                DEFINE_COUNTER_INSTANCE(PlbTriggerSwapPrimaryBase, 64)
                DEFINE_COUNTER_INSTANCE(PlbTriggerSwapPrimary, 65)
                DEFINE_COUNTER_INSTANCE(PlbTriggerMoveSecondaryBase, 66)
                DEFINE_COUNTER_INSTANCE(PlbTriggerMoveSecondary, 67)
                DEFINE_COUNTER_INSTANCE(PlbOnDroppedPLBMovementBase, 68)
                DEFINE_COUNTER_INSTANCE(PlbOnDroppedPLBMovement, 69)
                DEFINE_COUNTER_INSTANCE(PlbToggleVerboseServicePlacementHealthReportingBase, 70)
                DEFINE_COUNTER_INSTANCE(PlbToggleVerboseServicePlacementHealthReporting, 71)
                DEFINE_COUNTER_INSTANCE(PlbOnDroppedPLBMovementsBase, 72)
                DEFINE_COUNTER_INSTANCE(PlbOnDroppedPLBMovements, 73)
                DEFINE_COUNTER_INSTANCE(PlbOnExecutePLBMovementBase, 74)
                DEFINE_COUNTER_INSTANCE(PlbOnExecutePLBMovement, 75)
                DEFINE_COUNTER_INSTANCE(PlbUpdateClusterUpgradeBase, 76)
                DEFINE_COUNTER_INSTANCE(PlbUpdateClusterUpgrade, 77)
                DEFINE_COUNTER_INSTANCE(PlbOnFMBusyBase, 78)
                DEFINE_COUNTER_INSTANCE(PlbOnFMBusy, 79)
                END_COUNTER_SET_INSTANCE()
        };

        typedef std::shared_ptr<FailoverUnitCounters> FailoverUnitCountersSPtr;
    }
}
