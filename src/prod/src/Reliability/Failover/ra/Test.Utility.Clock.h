// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class TestClock : public Infrastructure::Clock
            {
            public:
                TestClock() : isManual_(false) {}

                Common::StopwatchTime SetManualMode()
                {
                    isManual_ = true;
                    currentTime_ = innerClock_.Now();
                    currentDateTime_ = innerClock_.DateTimeNow();
                    return currentTime_;
                }

                void AdvanceTime(Common::TimeSpan delta)
                {
                    currentTime_ += delta;
                    currentDateTime_ = currentDateTime_ + delta;
                }

                void AdvanceTimeBySeconds(int seconds)
                {
                    AdvanceTime(Common::TimeSpan::FromSeconds(seconds));
                }

                Common::StopwatchTime Now() const
                {
                    return isManual_ ? currentTime_ : innerClock_.Now();
                }

                Common::DateTime DateTimeNow() const
                {
                    return isManual_ ? currentDateTime_ : innerClock_.DateTimeNow();
                }

            private:
                bool isManual_;
                Common::StopwatchTime currentTime_;
                Common::DateTime currentDateTime_;
                Infrastructure::Clock innerClock_;
            };
        }
    }
}
