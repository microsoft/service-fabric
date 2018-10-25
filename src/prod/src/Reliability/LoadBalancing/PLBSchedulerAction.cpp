// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBSchedulerAction.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PLBSchedulerAction::PLBSchedulerAction()
    : action_(PLBSchedulerActionType::None), iteration_(0), isSkip_(false), isConstraintCheckLight_(false)
{
}

PLBSchedulerAction::PLBSchedulerAction(const PLBSchedulerAction & other)
    : action_(other.action_), iteration_(other.iteration_), isSkip_(other.isSkip_), isConstraintCheckLight_(other.isConstraintCheckLight_)
{
}

void PLBSchedulerAction::SetAction(PLBSchedulerActionType::Enum value)
{
    action_ = value;
    iteration_ = 0;
    isSkip_ = false;
    isConstraintCheckLight_ = false;
}

void PLBSchedulerAction::SetConstraintCheckLightness(bool lightness)
{
    isConstraintCheckLight_ = lightness;
}

void PLBSchedulerAction::Reset()
{
    action_ = PLBSchedulerActionType::None;
    iteration_ = 0;
    isSkip_ = false;
    isConstraintCheckLight_ = false;
}

void PLBSchedulerAction::End()
{
    action_ = PLBSchedulerActionType::NoActionNeeded;
    iteration_ = 0;
    isSkip_ = false;
    isConstraintCheckLight_ = false;
}

void PLBSchedulerAction::IncreaseIteration()
{
    iteration_++;
}

bool PLBSchedulerAction::CanRetry() const
{
    if (action_ == PLBSchedulerActionType::NewReplicaPlacementWithMove)
    {
        return iteration_ == 0;
    }
    else
    {
        return iteration_ < PLBConfig::GetConfig().PLBActionRetryTimes;
    }
}

void PLBSchedulerAction::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    switch (action_)
    {
    case PLBSchedulerActionType::None:
    case PLBSchedulerActionType::NoActionNeeded:
        w.Write("NoActionNeeded");
        break;
    case PLBSchedulerActionType::NewReplicaPlacement:
        w.Write("NewReplicaPlacement");
        break;
    case PLBSchedulerActionType::NewReplicaPlacementWithMove:
        w.Write("NewReplicaPlacementWithMove");
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        w.Write("ConstraintCheck");
        break;
    case PLBSchedulerActionType::QuickLoadBalancing:
        w.Write("QuickLoadBalancing");
        break;
    case PLBSchedulerActionType::LoadBalancing:
        w.Write("LoadBalancing");
        break;
    default:
        w.Write("Unknown");
        return;
    }

    w.Write("/{0}/{1}", iteration_, isSkip_);
}

void PLBSchedulerAction::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

std::wstring PLBSchedulerAction::ToQueryString() const
{
    if(isSkip_)
    {
        return L"ActionSkipped";
    }
    else
    {
        switch (action_)
        {
        case PLBSchedulerActionType::None:
        case PLBSchedulerActionType::NoActionNeeded:
            return L"NoActionNeeded";
        case PLBSchedulerActionType::NewReplicaPlacement:
            return L"NewReplicaPlacement";
        case PLBSchedulerActionType::NewReplicaPlacementWithMove:
            return L"NewReplicaPlacementWithMove";
        case PLBSchedulerActionType::ConstraintCheck:
            return L"ConstraintCheck";
        case PLBSchedulerActionType::QuickLoadBalancing:
        case PLBSchedulerActionType::LoadBalancing:
            return L"LoadBalancing";
        default:
            return L"UnknownAction";
        }
    }
}
