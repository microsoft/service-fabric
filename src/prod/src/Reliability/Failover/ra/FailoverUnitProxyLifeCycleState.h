// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Encapsulates the state transition time and the state
        class FailoverUnitProxyLifeCycleState
        {
        public:
            FailoverUnitProxyLifeCycleState() :
                state_(FailoverUnitProxyStates::Closed),
                lastTransitionTime_(Common::Stopwatch::Now())
            {
            }

            __declspec(property(get = get_LastTransitionTime)) Common::StopwatchTime LastTransitionTime;
            Common::StopwatchTime get_LastTransitionTime() const
            {
                return lastTransitionTime_;
            }

            __declspec(property(get = get_State)) FailoverUnitProxyStates::Enum State;
            FailoverUnitProxyStates::Enum get_State() const
            {
                return state_;
            }

            FailoverUnitProxyLifeCycleState & operator=(FailoverUnitProxyStates::Enum state)
            {
                state_ = state;
                lastTransitionTime_ = Common::Stopwatch::Now();
                return *this;
            }

            operator FailoverUnitProxyStates::Enum () const
            {
                return state_;
            }

            bool operator==(FailoverUnitProxyStates::Enum other) const
            {
                return state_ == other;
            }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w << state_;
            }

        private:
            FailoverUnitProxyStates::Enum state_;
            Common::StopwatchTime lastTransitionTime_;
        };
    }
}
