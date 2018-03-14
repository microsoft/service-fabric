// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FailoverUnitMovement.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class DecisionToken;
        
        class IFailoverManager
        {
        public:
            virtual ~IFailoverManager() = 0 {}
            virtual void ProcessFailoverUnitMovements(FailoverUnitMovementTable && fuMovements, DecisionToken && dToken) = 0;
            virtual void UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const&) = 0;
        };
    }
}
