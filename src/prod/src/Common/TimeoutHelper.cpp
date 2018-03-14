// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{

    TimeoutHelper::TimeoutHelper(TimeSpan timeout)
    {
        SetRemainingTime(timeout);
    }

    TimeSpan TimeoutHelper::GetRemainingTime() const
    {
        if (deadline_ == StopwatchTime::MaxValue)
        {
            return TimeSpan::MaxValue;
        }
        else
        {
            TimeSpan remaining = deadline_ - Stopwatch::Now();
            if (remaining <= TimeSpan::Zero)
            {
                return TimeSpan::Zero;
            }
            else
            {
                return remaining;
            }
        }
    }

    void TimeoutHelper::SetRemainingTime(TimeSpan timeout)
    {
        originalTimeout_ = timeout;
        deadline_ = Stopwatch::Now() + timeout;
    }
}
