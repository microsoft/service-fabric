// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class StatelessCheckTask : public StateMachineTask
        {
            DENY_COPY(StatelessCheckTask);

        public:
            StatelessCheckTask(FailoverManager const& fm, INodeCache const&);

            void CheckFailoverUnit(LockedFailoverUnitPtr& lockedFailoverUnit, std::vector<StateMachineActionUPtr>& actions);

        private:
            FailoverManager const& fm_;
			INodeCache const& nodeCache_;
        };
    }
}
