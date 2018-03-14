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
            // Defines a retry policy
            // The retry policy is told when retries start
            // and is responsible for saying whether a retry
            // can happen at a particular time
            // it is also told about each individual retry
            class IRetryPolicy
            {
            public:
                virtual ~IRetryPolicy() {}

                // Informs the policy to clear any state it is maintaing
                // with respect to retries
                virtual void Clear() = 0;

                // Informs the policy that a retry happened
                // The caller is responsible for ensuring that 'now'
                // would be a time at which ShouldRetry(now) would be true
                // Calling OnRetry without start is undefined behavior
                virtual void OnRetry(Common::StopwatchTime now) = 0;

                // Return whether a retry is allowed at 'now'
                // If true is returned it implies that a retry is allowed
                // at 'now' or any time t > 'now'
                // Calling ShouldRetry without start is undefined behavior
                virtual bool ShouldRetry(Common::StopwatchTime now) const = 0;
            };
        }
    }
}


