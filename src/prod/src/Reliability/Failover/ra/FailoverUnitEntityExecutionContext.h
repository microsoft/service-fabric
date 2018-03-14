// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitEntityExecutionContext : public Infrastructure::EntityExecutionContext
        {
        public:
            FailoverUnitEntityExecutionContext(
                FailoverConfig const & config,
                Infrastructure::IClock const & clock,
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue,
                Federation::NodeInstance const & nodeInstance,
                Infrastructure::UpdateContext & updateContext,
                Infrastructure::IEntityStateBase const * state) :
                Infrastructure::EntityExecutionContext(config, clock, hosting, queue, nodeInstance, updateContext, state)
                {
                }
        };
    }
}
