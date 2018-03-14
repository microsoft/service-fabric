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
            // Defines a linear retry policy
            class LinearRetryPolicy : public IRetryPolicy
            {
            public:
                LinearRetryPolicy(TimeSpanConfigEntry const & interval) :
                interval_(&interval),
                lastRetry_(Common::StopwatchTime::Zero)
                {
                }

                void Clear() override
                {           
                    lastRetry_ = Common::StopwatchTime::Zero;
                }

                void OnRetry(Common::StopwatchTime now) override
                {
                    lastRetry_ = now;
                }

                bool ShouldRetry(Common::StopwatchTime now) const override
                {
                    return (now - lastRetry_) >= interval_->GetValue();
                }

            private:
                Common::StopwatchTime lastRetry_;
                TimeSpanConfigEntry const * interval_;
            };
        }
    }
}


