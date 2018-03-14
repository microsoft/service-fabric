// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class StateMachineTask
        {
            DENY_COPY(StateMachineTask);

        public:
            // Check the FailoverUnit and generate appropriate StateMachineActions.
            virtual ~StateMachineTask() {}
            virtual void CheckFailoverUnit(
                LockedFailoverUnitPtr & lockedFailoverUnit,
                std::vector<StateMachineActionUPtr> & actions) = 0;

        protected:
            StateMachineTask();
        };

        class DynamicStateMachineTask : public StateMachineTask
        {
        };
    }
}
