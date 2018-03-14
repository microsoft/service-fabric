// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Used to abstract away the policy for handling retryable errors and breaking loops in RA
        class RetryableErrorState
        {
        public:
            RetryableErrorState();

            __declspec(property(get=get_CurrentState)) RetryableErrorStateName::Enum CurrentState;
            RetryableErrorStateName::Enum get_CurrentState() const { return currentState_; }

            __declspec(property(get=get_CurrentFailureCount)) int CurrentFailureCount;
            int get_CurrentFailureCount() const { return currentFailureCount_; }

            void EnterState(RetryableErrorStateName::Enum state);

            RetryableErrorAction::Enum OnFailure(Reliability::FailoverConfig const & config);

            RetryableErrorAction::Enum OnSuccessAndTransitionTo(
                RetryableErrorStateName::Enum state,
                Reliability::FailoverConfig const & config);

            bool IsLastRetryBeforeDrop(Reliability::FailoverConfig const & config) const;

            void Reset();

            void Test_Set(RetryableErrorStateName::Enum state, int currentCount);

            void Test_SetThresholds(RetryableErrorStateName::Enum state, int warningThreshold, int dropThreshold, int restartThreshold, int errorThreshold);

            std::string GetTraceState(Reliability::FailoverConfig const & config) const;

        private:
            RetryableErrorStateThresholdCollection::Threshold GetCurrentThresholds(Reliability::FailoverConfig const&) const;

            RetryableErrorStateThresholdCollection thresholds_;

            RetryableErrorStateName::Enum currentState_;
            int currentFailureCount_;
        };
    }
}

