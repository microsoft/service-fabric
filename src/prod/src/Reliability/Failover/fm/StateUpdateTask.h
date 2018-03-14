// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class StateUpdateTask : public StateMachineTask
        {
            DENY_COPY(StateUpdateTask);

        public:
            StateUpdateTask(FailoverManager const& fm, INodeCache const& nodeCache);

            void CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, std::vector<StateMachineActionUPtr> & actions);

        private:
            bool IsSwapPrimaryNeeded(ApplicationInfoSPtr const& applicationInfo, FailoverUnit const& failoverUnit, Replica const& replica) const;
            void TryPlacePrimary(LockedFailoverUnitPtr & failoverUnit, Replica & replica) const;
            void TryClearUpgradeFlags(ApplicationInfoSPtr const& applicationInfo, LockedFailoverUnitPtr & failoverUnit, Replica & replica) const;

            bool IsUpgradingOrDeactivating(ApplicationInfoSPtr const& applicationInfo, Replica const& replica) const;

            FailoverManager const& fm_;
            INodeCache const& nodeCache_;
        };
    }
}
