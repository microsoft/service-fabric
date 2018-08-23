// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/PerformanceCounter.h"
#include "PLBSchedulerAction.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadBalancingCounters
        {
            DENY_COPY(LoadBalancingCounters)

        public:

            bool isCreationsUpdated, isCreationsDurationUpdated,
                isCreationWithMoveUpdated, isCreationWithMoveDurationUpdated,
                isConstraintUpdated, isConstraintDurationUpdated,
                isFastBalancingUpdated, isFastBalancingDurationUpdated,
                isSlowBalancingUpdated, isSlowBalancingDurationUpdated;

            bool isNoneUpdated, isSwapUpdated,
                isMoveUpdated, isAddUpdated,
                isPromoteUpdated, isAddAndPromoteUpdated,
                isVoidUpdated, isDropUpdated;

            void ResetCategoricalCounterCheckStates();
            void UpdateAggregatePerformanceCounters(uint64 moveCount, uint64 msTimeCount);
            void UpdateCategoricalPerformanceCounters(uint64 moveCount, uint64 msTimeCount, PLBSchedulerAction perfCounterCategory , std::tuple<uint64, uint64, uint64, uint64, uint64, uint64, uint64, uint64> movementTypeTuple);
            void IncrementCategoricalPerformanceCounterBases();

            BEGIN_COUNTER_SET_DEFINITION(
                L"32091174-B9CB-4896-8047-CE4E63C2B0DF",
                L"Load Balancing Component",
                L"Counters for Load Balancing Component",
                Common::PerformanceCounterSetInstanceType::Multiple)

                COUNTER_DEFINITION(11, Common::PerformanceCounterType::AverageBase, L"#PLB-Generated Actions Base",                               L"PLB-Generated Actions Denominator", noDisplay)
                COUNTER_DEFINITION(12, Common::PerformanceCounterType::AverageBase, L"PLB-Generated Duration Base",                               L"PLB-Generated Durations Denominator", noDisplay)
                COUNTER_DEFINITION(13, Common::PerformanceCounterType::AverageBase, L"#PLBScheduler NewReplicaPlacement Actions Base",                       L"PLBScheduler NewReplicaPlacement Actions Generated Denominator", noDisplay)
                COUNTER_DEFINITION(14, Common::PerformanceCounterType::AverageBase, L"PLBScheduler NewReplicaPlacement Generation Duration Base",            L"PLBScheduler NewReplicaPlacement Generation Duration Denominator", noDisplay)
                COUNTER_DEFINITION(15, Common::PerformanceCounterType::AverageBase, L"#PLBScheduler ConstraintCheck Actions Base",                L"PLBScheduler ConstraintCheck Actions Generated Denominator", noDisplay)
                COUNTER_DEFINITION(16, Common::PerformanceCounterType::AverageBase, L"PLBScheduler ConstraintCheck Generation Duration Base",     L"PLBScheduler ConstraintCheck Generation Duration Denominator", noDisplay)
                COUNTER_DEFINITION(17, Common::PerformanceCounterType::AverageBase, L"#PLBScheduler QuickLoadBalancing Actions Base",                  L"PLBScheduler QuickLoadBalancing Actions Generated Denominator", noDisplay)
                COUNTER_DEFINITION(18, Common::PerformanceCounterType::AverageBase, L"PLBScheduler QuickLoadBalancing Generation Duration Base",       L"PLBScheduler QuickLoadBalancing Generation Duration Denominator", noDisplay)
                COUNTER_DEFINITION(19, Common::PerformanceCounterType::AverageBase, L"#PLBScheduler LoadBalancing Actions Base",                  L"PLBScheduler LoadBalancing Actions Generated Denominator", noDisplay)
                COUNTER_DEFINITION(20, Common::PerformanceCounterType::AverageBase, L"PLBScheduler LoadBalancing Generation Duration Base",       L"PLBScheduler LoadBalancing Generation Duration Denominator", noDisplay)

                COUNTER_DEFINITION(38, Common::PerformanceCounterType::AverageBase, L"#PLBScheduler NewReplicaPlacementWithMove Actions Base",               L"PLBScheduler NewReplicaPlacementWithMove Actions Generated Denominator", noDisplay)
                COUNTER_DEFINITION(39, Common::PerformanceCounterType::AverageBase, L"PLBScheduler NewReplicaPlacementWithMove Generation Duration Base",    L"PLBScheduler NewReplicaPlacementWithMove Generation Duration Denominator", noDisplay)

                COUNTER_DEFINITION(31, Common::PerformanceCounterType::AverageBase, L"PLB Movements None Base",                                 L"PLB Movement Type None Denominator", noDisplay)
                COUNTER_DEFINITION(32, Common::PerformanceCounterType::AverageBase, L"PLB Movements Swap Base",                                 L"PLB Movement Type Swap Denominator", noDisplay)
                COUNTER_DEFINITION(33, Common::PerformanceCounterType::AverageBase, L"PLB Movements Move Base",                                 L"PLB Movement Type Move Denominator", noDisplay)
                COUNTER_DEFINITION(34, Common::PerformanceCounterType::AverageBase, L"PLB Movements Add Base",                                  L"PLB Movement Type Add Denominator", noDisplay)
                COUNTER_DEFINITION(35, Common::PerformanceCounterType::AverageBase, L"PLB Movements Promote Base",                              L"PLB Movement Type Promote Denominator", noDisplay)
                COUNTER_DEFINITION(36, Common::PerformanceCounterType::AverageBase, L"PLB Movements AddAndPromote Base",                        L"PLB Movement Type AddAndPromote Denominator", noDisplay)
                COUNTER_DEFINITION(37, Common::PerformanceCounterType::AverageBase, L"PLB Movements Void Base",                                 L"PLB Movement Type Void Denominator", noDisplay)
                COUNTER_DEFINITION(50, Common::PerformanceCounterType::AverageBase, L"PLB Movements Drop Base",                                 L"PLB Movement Type Drop Denominator", noDisplay)

                COUNTER_DEFINITION(200, Common::PerformanceCounterType::AverageBase, L"PLB Movements Dropped By FM Base",                       L"PLB Movements Dropped By FM Denominator", noDisplay)
                COUNTER_DEFINITION(201, Common::PerformanceCounterType::AverageBase, L"PLB MovementThrottled FailoverUnits Base",               L"PLB MovementThrottled FailoverUnits Denominator", noDisplay)
                COUNTER_DEFINITION(202, Common::PerformanceCounterType::AverageBase, L"PLB Pending Movements Base",                             L"PLB Pending Movements Denominator", noDisplay)

                COUNTER_DEFINITION_WITH_BASE( 1, 11, Common::PerformanceCounterType::AverageCount64, L"#PLB-Generated Actions",                                   L"Moving average of actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE( 2, 12, Common::PerformanceCounterType::AverageCount64, L"PLB Action Generation Duration",                           L"Moving average of Time in milliseconds it took the PLB to generate the actions during one Refresh")
                COUNTER_DEFINITION_WITH_BASE( 3, 13, Common::PerformanceCounterType::AverageCount64, L"#PLBScheduler NewReplicaPlacement Actions",                           L"Moving average of PLBScheduler NewReplicaPlacement Actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE( 4, 14, Common::PerformanceCounterType::AverageCount64, L"PLBScheduler NewReplicaPlacement Generation Duration",                L"Moving average of Time in milliseconds it took the PLB to generate the PLBScheduler NewReplicaPlacement Actions")
                COUNTER_DEFINITION_WITH_BASE( 5, 15, Common::PerformanceCounterType::AverageCount64, L"#PLBScheduler ConstraintCheck Actions",                    L"Moving average of PLBScheduler ConstraintCheck Actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE( 6, 16, Common::PerformanceCounterType::AverageCount64, L"PLBScheduler ConstraintCheck Generation Duration",         L"Moving average of Time in milliseconds it took the PLB to generate the PLBScheduler ConstraintCheck Actions")
                COUNTER_DEFINITION_WITH_BASE( 7, 17, Common::PerformanceCounterType::AverageCount64, L"#PLBScheduler QuickLoadBalancing Actions",                      L"Moving average of PLBScheduler QuickLoadBalancing Actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE( 8, 18, Common::PerformanceCounterType::AverageCount64, L"PLBScheduler QuickLoadBalancing Generation Duration",           L"Moving average of Time in milliseconds it took the PLB to generate the PLBScheduler QuickLoadBalancing Actions")
                COUNTER_DEFINITION_WITH_BASE( 9, 19, Common::PerformanceCounterType::AverageCount64, L"#PLBScheduler LoadBalancing Actions",                      L"Moving average of PLBScheduler LoadBalancing Actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE(10, 20, Common::PerformanceCounterType::AverageCount64, L"PLBScheduler LoadBalancing Generation Duration",           L"Moving average of Time in milliseconds it took the PLB to generate the PLBScheduler LoadBalancing Actions")

                COUNTER_DEFINITION_WITH_BASE(21, 31, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements None",                   L"Moving average of number of number of None movements")
                COUNTER_DEFINITION_WITH_BASE(22, 32, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Swap",                   L"Moving average of number of number of Swap movements")
                COUNTER_DEFINITION_WITH_BASE(23, 33, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Move",                   L"Moving average of number of number of Move movements")
                COUNTER_DEFINITION_WITH_BASE(24, 34, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Add",                    L"Moving average of number of number of Add movements")
                COUNTER_DEFINITION_WITH_BASE(25, 35, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Promote",                L"Moving average of number of number of Promote movements")
                COUNTER_DEFINITION_WITH_BASE(26, 36, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements AddAndPromote",          L"Moving average of number of number of AddAndPromote movements")
                COUNTER_DEFINITION_WITH_BASE(27, 37, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Void",                   L"Moving average of number of number of Void movements")
                COUNTER_DEFINITION_WITH_BASE(40, 50, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Drop",                   L"Moving average of number of number of Drop movements")

                COUNTER_DEFINITION_WITH_BASE(28, 38, Common::PerformanceCounterType::AverageCount64, L"#PLBScheduler NewReplicaPlacementWithMove Actions", L"Moving average of PLBScheduler NewReplicaPlacementWithMove Actions generated by the PLB during one Refresh")
                COUNTER_DEFINITION_WITH_BASE(29, 39, Common::PerformanceCounterType::AverageCount64, L"PLBScheduler NewReplicaPlacementWithMove Generation Duration", L"Moving average of Time in milliseconds it took the PLB to generate the PLBScheduler NewReplicaPlacementWithMove Actions")

                COUNTER_DEFINITION_WITH_BASE(100, 200, Common::PerformanceCounterType::AverageCount64, L"#PLB Movements Dropped By FM", L"Moving average of number of PLB Movements Dropped By FM")
                COUNTER_DEFINITION_WITH_BASE(101, 201, Common::PerformanceCounterType::AverageCount64, L"#PLB MovementThrottled FailoverUnits", L"Moving average of the number of FailoverUnits in all ServiceDomains that are throttled by the PLB")
                COUNTER_DEFINITION_WITH_BASE(102, 202, Common::PerformanceCounterType::AverageCount64, L"#PLB Pending Movements", L"Moving average of the number of Pending Movements in all ServiceDomains MovePlans")

            END_COUNTER_SET_DEFINITION()


            DECLARE_COUNTER_INSTANCE(NumberOfPLBGeneratedActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingPLBActions)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationActions)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationWithMoveActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationWithMoveActions)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerConstraintCheckActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingConstraintCheckActions)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerFastBalancingActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingFastBalancingActions)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerSlowBalancingActions)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingSlowBalancingActions)


            DECLARE_COUNTER_INSTANCE(NumberOfPLBGeneratedActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingPLBActionsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationActionsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationWithMoveActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationWithMoveActionsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerConstraintCheckActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingConstraintCheckActionsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerFastBalancingActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingFastBalancingActionsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSchedulerSlowBalancingActionsBase)
            DECLARE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingSlowBalancingActionsBase)


            DECLARE_COUNTER_INSTANCE(NumberOfPLBNoneMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBNoneMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBSwapMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBSwapMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBMoveMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBMoveMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBAddMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBAddMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBPromoteMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBPromoteMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBAddAndPromoteMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBAddAndPromoteMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBVoidMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBVoidMovementsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBDropMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBDropMovementsBase)

            
            DECLARE_COUNTER_INSTANCE(NumberOfPLBMovementsDroppedByFM)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBMovementsDroppedByFMBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBMovementThrottledFailoverUnits)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBMovementThrottledFailoverUnitsBase)

            DECLARE_COUNTER_INSTANCE(NumberOfPLBPendingMovements)
            DECLARE_COUNTER_INSTANCE(NumberOfPLBPendingMovementsBase)

            BEGIN_COUNTER_SET_INSTANCE(LoadBalancingCounters)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBGeneratedActions, 1)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingPLBActions, 2)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationActions, 3)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationActions, 4)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationWithMoveActions, 28)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationWithMoveActions, 29)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerConstraintCheckActions, 5)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingConstraintCheckActions, 6)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerFastBalancingActions, 7)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingFastBalancingActions, 8)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerSlowBalancingActions, 9)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingSlowBalancingActions, 10)


                DEFINE_COUNTER_INSTANCE(NumberOfPLBGeneratedActionsBase, 11)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingPLBActionsBase, 12)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationActionsBase, 13)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationActionsBase, 14)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerCreationWithMoveActionsBase, 38)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingCreationWithMoveActionsBase, 39)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerConstraintCheckActionsBase, 15)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingConstraintCheckActionsBase, 16)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerFastBalancingActionsBase, 17)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingFastBalancingActionsBase, 18)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBSchedulerSlowBalancingActionsBase, 19)
                DEFINE_COUNTER_INSTANCE(NumberOfMillisecondsGeneratingSlowBalancingActionsBase, 20)


                DEFINE_COUNTER_INSTANCE(NumberOfPLBNoneMovements, 21)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBSwapMovements, 22)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBMoveMovements, 23)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBAddMovements, 24)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBPromoteMovements, 25)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBAddAndPromoteMovements, 26)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBVoidMovements, 27)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBDropMovements, 40)


                DEFINE_COUNTER_INSTANCE(NumberOfPLBNoneMovementsBase, 31)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBSwapMovementsBase, 32)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBMoveMovementsBase, 33)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBAddMovementsBase, 34)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBPromoteMovementsBase, 35)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBAddAndPromoteMovementsBase, 36)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBVoidMovementsBase, 37)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBDropMovementsBase, 50)


                DEFINE_COUNTER_INSTANCE(NumberOfPLBMovementsDroppedByFM, 100)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBMovementsDroppedByFMBase, 200)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBMovementThrottledFailoverUnits, 101)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBMovementThrottledFailoverUnitsBase, 201)

                DEFINE_COUNTER_INSTANCE(NumberOfPLBPendingMovements, 102)
                DEFINE_COUNTER_INSTANCE(NumberOfPLBPendingMovementsBase, 202)

            END_COUNTER_SET_INSTANCE()
        };

        typedef std::shared_ptr<LoadBalancingCounters> LoadBalancingCountersSPtr;
    }
}
