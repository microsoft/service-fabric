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
            class RAStopwatch
            {
            public:
                RAStopwatch(bool useTestAssert = false) :
                    startTime_(Common::StopwatchTime::Zero),
                    elapsed_(Common::TimeSpan::Zero),
                    useTestAssert_(useTestAssert)
                {
                }

                __declspec(property(get = get_IsRunning)) bool IsRunning;
                bool get_IsRunning() const { return startTime_ != Common::StopwatchTime::Zero; }

                __declspec(property(get = get_Elapsed)) Common::TimeSpan Elapsed;
                Common::TimeSpan get_Elapsed() const { return elapsed_; }

                void Start(IClock const & clock)
                {
                    AssertIfRunning();
                    startTime_ = clock.Now();
                }

                void Stop(IClock const & clock)
                {
                    AssertIfNotRunning();
                    elapsed_ = clock.Now() - startTime_;
                    startTime_ = Common::StopwatchTime::Zero;
                }

                void AssertIfRunning() const
                {
                    AssertIfIsRunningIs(true);
                }

                void AssertIfNotRunning() const
                {
                    AssertIfIsRunningIs(false);
                }

            private:

                void AssertIfIsRunningIs(bool expected) const
                {
                    if (useTestAssert_)
                    {
                        TESTASSERT_IF(IsRunning == expected, "Mismatch {0} {1}", IsRunning, expected);
                    }
                    else
                    {
                        ASSERT_IF(IsRunning == expected, "Mismatch {0} {1}", IsRunning, expected);
                    }
                }

                bool useTestAssert_;
                Common::StopwatchTime startTime_;
                Common::TimeSpan elapsed_;
            };
        }
    }
}



