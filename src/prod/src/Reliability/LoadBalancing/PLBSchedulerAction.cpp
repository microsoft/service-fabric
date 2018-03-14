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

void PLBSchedulerAction::Increase()
{
    int retryTimes = PLBConfig::GetConfig().PLBActionRetryTimes;

    switch (action_)
    {
    case PLBSchedulerActionType::None:
        action_ = PLBSchedulerActionType::Creation;
        break;
    case PLBSchedulerActionType::Creation:
        ++iteration_;
        if (iteration_ >= retryTimes)
        {
            if (PLBConfig::GetConfig().MoveExistingReplicaForPlacement)
            {
                action_ = PLBSchedulerActionType::CreationWithMove;
            }
            else
            {
                action_ = PLBSchedulerActionType::ConstraintCheck;
            }
            iteration_ = 0;
        }
        break;
    case PLBSchedulerActionType::CreationWithMove:
        ++iteration_;
        if (iteration_ > 0)
        {
            action_ = PLBSchedulerActionType::ConstraintCheck;
            iteration_ = 0;
        }
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        ++iteration_;
        if (iteration_ >= retryTimes)
        {
            action_ = PLBSchedulerActionType::FastBalancing;
            iteration_ = 0;
        }
        break;
    case PLBSchedulerActionType::FastBalancing:
        ++iteration_;
        if (iteration_ >= retryTimes)
        {
            action_ = PLBSchedulerActionType::SlowBalancing;
            iteration_ = 0;
        }
        break;
    case PLBSchedulerActionType::SlowBalancing:
        ++iteration_;
        if (iteration_ >= retryTimes)
        {
            action_ = PLBSchedulerActionType::NoActionNeeded;
            iteration_ = 0;
        }
        break;
    case PLBSchedulerActionType::NoActionNeeded:
        break;
    default:
        Assert::CodingError("Unknown current action");
    }

    isSkip_ = false;
}

void PLBSchedulerAction::MoveForward()
{
    switch (action_)
    {
    case PLBSchedulerActionType::None:
        action_ = PLBSchedulerActionType::Creation;
        break;
    case PLBSchedulerActionType::Creation:
        action_ = PLBSchedulerActionType::ConstraintCheck;
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        action_ = PLBSchedulerActionType::FastBalancing;
        break;
    case PLBSchedulerActionType::FastBalancing:
        action_ = PLBSchedulerActionType::SlowBalancing;
        break;
    case PLBSchedulerActionType::SlowBalancing:
        action_ = PLBSchedulerActionType::NoActionNeeded;
        break;
    case PLBSchedulerActionType::NoActionNeeded:
        break;
    default:
        Assert::CodingError("Unknown current action");
    }

    iteration_ = 0;
    isSkip_ = false;
}

void PLBSchedulerAction::IncreaseIteration()
{
    iteration_++;
}

bool PLBSchedulerAction::CanRetry() const
{
    if (action_ == PLBSchedulerActionType::CreationWithMove)
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
        w.Write("None");
        break;
    case PLBSchedulerActionType::Creation:
        w.Write("Creation");
        break;
    case PLBSchedulerActionType::CreationWithMove:
        w.Write("CreationWithMove");
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        w.Write("ConstraintCheck");
        break;
    case PLBSchedulerActionType::FastBalancing:
        w.Write("FastBalancing");
        break;
    case PLBSchedulerActionType::SlowBalancing:
        w.Write("SlowBalancing");
        break;
    case PLBSchedulerActionType::NoActionNeeded:
        w.Write("NoActionNeeded");
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
            return L"None";
        case PLBSchedulerActionType::Creation:
            return L"Creation";
        case PLBSchedulerActionType::CreationWithMove:
            return L"CreationWithMove";
        case PLBSchedulerActionType::ConstraintCheck:
            return L"ConstraintCheck";
        case PLBSchedulerActionType::FastBalancing:
        case PLBSchedulerActionType::SlowBalancing:
            return L"LoadBalancing";
        case PLBSchedulerActionType::NoActionNeeded:
            return L"NoActionNeeded";
        default:
            return L"UnknownAction";
        }
    }
}
