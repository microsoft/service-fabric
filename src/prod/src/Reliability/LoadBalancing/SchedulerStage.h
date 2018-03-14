// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class SchedulerStage
        {
        public:
            enum Stage {
                Placement = 0,
                ConstraintCheck = 1,
                Balancing = 2,
                Skip = 3,
                NoneExpired = 4,
                ClientAPICall = 5
            };
        };

    }
}
