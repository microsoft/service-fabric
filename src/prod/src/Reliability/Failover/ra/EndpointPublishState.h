// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Part of FT state tracking whether publishing of the endpoint of a primary to fm is pending
        class EndpointPublishState
        {
        public:
            EndpointPublishState(
                TimeSpanConfigEntry const * publishTimeout,
                ReconfigurationState const * reconfigState, 
                FMMessageState * fmMessageState);

            EndpointPublishState(
                ReconfigurationState const * reconfigState, 
                FMMessageState * fmMessageState, 
                EndpointPublishState const & other);

            EndpointPublishState & operator=(
                EndpointPublishState const & other);

            __declspec(property(get = get_IsEndpointPublishPending)) bool IsEndpointPublishPending;
            bool get_IsEndpointPublishPending() const { return state_ == State::PublishPending; }
            void Test_SetEndpointPublishPending() { state_ = State::PublishPending; }

            void StartReconfiguration(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            // Return whether a message send is required
            bool OnEndpointUpdated(
                Infrastructure::IClock const & clock,
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReconfigurationTimer(
                Infrastructure::IClock const & clock,
                Infrastructure::StateUpdateContext & stateUpdateContext,
                __out bool & sendMessage,
                __out bool & isRetryRequired);

            void OnFmReply(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void Clear(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void AssertInvariants(
                FailoverUnit const & owner) const;

        private:
            class State
            {
            public:
                enum Enum
                {
                    None,
                    PublishPending,
                    Completed
                };
            };
            
            bool HasTimeoutExpired(Infrastructure::IClock const & clock) const;

            bool UpdateFMMessageState(Infrastructure::StateUpdateContext & context);
            
            State::Enum state_;

            TimeSpanConfigEntry const * publishTimeout_;
            ReconfigurationState const * reconfigState_;
            FMMessageState * fmMessageState_;
        };
    }
}
