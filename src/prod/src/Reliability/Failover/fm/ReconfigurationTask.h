// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ReconfigurationTask : public StateMachineTask
        {
            DENY_COPY(ReconfigurationTask);

        public:
            ReconfigurationTask(FailoverManager * fm);

            void CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, std::vector<StateMachineActionUPtr> & actions);

            // This function is also used during the rebuild merge to start
            // reconfiguration when the primary is not available.
            void ReconfigPrimary(LockedFailoverUnitPtr & lockedFailoverUnit, FailoverUnit & failoverUnit);

        private:
            void ReconfigSecondary(LockedFailoverUnitPtr & lockedFailoverUnit);
            void ReconfigSwapPrimary(LockedFailoverUnitPtr & lockedFailoverUnit);

            // < 0 better than, == 0 equal, > 0 worse than
            int CompareForPrimary(FailoverUnit const& failoverUnit, Replica const & replica, Replica const & current);

            FailoverManager * fm_;
        };
    }
}
