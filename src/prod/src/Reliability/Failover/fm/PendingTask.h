// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class PendingTask : public StateMachineTask
        {
            DENY_COPY(PendingTask);

        public:
            PendingTask(FailoverManager & fm);

            void CheckFailoverUnit(LockedFailoverUnitPtr& lockedFailoverUnit, std::vector<StateMachineActionUPtr>& actions);

        private:
            void ProcessExtraStandByReplicas(LockedFailoverUnitPtr & failoverUnit);

            FailoverManager & fm_;
        };
    }
}
