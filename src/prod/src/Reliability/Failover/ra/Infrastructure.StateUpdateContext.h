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
            // Encapsulates action queue and update context
            // Everything needed to change state
            class StateUpdateContext
            {
            public:
                StateUpdateContext(
                    StateMachineActionQueue & queue,
                    UpdateContext & updateContext) :
                    queue_(&queue),
                    updateContext_(&updateContext)
                {
                }

                __declspec(property(get = get_Queue)) StateMachineActionQueue & ActionQueue;
                StateMachineActionQueue & get_Queue() const { return *queue_; }

                __declspec(property(get = get_UpdateContextObj)) UpdateContext & UpdateContextObj;
                UpdateContext & get_UpdateContextObj() const { return *updateContext_; }

            private:
                UpdateContext * updateContext_;
                StateMachineActionQueue * queue_;
            };
        }
    }
}



