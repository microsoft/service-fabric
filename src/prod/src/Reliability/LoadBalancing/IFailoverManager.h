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
            // Processes FT movements that PLB generates.
            virtual void ProcessFailoverUnitMovements(FailoverUnitMovementTable && fuMovements, DecisionToken && dToken) = 0;
            // Updates status of safety checks in case when resource governance is changing.
            virtual void UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const&) = 0;
            // Updates target instance count for a partition that is auto scaling number of instances
            virtual void UpdateFailoverUnitTargetReplicaCount(Common::Guid const &, int targetCount) = 0;
        };
    }
}
