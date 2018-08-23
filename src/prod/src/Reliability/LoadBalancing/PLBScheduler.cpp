// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBScheduler.h"
#include "ISystemState.h"
#include "PLBConfig.h"
#include "PLBEventSource.h"
#include "FailoverUnitMovement.h"
#include "SystemState.h"
#include "ServiceDomain.h"
#include "PLBDiagnostics.h"


using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PLBScheduler::PLBScheduler(
    wstring const& serviceDomainId,
    PLBEventSource const & trace,
    bool constraintCheckEnabled,
    bool balancingEnabled,
    StopwatchTime lastNodeDown,
    StopwatchTime lastBalancing,
    StopwatchTime lastNewNode)
    : serviceDomainId_(serviceDomainId),
      trace_(trace),
      constraintCheckEnabled_(constraintCheckEnabled),
      balancingEnabled_(balancingEnabled),
      lastStateChange_(StopwatchTime::Zero),
      lastLoadChange_(StopwatchTime::Zero),
      lastNodeDown_(lastNodeDown),
      lastNewNode_(lastNewNode),
      currentAction_(),
      placementAction_(),
      balancingAction_(),
      lastActionTime_(StopwatchTime::Zero),
      lastPlacement_(StopwatchTime::Zero),
      lastConstraintCheck_(StopwatchTime::Zero),
      lastBalancing_(lastBalancing),
      hasLastBalancingAvgStdDev_(false),
      lastBalancingAvgStdDev_(),
      movementCounterPerFU_()
{
    placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
    balancingAction_.SetAction(PLBSchedulerActionType::QuickLoadBalancing);
}

PLBScheduler::PLBScheduler(PLBScheduler && other)
    : serviceDomainId_(other.serviceDomainId_),
      trace_(other.trace_),
      constraintCheckEnabled_(other.constraintCheckEnabled_),
      balancingEnabled_(other.balancingEnabled_),
      lastStateChange_(other.lastStateChange_),
      lastLoadChange_(other.lastLoadChange_),
      lastNodeDown_(other.lastNodeDown_),
      lastNewNode_(other.lastNewNode_),
      currentAction_(other.currentAction_),
      placementAction_(other.placementAction_),
      balancingAction_(other.balancingAction_),
      lastActionTime_(other.lastActionTime_),
      lastPlacement_(other.lastPlacement_),
      lastConstraintCheck_(other.lastConstraintCheck_),
      lastBalancing_(other.lastBalancing_),
      hasLastBalancingAvgStdDev_(other.hasLastBalancingAvgStdDev_),
      lastBalancingAvgStdDev_(other.lastBalancingAvgStdDev_),
      movementCounterPerFU_(move(other.movementCounterPerFU_))
{
}

void PLBScheduler::OnNodeUp(StopwatchTime timeStamp)
{
    SystemStateChanged(timeStamp);
    lastNewNode_ = timeStamp;
}

void PLBScheduler::OnNodeDown(StopwatchTime timeStamp)
{
    SystemStateChanged(timeStamp);
    lastNodeDown_ = timeStamp;
}

void PLBScheduler::OnNodeChanged(StopwatchTime timeStamp)
{
    SystemStateChanged(timeStamp);
}

void PLBScheduler::OnServiceTypeChanged(StopwatchTime timeStamp)
{
    SystemStateChanged(timeStamp);
}

void PLBScheduler::OnServiceChanged(StopwatchTime timeStamp)
{
    SystemStateChanged(timeStamp);
}

void PLBScheduler::OnServiceDeleted(StopwatchTime timeStamp)
{
    timeStamp;
}

void PLBScheduler::OnFailoverUnitAdded(StopwatchTime timeStamp, Guid fuId)
{
    fuId;
    SystemStateChanged(timeStamp);
}

void PLBScheduler::OnFailoverUnitChanged(StopwatchTime timeStamp, Guid fuId)
{
    fuId;
    SystemStateChanged(timeStamp);
}

void PLBScheduler::OnFailoverUnitDeleted(StopwatchTime timeStamp, Common::Guid fuId)
{
    SystemStateChanged(timeStamp);

    movementCounterPerFU_.erase(fuId);
}

void PLBScheduler::OnLoadChanged(StopwatchTime timeStamp)
{
    lastLoadChange_ = timeStamp;
}

void PLBScheduler::OnDomainInterrupted(StopwatchTime timeStamp)
{
    // If search was interrupted, we will allow one more round of balancing
    SystemStateChanged(timeStamp);
}

void PLBScheduler::SetAction(PLBSchedulerActionType::Enum action)
{
    currentAction_.SetAction(action);
}

void PLBScheduler::RefreshAction(ISystemState const & systemState, StopwatchTime timeStamp, SchedulingDiagnostics::ServiceDomainSchedulerStageDiagnosticsSPtr stageDiagnosticsSPtr /* = nullptr */)
{
    RefreshTimedCounters(timeStamp);

    // To handle the Test cases in plbscheduler.test.cpp
    if (stageDiagnosticsSPtr == nullptr)
    {
        stageDiagnosticsSPtr = make_shared<ServiceDomainSchedulerStageDiagnostics>();
    }

    currentAction_.IsSkip = false;

    PLBConfig & config = PLBConfig::GetConfig();

    Common::TimeSpan minConstraintCheckInterval = GetMinConstraintCheckInterval();
    Common::TimeSpan minPlacementInterval = GetMinPlacementInterval();

    // This list takes all actions that are due to run.
    vector<pair<Common::StopwatchTime, SchedulerStage::Stage>> expiredActions;

    //Diagnostics
    {
        stageDiagnosticsSPtr->newReplicaNeeded_ = systemState.HasNewReplica();
        stageDiagnosticsSPtr->upgradeNeeded_ = systemState.HasPartitionWithReplicaInUpgrade();
        stageDiagnosticsSPtr->dropNeeded_ = systemState.HasExtraReplicas();
    }


    bool isCreationActionNeeded = systemState.HasNewReplica() || systemState.HasPartitionWithReplicaInUpgrade() || systemState.HasExtraReplicas();

    bool const constraintCheckEnabledLocalConfig = PLBConfig::GetConfig().ConstraintCheckEnabled;
    bool const loadBalancingEnabledLocalConfig = PLBConfig::GetConfig().LoadBalancingEnabled;
    bool const canBalanceDueToAppUpgrade = !systemState.HasPartitionInAppUpgrade() || PLBConfig::GetConfig().AllowBalancingDuringApplicationUpgrade;

    //Diagnostics
    {
        TESTASSERT_IF(stageDiagnosticsSPtr->CreationActionNeeded() != isCreationActionNeeded, "Diagnostics CreationActionNeeded Method must be updated to match, failed for sd {0}", serviceDomainId_);

        stageDiagnosticsSPtr->constraintCheckEnabledConfig_ = constraintCheckEnabledLocalConfig;
        stageDiagnosticsSPtr->constraintCheckPaused_ = !constraintCheckEnabled_;

        stageDiagnosticsSPtr->balancingEnabledConfig_ = loadBalancingEnabledLocalConfig;
        //there will be a messaged surfaced that upgrades caused balancing not to run
        //in the first case it is due to fabric upgrade
        //in the second it is due to app upgrade
        stageDiagnosticsSPtr->balancingPaused_ = !balancingEnabled_ || !canBalanceDueToAppUpgrade;
    }

    bool isBalancingEnabled = loadBalancingEnabledLocalConfig && balancingEnabled_ && canBalanceDueToAppUpgrade;
    bool isConstraintCheckEnabled = constraintCheckEnabledLocalConfig && constraintCheckEnabled_;

    bool const placementExpired = (timeStamp >= lastPlacement_ + minPlacementInterval);
    bool const constraintCheckExpired = (timeStamp >= lastConstraintCheck_ + minConstraintCheckInterval);
    bool const balancingExpired = (timeStamp >= lastBalancing_ + config.MinLoadBalancingInterval);
    bool const hasMovableReplica = systemState.HasMovableReplica();

    //Diagnostics
    {
        TESTASSERT_IF( (stageDiagnosticsSPtr->constraintCheckEnabledConfig_ && !stageDiagnosticsSPtr->constraintCheckPaused_) != isConstraintCheckEnabled, "Diagnostics must match isConstraintCheckEnabled");
        TESTASSERT_IF( (stageDiagnosticsSPtr->balancingEnabledConfig_ && !stageDiagnosticsSPtr->balancingPaused_) != isBalancingEnabled, "Diagnostics must match isBalancingEnabled");

        stageDiagnosticsSPtr->placementExpired_ = placementExpired;
        stageDiagnosticsSPtr->constraintCheckExpired_ = constraintCheckExpired;
        stageDiagnosticsSPtr->balancingExpired_ = balancingExpired;
        stageDiagnosticsSPtr->hasMovableReplica_ = hasMovableReplica;
    }

    if (placementExpired)
    {
        expiredActions.push_back(make_pair(lastPlacement_ + minPlacementInterval, SchedulerStage::Stage::Placement));
    }

    if (hasMovableReplica && isConstraintCheckEnabled && constraintCheckExpired)
    {
        expiredActions.push_back(make_pair(lastConstraintCheck_ + minConstraintCheckInterval, SchedulerStage::Stage::ConstraintCheck));
    }

    if (hasMovableReplica && isBalancingEnabled && balancingExpired)
    {
        expiredActions.push_back(make_pair(lastBalancing_ + config.MinLoadBalancingInterval, SchedulerStage::Stage::Balancing));
    }

    bool const noExpiredTimerActions = expiredActions.empty();

    //Diagnostics
    {
        stageDiagnosticsSPtr->noExpiredTimerActions_ = noExpiredTimerActions;
    }

    // If no timers have expired, exit with no action needed
    if (noExpiredTimerActions)
    {
        trace_.Scheduler(serviceDomainId_, wformatString("There are no action eligible for execution in service domain {0}", serviceDomainId_));
        currentAction_.End();

        //Diagnostics
        {
            stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::NoneExpired;
            stageDiagnosticsSPtr->decisionScheduled_ = false;
        }
        return;
    }

    // Look at the actions in order: from the one that waited the longest
    stable_sort(expiredActions.begin(), expiredActions.end(), [](std::pair<Common::StopwatchTime, SchedulerStage::Stage> first, std::pair<Common::StopwatchTime, SchedulerStage::Stage> second){
        return first.first < second.first;
    });

    bool const creationWithMoveEnabled = config.MoveExistingReplicaForPlacement;

    for (auto it = expiredActions.begin(); it != expiredActions.end(); it++)
    {
        SchedulerStage::Stage actionToDo = it->second;
        std::vector<std::shared_ptr<IConstraintDiagnosticsData>> constraintsDiagnosticsData;

        switch (actionToDo)
        {
        case SchedulerStage::Stage::Placement: // Placement stage
            lastPlacement_ = timeStamp;
            if (isCreationActionNeeded)
            {
                if (systemState.HasPartitionWithReplicaInUpgrade())
                {
                    SystemState const& state = dynamic_cast<SystemState const&>(systemState);
                    ServiceDomain& domain = state.serviceDomain_;
                    if (!domain.PartitionsInUpgrade.empty())
                    {
                        trace_.Scheduler(serviceDomainId_, wformatString("Service domain partition {0} is in upgrade", *domain.PartitionsInUpgrade.begin()));
                    }
                }

                //Re-evaluate throttling conditions for creationwithmove...
                if (placementAction_.IsCreationWithMove() && IsCreationWithMoveThrottled(systemState))
                {
                    // NewReplicaPlacementWithMove is throttled, fall back to creation
                    placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
                }

                // Select this action (NewReplicaPlacement or NewReplicaPlacementWithMove) to run for this service domain
                currentAction_ = placementAction_;

                // Advance the creation action, so that we can switch to NewReplicaPlacementWithMove if enabled.
                // This value will be looked at only after MinPlacementInterval passes again
                placementAction_.IncreaseIteration();
                if (!placementAction_.CanRetry())
                {
                    if (placementAction_.Action == PLBSchedulerActionType::NewReplicaPlacement && creationWithMoveEnabled)
                    {
                            // NewReplicaPlacement has reached maximum number of attempts,
                            // try NewReplicaPlacementWithMove after MinPlacementInterval passes
                            placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacementWithMove);
                    }
                    else
                    {
                        // NewReplicaPlacementWithMove can't retry, fall back to creation
                        placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
                    }
                }

                //Diagnostics
                {
                    stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::Placement;
                    stageDiagnosticsSPtr->decisionScheduled_ = true;
                }

                trace_.Scheduler(serviceDomainId_, wformatString("Current action is {0}; system state has new replica? {1}, has upgrade partition? {2}",
                L"Placement", systemState.HasNewReplica(), systemState.HasPartitionWithReplicaInUpgrade()));
                return;
            }
            else
            {
                // Placement stage is not needed, just pass onto next eligible action
                trace_.SchedulerActionSkip(serviceDomainId_, L"Placement", L"it is not needed");
                placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
            }
            break;
        case SchedulerStage::Stage::ConstraintCheck: // ConstraintCheck stage
            lastConstraintCheck_ = timeStamp;
            stageDiagnosticsSPtr->wasConstraintCheckRun_ = true;
            if (systemState.IsConstraintSatisfied(constraintsDiagnosticsData))
            {
                // ConstraintCheck stage is not needed. Set lastConstraintCheck_ and goto check next stage
                trace_.SchedulerActionSkip(serviceDomainId_, L"ConstraintCheck", L"it is not needed");
            }
            else
            {
                //Diagnostics
                {
                    stageDiagnosticsSPtr->constraintsDiagnosticsData_ = move(constraintsDiagnosticsData);
                }

                bool nodeThrottled = IsConstraintCheckThrottled(timeStamp);
                bool allViolationsSkippable = stageDiagnosticsSPtr->AllViolationsSkippable();



                if (!nodeThrottled)
                {
                    // Select ConstraintCheck action to run for this service domain. No tracing here, since PLBDomainStart will trace the action.
                    currentAction_.SetAction(PLBSchedulerActionType::ConstraintCheck);

                    //Diagnostics
                    {
                        stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::ConstraintCheck;
                        stageDiagnosticsSPtr->decisionScheduled_ = true;
                    }

                    return;
                }
                else
                {
                    //Diagnostics
                    {
                        stageDiagnosticsSPtr->isConstraintCheckNodeChangeThrottled_ = true;
                    }

                    if (!allViolationsSkippable)
                    {
                        // Select ConstraintCheck action to run for this service domain. No tracing here, since PLBDomainStart will trace the action.
                        currentAction_.SetAction(PLBSchedulerActionType::ConstraintCheck);
                        currentAction_.SetConstraintCheckLightness(true);

                        //Diagnostics
                        {
                            stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::ConstraintCheck;
                            stageDiagnosticsSPtr->decisionScheduled_ = true;
                        }
                        return;
                    }
                }
            }
            break;
        case SchedulerStage::Stage::Balancing: // Balancing stage
            if (IsBalancingThrottled(timeStamp, systemState))
            {
                //Diagnostics
                {
                    stageDiagnosticsSPtr->isbalanced_ = false;
                    stageDiagnosticsSPtr->isBalancingNodeChangeThrottled_ = true;
                }

                // Goto next check, do not update timestamp since we want to give balancing a chance if it is throttled by node up/down events
                continue;
            }
            if (systemState.IsBalanced())
            {
                trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", L"system is balanced");
                lastBalancing_ = timeStamp;
                // QuickLoadBalancing did the job, no need to move towards LoadBalancing
                balancingAction_.SetAction(PLBSchedulerActionType::QuickLoadBalancing);

                //Diagnostics
                {
                    stageDiagnosticsSPtr->isbalanced_ = true;
                }

                continue;
            }
            if (ThrottleBalancingForSmallStdDevChange(systemState))
            {
                lastBalancing_ = timeStamp;

                //Diagnostics
                {
                    stageDiagnosticsSPtr->isbalanced_ = false;
                    stageDiagnosticsSPtr->isBalancingSmallChangeThrottled_ = true;
                }
            }
            else
            {
                // Choose current balancing action to be run for this service domain
                lastBalancing_ = timeStamp;
                currentAction_ = balancingAction_;
                //Diagnostics
                {
                    stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::Balancing;
                    stageDiagnosticsSPtr->decisionScheduled_ = true;
                }

                // Advance the balancing action, so that we can switch to LoadBalancing if QuickLoadBalancing does not find a solution.
                // This value will be looked at only after MinLoadBalancingInterval passes again
                balancingAction_.IncreaseIteration();
                if (!balancingAction_.CanRetry())
                {
                    if (balancingAction_.Action == PLBSchedulerActionType::QuickLoadBalancing)
                    {
                        // After we do QuickLoadBalancing for PLBActionRetryTimes times, switch to slow after MinBalancingIntervalExpires.
                        balancingAction_.SetAction(PLBSchedulerActionType::LoadBalancing);
                    }
                    else
                    {
                        // After we do LoadBalancing for PLBActionRetryTimes times, switch to fast after MinBalancingIntervalExpires.
                        balancingAction_.SetAction(PLBSchedulerActionType::QuickLoadBalancing);
                    }
                }
                // Select balancing action for this domain. No tracing here, since PLBDomainStart will trace the action.
                return;
            }
            break;
        }
    }

    // If there is no action selected after checking all states, exit with no action needed
    // No tracing is done here, RunSearcher() will trace PLBDomainSkip since we are returing NoActionNeeded.
    currentAction_.End();
    //Diagnostics
    {
        stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::Skip;
        stageDiagnosticsSPtr->decisionScheduled_ = false;
    }
}

Common::TimeSpan PLBScheduler::GetNextRefreshInterval(Common::StopwatchTime timestamp)
{
    PLBConfig & config = PLBConfig::GetConfig();

    // Set the timers for placement and constraint check, based on their values or based on refresh interval.
    Common::TimeSpan minConstraintCheckInterval = GetMinConstraintCheckInterval();
    Common::TimeSpan minPlacementInterval = GetMinPlacementInterval();

    Common::StopwatchTime nextStageTime = lastPlacement_ + minPlacementInterval;
    nextStageTime = min(nextStageTime, lastConstraintCheck_ + minConstraintCheckInterval);
    nextStageTime = min(nextStageTime, lastBalancing_ + config.MinLoadBalancingInterval);

    Common::TimeSpan interval = nextStageTime < timestamp ? Common::TimeSpan::Zero : nextStageTime - timestamp;

    return interval;
}

void PLBScheduler::OnMovementGenerated(StopwatchTime timeStamp, double newAvgStdDev, map<Guid, FailoverUnitMovement> const& movementList)
{
    if (currentAction_.Action == PLBSchedulerActionType::NoActionNeeded || currentAction_.IsSkip)
    {
        return;
    }

    bool isBalancing = currentAction_.IsBalancing();

    bool existDefragMetric = false;
    for (auto defragMetric : PLBConfig::GetConfig().DefragmentationMetrics)
    {
        existDefragMetric |= defragMetric.second;
    }
    for (auto placementStrategy : PLBConfig::GetConfig().PlacementStrategy)
    {
        existDefragMetric |= (placementStrategy.second != Metric::PlacementStrategy::Balancing);
    }

    if (!existDefragMetric)
    {
        ASSERT_IF(isBalancing && newAvgStdDev < 0.0, "Executed action is Balancing but no or bad newAvgStdDev provided.");
    }

    lastActionTime_ = timeStamp;

    if (isBalancing)
    {
        hasLastBalancingAvgStdDev_ = true;
        lastBalancingAvgStdDev_ = newAvgStdDev;
        UpdateMovements(timeStamp, movementList);
    }
}

void PLBScheduler::Merge(StopwatchTime timeStamp, PLBScheduler && other)
{
    lastNodeDown_ = max(lastNodeDown_, other.lastNodeDown_);
    lastNewNode_ = max(lastNewNode_, other.lastNewNode_);
    lastLoadChange_ = max(lastLoadChange_, other.lastLoadChange_);

    lastActionTime_ = max(lastActionTime_, other.lastActionTime_);
    lastBalancing_ = max(lastBalancing_, other.lastBalancing_);

    // if two schedulers are getting merged, we assume the system has changed so we update lastStateChange_ to the current timestamp
    lastStateChange_ = timeStamp;
    currentAction_.Reset();
    placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
    balancingAction_.SetAction(PLBSchedulerActionType::QuickLoadBalancing);
    hasLastBalancingAvgStdDev_ = false;
    lastBalancingAvgStdDev_ = 0.0;
    movementCounterPerFU_.insert(other.movementCounterPerFU_.begin(), other.movementCounterPerFU_.end());
}

void PLBScheduler::SetConstraintCheckEnabled(bool constraintCheckEnabled, StopwatchTime timeStamp)
{
    if (constraintCheckEnabled_ != constraintCheckEnabled)
    {
        trace_.Scheduler(serviceDomainId_, wformatString("ConstraintCheck enabled for service domain {0}: {1}", serviceDomainId_, constraintCheckEnabled));
        constraintCheckEnabled_ = constraintCheckEnabled;
        SystemStateChanged(timeStamp);
    }
}

void PLBScheduler::SetBalancingEnabled(bool balancingEnabled, StopwatchTime timeStamp)
{
    if (balancingEnabled_ != balancingEnabled)
    {
        trace_.Scheduler(serviceDomainId_, wformatString("Balancing enabled for service domain {0}: {1}", serviceDomainId_, balancingEnabled));
        balancingEnabled_ = balancingEnabled;
        SystemStateChanged(timeStamp);
    }
}

void PLBScheduler::RefreshTimedCounters(StopwatchTime timeStamp)
{
    for (auto it = movementCounterPerFU_.begin(); it != movementCounterPerFU_.end(); )
    {
        it->second.Refresh(timeStamp);
        if (0 == it->second.GetCount())
        {
            it = movementCounterPerFU_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

set<Guid> PLBScheduler::GetMovementThrottledFailoverUnits() const
{
    set<Guid> throttled;
    uint threshold = PLBConfig::GetConfig().MovementPerPartitionThrottleThreshold;
    for (auto it = movementCounterPerFU_.begin(); it != movementCounterPerFU_.end(); it++)
    {
        if (it->second.GetCount() >= threshold)
        {
            throttled.insert(it->first);
        }
    }
    return throttled;
}

bool PLBScheduler::IsSystemChangedSince(StopwatchTime timeStamp) const
{
    return (lastStateChange_ > timeStamp || lastLoadChange_ > timeStamp);
}

// private members

bool  PLBScheduler::IsConstraintCheckThrottled(StopwatchTime timeStamp)
{
    PLBConfig const & config = PLBConfig::GetConfig();

    if (timeStamp < lastNodeDown_ + config.ConstraintFixPartialDelayAfterNodeDown)
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"ConstraintCheckPartial", L"it has not been long enough since the last node down event");
        return true;
    }
    else if (timeStamp < lastNewNode_ + config.ConstraintFixPartialDelayAfterNewNode)
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"ConstraintCheckPartial", L"it has not been long enough since the last new node event");
        return true;
    }

    return false;
}

bool PLBScheduler::IsBalancingThrottled(StopwatchTime timeStamp, ISystemState const & systemState)
{
    PLBConfig const & config = PLBConfig::GetConfig();

    if (timeStamp < lastNodeDown_ + config.BalancingDelayAfterNodeDown)
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", L"it has not been long enough since the last node down event");
        return true;
    }
    else if (timeStamp < lastNewNode_ + config.BalancingDelayAfterNewNode)
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", L"it has not been long enough since the last new node event");
        return true;
    }
    else if (ThrottleGlobalMovements(systemState, L"Balancing") || ThrottleGlobalBalancingMovements(systemState) || (config.MaxPercentageToMove == 0.0))
    {
        return true;
    }
    else if (ThrottleBalancingForPerFUMovements(systemState))
    {
        return true;
    }

    return false;
}

bool PLBScheduler::IsCreationWithMoveThrottled(ISystemState const & systemState)
{
    if (ThrottleGlobalMovements(systemState, L"NewReplicaPlacementWithMove") || ThrottleGlobalPlacementMovements(systemState))
    {
        return true;
    }
    else if (PLBConfig::GetConfig().MaxPercentageToMoveForPlacement == 0.0)
    {
        return true;
    }

    return false;
}

void PLBScheduler::SystemStateChanged(StopwatchTime timeStamp)
{
    placementAction_.SetAction(PLBSchedulerActionType::NewReplicaPlacement);
    lastStateChange_ = timeStamp;
}

Common::TimeSpan PLBScheduler::GetMinConstraintCheckInterval()
{
    // Calculate correct value for constraint check interval:
    // If user has specified PLBRefreshInterval then use that value, otherwise use MinConstraintCheckInterval
    // NOTE: This will exist until we deprecate PLBRefreshInterval, and user cannot specify both values (manifest check will fail).
    PLBConfig & config = PLBConfig::GetConfig();
    Common::TimeSpan minConstraintCheckInterval = config.MinConstraintCheckInterval;
    if (config.MinConstraintCheckIntervalEntry.DefaultValue == minConstraintCheckInterval &&
        config.PLBRefreshIntervalEntry.DefaultValue != config.PLBRefreshInterval)
    {
        // If user has changed refresh interval, but not MinConstraintCheckInterval, use refresh instead
        minConstraintCheckInterval = config.PLBRefreshInterval;
    }
    return minConstraintCheckInterval;
}

Common::TimeSpan PLBScheduler::GetMinPlacementInterval()
{
    // Calculate correct value for placement interval:
    // If user has specified PLBRefreshInterval then use that value, otherwise use MinPlacementInterval
    // NOTE: This will exist until we deprecate PLBRefreshInterval, and user cannot specify both values (manifest check will fail).
    PLBConfig & config = PLBConfig::GetConfig();
    Common::TimeSpan minPlacementInterval = config.MinPlacementInterval;
    if (config.MinPlacementIntervalEntry.DefaultValue == minPlacementInterval &&
        config.PLBRefreshIntervalEntry.DefaultValue != config.PLBRefreshInterval)
    {
        // If user has changed refresh interval, but not MinPlacementInterval, use refresh instead
        minPlacementInterval = config.PLBRefreshInterval;
    }
    return minPlacementInterval;
}

bool PLBScheduler::ThrottleBalancingForSmallStdDevChange(ISystemState const & systemState) const
{
    if (lastStateChange_ > lastBalancing_)
    {
        // don't throttle balancing if there is any state change since the last balancing
        return false;
    }

    if (lastStateChange_ < lastBalancing_ && lastLoadChange_ < lastBalancing_)
    {
        // there is no system state change since the last balancing thus there is no need for balancing.
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", L"there is no system change since last balancing");
        return true;
    }

    double avgStdDevDeltaThrottleThreshold = PLBConfig::GetConfig().AvgStdDevDeltaThrottleThreshold;
    if (avgStdDevDeltaThrottleThreshold >= 0.0 && hasLastBalancingAvgStdDev_ && systemState.GetAvgStdDev() <= lastBalancingAvgStdDev_ * (1 + avgStdDevDeltaThrottleThreshold))
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", wformatString("the current AvgStdDev ({0}) is not much worse than the AvgStdDev from the last balancing ({1})", systemState.GetAvgStdDev(), lastBalancingAvgStdDev_));
        return true;
    }

    return false;
}

bool PLBScheduler::AreMovementsThrottled(ISystemState const & systemState,
    size_t currentMovementCount,
    double allowedPercentageToMove,
    uint maxReplicasToMove)
{
    size_t maxMovements = SIZE_T_MAX;
    // First check if we have a throttle on absolute number of movements.
    if (maxReplicasToMove != 0)
    {
        maxMovements = maxReplicasToMove;
    }
    // Then check if we have a throttle on percentage of replicas in the cluster.
    if (allowedPercentageToMove > 0.0)
    {
        size_t maxMovementsByPercentage = static_cast<size_t> (ceil(allowedPercentageToMove * systemState.ExistingReplicaCount()));
        // If we have both throttles, pick the one that is more conservative.
        if (maxMovementsByPercentage < maxMovements)
        {
            maxMovements = maxMovementsByPercentage;
        }
    }
    // If we have already moved more than threshold, then throttle.
    return currentMovementCount >= maxMovements;
}

bool PLBScheduler::ThrottleGlobalBalancingMovements(ISystemState const & systemState)
{
    // global throttle
    PLBConfig const& config = PLBConfig::GetConfig();
    size_t balancingMovements = systemState.BalancingMovementCount();
    uint balancingThreshold = config.GlobalMovementThrottleThresholdForBalancing;
    if (AreMovementsThrottled(systemState, balancingMovements, config.GlobalMovementThrottleThresholdPercentageForBalancing, balancingThreshold))
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing",
            wformatString("The number of movements for Balancing ({0}) in the past {1}) has reached or exceeded the threshold (absolute: {2} percentage: {3})",
                balancingMovements,
                config.GlobalMovementThrottleCountingInterval,
                balancingThreshold,
                static_cast<size_t>(ceil(systemState.ExistingReplicaCount() * config.GlobalMovementThrottleThresholdPercentageForBalancing))));
        return true;
    }
    return false;
}

bool PLBScheduler::ThrottleGlobalPlacementMovements(ISystemState const & systemState)
{
    // global throttle
    PLBConfig const& config = PLBConfig::GetConfig();
    size_t placementMovements = systemState.PlacementMovementCount();
    uint placementThreshold = config.GlobalMovementThrottleThresholdForPlacement;
    if (AreMovementsThrottled(systemState, placementMovements, config.GlobalMovementThrottleThresholdPercentageForPlacement, placementThreshold))
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Placement",
            wformatString("The number of movements for Placement ({0}) in the past {1}) has reached or exceeded the threshold (absolute: {2} percentage: {3})",
                placementMovements,
                config.GlobalMovementThrottleCountingInterval,
                placementThreshold,
                static_cast<size_t>(ceil(systemState.ExistingReplicaCount() * config.GlobalMovementThrottleThresholdPercentageForPlacement))));
        return true;
    }
    return false;
}

bool PLBScheduler::ThrottleGlobalMovements(ISystemState const & systemState, std::wstring const& action)
{
    // global throttle
    PLBConfig const& config = PLBConfig::GetConfig();
    size_t totalMovements = systemState.BalancingMovementCount() +systemState.PlacementMovementCount();
    uint threshold = config.GlobalMovementThrottleThreshold;
    if (AreMovementsThrottled(systemState, totalMovements, config.GlobalMovementThrottleThresholdPercentage, threshold))
    {
        trace_.SchedulerActionSkip(serviceDomainId_,
            action,
            wformatString("the total number of failover unit movements ({0}) in the past {1}) has reached or exceeded the threshold (absolute: {2} percentage: {3})",
                totalMovements,
                config.GlobalMovementThrottleCountingInterval,
                threshold,
                static_cast<size_t>(ceil(systemState.ExistingReplicaCount() * config.GlobalMovementThrottleThresholdPercentage))));
        return true;
    }
    return false;
}

bool PLBScheduler::ThrottleBalancingForPerFUMovements(ISystemState const & systemState)
{
    PLBConfig const& config = PLBConfig::GetConfig();

    // per-FailoverUnit throttle
    auto throttledFailoverUnits = GetMovementThrottledFailoverUnits();
    auto imbalancedFailoverUnits = systemState.GetImbalancedFailoverUnits();
    for (auto it = throttledFailoverUnits.begin(); it != throttledFailoverUnits.end(); )
    {
        if (imbalancedFailoverUnits.find(*it) == imbalancedFailoverUnits.end())
        {
            it = throttledFailoverUnits.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (throttledFailoverUnits.size() > imbalancedFailoverUnits.size() * config.MovementThrottledPartitionsPercentageThreshold)
    {
        trace_.SchedulerActionSkip(serviceDomainId_, L"Balancing", wformatString("the number of movement throttled failover units ({0}) has exceeded {1}% of the number of imbalanced failover units ({2})",
                                        throttledFailoverUnits.size(),
                                        config.MovementThrottledPartitionsPercentageThreshold * 100,
                                        imbalancedFailoverUnits.size()));
        return true;
    }

    return false;
}

void PLBScheduler::UpdateMovements(StopwatchTime timeStamp, FailoverUnitMovementTable const& movementList)
{
    PLBConfig const& config = PLBConfig::GetConfig();
    TimeSpan interval = config.MovementPerPartitionThrottleCountingInterval;

    for (auto it = movementList.begin(); it != movementList.end(); ++it)
    {
        auto counterIt = movementCounterPerFU_.find(it->first);
        if (counterIt == movementCounterPerFU_.end())
        {
            counterIt = movementCounterPerFU_.insert(make_pair(it->first, TimedCounter(interval, FUMovementCounterWindowCount))).first;
        }
        counterIt->second.Record(1, timeStamp);
    }
}
