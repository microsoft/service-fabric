// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class AutoScalingTask : public DynamicStateMachineTask
        {
            DENY_COPY(AutoScalingTask);

        public:

			AutoScalingTask(int targetReplicaSetSize);

            void CheckFailoverUnit(LockedFailoverUnitPtr & failoverUnit, std::vector<StateMachineActionUPtr> & actions);

        private:
            int targetReplicaSetSize_;
        };
    }
}
