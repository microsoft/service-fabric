// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TimeoutHelper
    {
        DENY_COPY(TimeoutHelper);

    public:
        explicit TimeoutHelper(Common::TimeSpan timeout);

        __declspec(property(get=get_IsExpired)) bool IsExpired;
        __declspec(property(get=get_OriginalTimeout)) Common::TimeSpan OriginalTimeout;

        bool get_IsExpired() const
        {
            return ((originalTimeout_ != Common::TimeSpan::MaxValue) && (deadline_ != Common::StopwatchTime::MaxValue) && (Common::Stopwatch::Now() > deadline_));
        }

        Common::TimeSpan get_OriginalTimeout() const
        {
            return originalTimeout_;
        }

        Common::TimeSpan GetRemainingTime() const;

        void SetRemainingTime(Common::TimeSpan timeout);

    private:
        Common::StopwatchTime deadline_;
        Common::TimeSpan originalTimeout_;
    };
}
