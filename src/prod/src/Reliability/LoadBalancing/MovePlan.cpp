// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBConfig.h"
#include "Service.h"
#include "FailoverUnit.h"
#include "MovePlan.h"
#include "FailoverUnitMovement.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

MovePlan::MovePlan()
    : pendingMovements_(),
    pendingMovementsTime_(StopwatchTime::Zero),
    ongoingMovements_(),
    ongoingMovementsTime_(StopwatchTime::Zero),
    inBuildThrottledServiceNames_(),
    inBuildReplicaCount_(0),
    swapPrimaryThrottledServiceNames_(),
    swapPrimaryReplicaCount_(0)
{
}

MovePlan::MovePlan(MovePlan && other)
    : pendingMovements_(move(other.pendingMovements_)),
    pendingMovementsTime_(other.pendingMovementsTime_),
    ongoingMovements_(move(other.ongoingMovements_)),
    ongoingMovementsTime_(other.ongoingMovementsTime_),
    inBuildThrottledServiceNames_(move(other.inBuildThrottledServiceNames_)),
    inBuildReplicaCount_(other.inBuildReplicaCount_),
    swapPrimaryThrottledServiceNames_(move(other.swapPrimaryThrottledServiceNames_)),
    swapPrimaryReplicaCount_(other.swapPrimaryReplicaCount_)
{
}

void MovePlan::Merge(MovePlan && other)
{
    pendingMovements_.clear();
    pendingMovementsTime_ = StopwatchTime::Zero;
    ongoingMovements_.insert(other.ongoingMovements_.begin(), other.ongoingMovements_.end());
    ongoingMovementsTime_ = max(ongoingMovementsTime_, other.ongoingMovementsTime_);

    inBuildThrottledServiceNames_.insert(other.inBuildThrottledServiceNames_.begin(), other.inBuildThrottledServiceNames_.end());
    inBuildReplicaCount_ += other.inBuildReplicaCount_;
    swapPrimaryThrottledServiceNames_.insert(other.swapPrimaryThrottledServiceNames_.begin(), other.swapPrimaryThrottledServiceNames_.end());
    swapPrimaryReplicaCount_ += other.swapPrimaryReplicaCount_;
}

void MovePlan::OnNodeUp()
{
    ClearPendingMovements();
}

void MovePlan::OnNodeDown()
{
    ClearPendingMovements();
}

void MovePlan::OnNodeChanged()
{
    ClearPendingMovements();
}

void MovePlan::OnServiceTypeChanged()
{
    ClearPendingMovements();
}

void MovePlan::OnServiceChanged(Service const& service)
{
    PLBConfig const& config = PLBConfig::GetConfig();
    if (config.InBuildThrottlingAssociatedMetric.empty() || service.ContainsMetric(config.InBuildThrottlingAssociatedMetric))
    {
        inBuildThrottledServiceNames_.insert(service.ServiceDesc.Name);
    }
    else
    {
        inBuildThrottledServiceNames_.erase(service.ServiceDesc.Name);
    }

    if (config.SwapPrimaryThrottlingAssociatedMetric.empty() || service.ContainsMetric(config.SwapPrimaryThrottlingAssociatedMetric))
    {
        swapPrimaryThrottledServiceNames_.insert(service.ServiceDesc.Name);
    }
    else
    {
        swapPrimaryThrottledServiceNames_.erase(service.ServiceDesc.Name);
    }
}

void MovePlan::OnServiceDeleted(wstring const& serviceName)
{
    inBuildThrottledServiceNames_.erase(serviceName);
    swapPrimaryThrottledServiceNames_.erase(serviceName);
}

void MovePlan::OnFailoverUnitAdded(FailoverUnit const& failoverUnit)
{
    if (inBuildThrottledServiceNames_.find(failoverUnit.FuDescription.ServiceName) != inBuildThrottledServiceNames_.end())
    {
        inBuildReplicaCount_ += static_cast<int64>(failoverUnit.FuDescription.InBuildReplicaCount);
    }

    if (swapPrimaryThrottledServiceNames_.find(failoverUnit.FuDescription.ServiceName) != swapPrimaryThrottledServiceNames_.end())
    {
        swapPrimaryReplicaCount_ += static_cast<int64>(failoverUnit.FuDescription.SwapPrimaryReplicaCount);
    }

    if (!pendingMovements_.empty())
    {
        pendingMovements_.erase(failoverUnit.FUId);
    }

    if (!ongoingMovements_.empty())
    {
        ongoingMovements_.erase(failoverUnit.FUId);
    }
}

void MovePlan::OnFailoverUnitChanged(FailoverUnit const& oldFU, FailoverUnitDescription const& newFU)
{
    ASSERT_IFNOT(oldFU.FUId == newFU.FUId, "Failover unit ids don't match");

    if (inBuildThrottledServiceNames_.find(oldFU.FuDescription.ServiceName) != inBuildThrottledServiceNames_.end())
    {
        inBuildReplicaCount_ += static_cast<int64>(newFU.InBuildReplicaCount);
        inBuildReplicaCount_ -= static_cast<int64>(oldFU.FuDescription.InBuildReplicaCount);
        ASSERT_IF(inBuildReplicaCount_ < 0, "inBuildReplicaCount_ error");
    }
    
    if (swapPrimaryThrottledServiceNames_.find(oldFU.FuDescription.ServiceName) != swapPrimaryThrottledServiceNames_.end())
    {
        swapPrimaryReplicaCount_ += static_cast<int64>(newFU.SwapPrimaryReplicaCount);
        swapPrimaryReplicaCount_ -= static_cast<int64>(oldFU.FuDescription.SwapPrimaryReplicaCount);
        ASSERT_IF(swapPrimaryReplicaCount_ < 0, "swapPrimaryReplicaCount_ error");
    }

    if (!pendingMovements_.empty())
    {
        pendingMovements_.erase(oldFU.FUId);
    }

    if (!ongoingMovements_.empty())
    {
        ongoingMovements_.erase(oldFU.FUId);
    }
}

void MovePlan::OnFailoverUnitDeleted(FailoverUnit const& failoverUnit)
{
    if (inBuildThrottledServiceNames_.find(failoverUnit.FuDescription.ServiceName) != inBuildThrottledServiceNames_.end())
    {
        inBuildReplicaCount_ -= static_cast<int64>(failoverUnit.FuDescription.InBuildReplicaCount);
        ASSERT_IF(inBuildReplicaCount_ < 0, "inBuildReplicaCount_ error");
    }
    
    if (swapPrimaryThrottledServiceNames_.find(failoverUnit.FuDescription.ServiceName) != swapPrimaryThrottledServiceNames_.end())
    {
        swapPrimaryReplicaCount_ -= static_cast<int64>(failoverUnit.FuDescription.SwapPrimaryReplicaCount);
        ASSERT_IF(swapPrimaryReplicaCount_ < 0, "swapPrimaryReplicaCount_ error");
    }

    if (!pendingMovements_.empty())
    {
        pendingMovements_.erase(failoverUnit.FUId);
    }

    if (!ongoingMovements_.empty())
    {
        ongoingMovements_.erase(failoverUnit.FUId);
    }
}

void MovePlan::OnLoadChanged()
{
    ClearPendingMovements();
}

void MovePlan::OnConstraintCheckDisabled()
{
    ClearPendingMovements();
}

void MovePlan::OnBalancingDisabled()
{
    ClearPendingMovements();
}

void MovePlan::OnMovementGenerated(FailoverUnitMovementTable && movementList, StopwatchTime timestamp)
{
    ASSERT_IFNOT(IsEmpty, "The move plan should be empty when adding movements");

    pendingMovements_ = move(movementList);
    pendingMovementsTime_ = timestamp;
}

void MovePlan::Refresh(StopwatchTime now)
{
    if (!pendingMovements_.empty() && now >= pendingMovementsTime_ + PLBConfig::GetConfig().MaxMovementHoldingTime)
    {
        pendingMovements_.clear();
        pendingMovementsTime_ = StopwatchTime::Zero;
    }

    if (!ongoingMovements_.empty() && now >= ongoingMovementsTime_ + PLBConfig::GetConfig().MaxMovementExecutionTime)
    {
        ongoingMovements_.clear();
        ongoingMovementsTime_ = StopwatchTime::Zero;
    }
}

FailoverUnitMovementTable MovePlan::GetMovements(StopwatchTime timestamp)
{
    FailoverUnitMovementTable movementTable;

    if (!ongoingMovements_.empty())
    {
        return movementTable;
    }

    PLBConfig const& config = PLBConfig::GetConfig();

    int64 inBuildRemaining = -1;
    int64 inBuildMax = static_cast<int64>(config.InBuildThrottlingGlobalMaxValue);
    if (!inBuildThrottledServiceNames_.empty() && config.InBuildThrottlingEnabled && inBuildMax > 0)
    {
        inBuildRemaining = inBuildMax <= inBuildReplicaCount_ ? 0 : inBuildMax - inBuildReplicaCount_;
    }

    int64 swapPrimaryRemaining = -1;
    int64 swapPrimaryMax = static_cast<int64>(config.SwapPrimaryThrottlingGlobalMaxValue);
    if (!swapPrimaryThrottledServiceNames_.empty() && config.SwapPrimaryThrottlingEnabled && swapPrimaryMax > 0)
    {
        swapPrimaryRemaining = swapPrimaryMax <= swapPrimaryReplicaCount_ ? 0 : swapPrimaryMax - swapPrimaryReplicaCount_;
    }

    for (auto itMovement = pendingMovements_.begin(); itMovement != pendingMovements_.end(); )
    {
        bool inBuildThrottled = inBuildRemaining >= 0 && inBuildThrottledServiceNames_.find(itMovement->second.ServiceName) != inBuildThrottledServiceNames_.end();
        bool swapPrimaryThrottled = swapPrimaryRemaining >= 0 && swapPrimaryThrottledServiceNames_.find(itMovement->second.ServiceName) != swapPrimaryThrottledServiceNames_.end();

        if (!inBuildThrottled && !swapPrimaryThrottled)
        {
            movementTable.insert(make_pair(itMovement->first, move(itMovement->second)));
            itMovement = pendingMovements_.erase(itMovement);
            continue;
        }

        vector<FailoverUnitMovement::PLBAction> const& actionList = itMovement->second.Actions;

        if (swapPrimaryThrottled)
        {
            auto itPrimaryMovement = find_if(actionList.begin(), actionList.end(), [](FailoverUnitMovement::PLBAction const& action)
            {
                return action.Action == FailoverUnitMovementType::MovePrimary || action.Action == FailoverUnitMovementType::SwapPrimarySecondary;
            });

            if (itPrimaryMovement != actionList.end())
            {
                if (swapPrimaryRemaining == 0)
                {
                    // skip all movements if a swap is throttled
                    ++itMovement;
                    continue;
                }
                else
                {
                    --swapPrimaryRemaining;
                }
            }
        }

        vector<FailoverUnitMovement::PLBAction> remainingActions;
        vector<FailoverUnitMovement::PLBAction> selectedActions;

        for (auto itAction = actionList.begin(); itAction != actionList.end(); ++itAction)
        {
            if (itAction->Action == FailoverUnitMovementType::AddPrimary)
            {
                if (inBuildThrottled && swapPrimaryThrottled)
                {
                    if (inBuildRemaining == 0 || swapPrimaryRemaining == 0)
                    {
                        remainingActions.push_back(*itAction);
                    }
                    else // both > 0
                    {
                        selectedActions.push_back(*itAction);
                        --inBuildRemaining;
                        --swapPrimaryRemaining;
                    }
                }
                else if (inBuildThrottled)
                {
                    if (inBuildRemaining == 0)
                    {
                        remainingActions.push_back(*itAction);
                    }
                    else // > 0
                    {
                        selectedActions.push_back(*itAction);
                        --inBuildRemaining;
                    }
                }
                else if (swapPrimaryThrottled)
                {
                    if (swapPrimaryRemaining == 0)
                    {
                        remainingActions.push_back(*itAction);
                    }
                    else // > 0
                    {
                        selectedActions.push_back(*itAction);
                        --swapPrimaryRemaining;
                    }
                }
                else
                {
                    Assert::CodingError("One of inBuildThrottled and swapPrimaryThrottled should be true");
                }
            }
            else if (inBuildThrottled  &&
                (itAction->Action == FailoverUnitMovementType::AddSecondary ||
                    itAction->Action == FailoverUnitMovementType::AddInstance))
            {
                if (inBuildRemaining == 0)
                {
                    remainingActions.push_back(*itAction);
                }
                else // > 0
                {
                    selectedActions.push_back(*itAction);
                    --inBuildRemaining;
                }
            }
            else
            {
                selectedActions.push_back(*itAction);
            }
        }

        if (!selectedActions.empty())
        {
            movementTable.insert(make_pair(itMovement->first, FailoverUnitMovement(
                itMovement->second.FailoverUnitId,
                wstring(itMovement->second.ServiceName),
                itMovement->second.IsStateful,
                itMovement->second.Version,
                itMovement->second.IsFuInTransition,
                move(selectedActions))));
        }

        if (!remainingActions.empty())
        {
            itMovement->second.UpdateActions(move(remainingActions));
            ++itMovement;
        }
        else
        {
            itMovement = pendingMovements_.erase(itMovement);
        }
    }

    if (pendingMovements_.empty())
    {
        pendingMovementsTime_ = StopwatchTime::Zero;
    }

    // put all movements to ongoing movement list

    if (!movementTable.empty())
    {
        for (auto it = movementTable.begin(); it != movementTable.end(); ++it)
        {
            ongoingMovements_.insert(it->first);
        }

        ongoingMovementsTime_ = timestamp;
    }

    return movementTable;
}

void MovePlan::ClearOngoingMovements()
{
    if (!ongoingMovements_.empty())
    {
        ongoingMovements_.clear();
        ongoingMovementsTime_ = StopwatchTime::Zero;
    }
}

void MovePlan::ClearPendingMovements()
{
    if (!pendingMovements_.empty())
    {
        pendingMovements_.clear();
        pendingMovementsTime_ = StopwatchTime::Zero;
    }
}

