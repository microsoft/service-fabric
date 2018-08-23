// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PLBSchedulerActionType.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace PLBSchedulerActionType
        {
            void PLBSchedulerActionType::WriteToTextWriter(TextWriter & w, PLBSchedulerActionType::Enum const & val)
            {
                w << ToString(val);
            }

            ENUM_STRUCTURED_TRACE(PLBSchedulerActionType, None, LastValidEnum);
        }
    }
}

wstring PLBSchedulerActionType::ToString(PLBSchedulerActionType::Enum schedulerActionType)
{
    switch (schedulerActionType)
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
        return L"QuickLoadBalancing";
    case PLBSchedulerActionType::LoadBalancing:
        return L"LoadBalancing";
    case PLBSchedulerActionType::Upgrade:
        return L"Upgrade";
    case PLBSchedulerActionType::ClientApiPromoteToPrimary:
        return L"ClientApiPromoteToPrimary";
    case PLBSchedulerActionType::ClientApiMovePrimary:
        return L"ClientApiMovePrimary";
    case PLBSchedulerActionType::ClientApiMoveSecondary:
        return L"ClientApiMoveSecondary";
    default:
        Common::Assert::CodingError("Unknown PLB Scheduler action {0}", static_cast<int>(schedulerActionType));
    }
}
