// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class PlacementTask : public StateMachineTask
        {
            DENY_COPY(PlacementTask);

        public:
            PlacementTask(FailoverManager const& fm);

            void CheckFailoverUnit(LockedFailoverUnitPtr& lockedFailoverUnit, std::vector<StateMachineActionUPtr>& actions);

        private:
            FailoverManager const& fm_;
        };
    }
}
