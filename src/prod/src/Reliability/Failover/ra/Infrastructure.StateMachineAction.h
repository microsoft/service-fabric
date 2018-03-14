// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class StateMachineAction
            {
                DENY_COPY(StateMachineAction);

            public:
                virtual ~StateMachineAction() {}

                void PerformAction(
                    std::wstring const & activityId, 
                    EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent);

                void CancelAction(ReconfigurationAgent & ra);

            protected:
                StateMachineAction();

            private:
                virtual void OnPerformAction(
                    std::wstring const & activityId, 
                    EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent) = 0;

                virtual void OnCancelAction(
                    ReconfigurationAgent & ra) = 0;
            };
        }
    }
}
