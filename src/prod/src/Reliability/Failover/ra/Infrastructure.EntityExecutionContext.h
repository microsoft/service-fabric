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
            /*
                This object represents some work that is executing on an entity
                This allows us to hide away the RA and provide one object that can be transferred around
                and allow entities to access different services (hosting, config, clock)
            */
            class EntityExecutionContext 
            {
            public:
                EntityExecutionContext(
                    FailoverConfig const & config, 
                    IClock const & clock,
                    Hosting::HostingAdapter & hosting,
                    StateMachineActionQueue & queue,
                    Federation::NodeInstance const & nodeInstance,
                    UpdateContext & updateContext,
                    IEntityStateBase const * state) :
                    config_(&config),
                    clock_(&clock),
                    hosting_(&hosting),
                    nodeInstance_(&nodeInstance),
                    stateUpdateContext_(queue, updateContext),
                    assertContext_(state)
                {
                }

                __declspec(property(get = get_Config)) FailoverConfig const & Config;
                FailoverConfig const & get_Config() const { return *config_; }

                __declspec(property(get = get_Clock)) IClock const & Clock;
                IClock const & get_Clock() const { return *clock_; }

                __declspec(property(get = get_Queue)) StateMachineActionQueue & Queue;
                StateMachineActionQueue & get_Queue() const { return stateUpdateContext_.ActionQueue; }

                __declspec(property(get = get_Hosting)) Hosting::HostingAdapter & Hosting;
                Hosting::HostingAdapter & get_Hosting() const { return *hosting_; }

                __declspec(property(get = get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
                Federation::NodeInstance const & get_NodeInstance() const { return *nodeInstance_; }

                __declspec(property(get = get_UpdateContextObj)) UpdateContext & UpdateContextObj;
                UpdateContext & get_UpdateContextObj() const { return stateUpdateContext_.UpdateContextObj; }

                __declspec(property(get = get_StateUpdateContextObj)) StateUpdateContext & StateUpdateContextObj;
                StateUpdateContext & get_StateUpdateContextObj() { return stateUpdateContext_; }

                __declspec(property(get = get_AssertContext)) EntityAssertContext const & AssertContext;
                EntityAssertContext const & get_AssertContext() const { return assertContext_; }

                template<typename T>
                T & As()
                {
                    return static_cast<T&>(*this);
                }

                template<typename T>
                T const & As() const
                {
                    return static_cast<T const &>(*this);
                }

                static EntityExecutionContext Create(
                    ReconfigurationAgent & ra, 
                    StateMachineActionQueue & queue, 
                    UpdateContext & updateContext,
                    IEntityStateBase const * state);

            private:
                Hosting::HostingAdapter * hosting_;
                FailoverConfig const * config_;
                IClock const * clock_;
                StateUpdateContext stateUpdateContext_;
                Federation::NodeInstance const * nodeInstance_;
                EntityAssertContext assertContext_;
            };
        }
    }
}



