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
        return L"None";
    case PLBSchedulerActionType::Creation:
        return L"Creation";
    case PLBSchedulerActionType::CreationWithMove:
        return L"CreationWithMove";
    case PLBSchedulerActionType::ConstraintCheck:
        return L"ConstraintCheck";
    case PLBSchedulerActionType::FastBalancing:
        return L"FastBalancing";
    case PLBSchedulerActionType::SlowBalancing:
        return L"SlowBalancing";
    case PLBSchedulerActionType::NoActionNeeded:
        return L"NoActionNeeded";
    case PLBSchedulerActionType::Upgrade:
        return L"Upgrade";
    case PLBSchedulerActionType::ClientPromoteToPrimaryApiCall:
        return L"ClientPromoteToPrimaryApiCall";
    case PLBSchedulerActionType::ClientMovePrimaryApiCall:
        return L"ClientMovePrimaryApiCall";
    case PLBSchedulerActionType::ClientMoveSecondaryApiCall:
        return L"ClientMoveSecondaryApiCall";
    default:
        Common::Assert::CodingError("Unknown PLB Scheduler action {0}", static_cast<int>(schedulerActionType));
    }
}
