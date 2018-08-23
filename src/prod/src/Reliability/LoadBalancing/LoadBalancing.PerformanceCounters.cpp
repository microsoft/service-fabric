// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Reliability/LoadBalancing/LoadBalancing.PerformanceCounters.h"

using namespace Reliability::LoadBalancingComponent;

void LoadBalancingCounters::ResetCategoricalCounterCheckStates()
{
    isCreationsUpdated = false;
    isCreationsDurationUpdated = false;
    isCreationWithMoveUpdated = false;
    isCreationWithMoveDurationUpdated = false;
    isConstraintUpdated = false;
    isConstraintDurationUpdated = false;
    isFastBalancingUpdated = false;
    isFastBalancingDurationUpdated = false;
    isSlowBalancingUpdated = false;
    isSlowBalancingDurationUpdated = false;

    isNoneUpdated = false;
    isSwapUpdated = false;
    isMoveUpdated = false;
    isAddUpdated = false;
    isPromoteUpdated = false;
    isAddAndPromoteUpdated = false;
    isVoidUpdated = false;
    isDropUpdated = false;
}

void LoadBalancingCounters::UpdateAggregatePerformanceCounters(uint64 moveCount, uint64 msTimeCount)
{
    NumberOfPLBGeneratedActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { NumberOfPLBGeneratedActionsBase.Increment(); }
    NumberOfMillisecondsGeneratingPLBActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { NumberOfMillisecondsGeneratingPLBActionsBase.Increment(); }
}

void LoadBalancingCounters::UpdateCategoricalPerformanceCounters(uint64 moveCount, uint64 msTimeCount, PLBSchedulerAction perfCounterCategory , std::tuple<uint64, uint64, uint64, uint64, uint64, uint64, uint64, uint64> movementTypeTuple)
{

    switch (perfCounterCategory.Action)
    {
    case PLBSchedulerActionType::NewReplicaPlacement:
        NumberOfPLBSchedulerCreationActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { isCreationsUpdated = true; }
        NumberOfMillisecondsGeneratingCreationActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { isCreationsDurationUpdated = true; }
        break;
    case PLBSchedulerActionType::NewReplicaPlacementWithMove:
        NumberOfPLBSchedulerCreationWithMoveActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { isCreationWithMoveUpdated = true; }
        NumberOfMillisecondsGeneratingCreationWithMoveActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { isCreationWithMoveDurationUpdated = true; }
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        NumberOfPLBSchedulerConstraintCheckActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { isConstraintUpdated = true; }
        NumberOfMillisecondsGeneratingConstraintCheckActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { isConstraintDurationUpdated = true; }
        break;
    case PLBSchedulerActionType::QuickLoadBalancing:
        NumberOfPLBSchedulerFastBalancingActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { isFastBalancingUpdated = true; }
        NumberOfMillisecondsGeneratingFastBalancingActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { isFastBalancingDurationUpdated = true; }
        break;
    case PLBSchedulerActionType::LoadBalancing:
        NumberOfPLBSchedulerSlowBalancingActions.Value += static_cast<Common::PerformanceCounterValue>(moveCount); if (moveCount != 0) { isSlowBalancingUpdated = true; }
        NumberOfMillisecondsGeneratingSlowBalancingActions.Value += static_cast<Common::PerformanceCounterValue>(msTimeCount); if (msTimeCount != 0) { isSlowBalancingDurationUpdated = true; }
        break;
    default:
        break;
    }

    uint64 temp = 0;

    NumberOfPLBNoneMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<0>(movementTypeTuple)); if (temp) { isNoneUpdated = true; }
    NumberOfPLBSwapMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<1>(movementTypeTuple)); if (temp) { isSwapUpdated = true; }
    NumberOfPLBMoveMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<2>(movementTypeTuple)); if (temp) { isMoveUpdated = true; }
    NumberOfPLBAddMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<3>(movementTypeTuple)); if (temp) { isAddUpdated = true; }
    NumberOfPLBPromoteMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<4>(movementTypeTuple)); if (temp) { isPromoteUpdated = true; }
    NumberOfPLBAddAndPromoteMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<5>(movementTypeTuple)); if (temp) { isAddAndPromoteUpdated = true; }
    NumberOfPLBVoidMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<6>(movementTypeTuple)); if (temp) { isVoidUpdated = true; }
    NumberOfPLBDropMovements.Value += static_cast<Common::PerformanceCounterValue>(temp = std::get<7>(movementTypeTuple)); if (temp) { isDropUpdated = true; }
}

void LoadBalancingCounters::IncrementCategoricalPerformanceCounterBases()
{

    if (isCreationsUpdated)             NumberOfPLBSchedulerCreationActionsBase.Increment();
    if (isCreationsDurationUpdated)     NumberOfMillisecondsGeneratingCreationActionsBase.Increment();
    if (isCreationWithMoveUpdated)             NumberOfPLBSchedulerCreationActionsBase.Increment();
    if (isCreationWithMoveDurationUpdated)     NumberOfMillisecondsGeneratingCreationActionsBase.Increment();
    if (isConstraintUpdated)            NumberOfPLBSchedulerConstraintCheckActionsBase.Increment();
    if (isConstraintDurationUpdated)    NumberOfMillisecondsGeneratingConstraintCheckActionsBase.Increment();
    if (isFastBalancingUpdated)         NumberOfPLBSchedulerFastBalancingActionsBase.Increment();
    if (isFastBalancingDurationUpdated) NumberOfMillisecondsGeneratingFastBalancingActionsBase.Increment();
    if (isSlowBalancingUpdated)         NumberOfPLBSchedulerSlowBalancingActionsBase.Increment();
    if (isSlowBalancingDurationUpdated) NumberOfMillisecondsGeneratingSlowBalancingActionsBase.Increment();

    if (isNoneUpdated) NumberOfPLBNoneMovementsBase.Increment();
    if (isSwapUpdated) NumberOfPLBSwapMovementsBase.Increment();
    if (isMoveUpdated) NumberOfPLBMoveMovementsBase.Increment();
    if (isAddUpdated) NumberOfPLBAddMovementsBase.Increment();
    if (isPromoteUpdated) NumberOfPLBPromoteMovementsBase.Increment();
    if (isAddAndPromoteUpdated) NumberOfPLBAddAndPromoteMovementsBase.Increment();
    if (isVoidUpdated) NumberOfPLBVoidMovementsBase.Increment();
    if (isDropUpdated) NumberOfPLBDropMovementsBase.Increment();

    ResetCategoricalCounterCheckStates();
}

INITIALIZE_COUNTER_SET(LoadBalancingCounters)
