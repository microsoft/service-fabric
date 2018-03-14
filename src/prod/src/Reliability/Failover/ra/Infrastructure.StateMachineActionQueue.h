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
            class StateMachineActionQueue
            {
                DENY_COPY(StateMachineActionQueue);

            public:
                StateMachineActionQueue();

                ~StateMachineActionQueue();

                __declspec(property(get = get_ActionQueue)) std::vector<StateMachineActionUPtr> const & ActionQueue;
                std::vector<StateMachineActionUPtr> const & get_ActionQueue() const { return actionQueue_; }

                __declspec(property(get = get_IsEmpty)) bool IsEmpty;
                bool get_IsEmpty() const { return actionQueue_.empty(); }

                void Enqueue(StateMachineActionUPtr && action);

                void ExecuteAllActions(
                    std::wstring const & activityId, 
                    EntityEntryBaseSPtr const &, 
                    ReconfigurationAgent & reconfigurationAgent);

                void AbandonAllActions(
                    ReconfigurationAgent & reconfigurationAgent);

            private:
                std::vector<StateMachineActionUPtr> actionQueue_;
                bool isConsumed_;
            };
        }
    }
}
