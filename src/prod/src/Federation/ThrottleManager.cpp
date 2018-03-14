// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Federation;

ThrottleManager::ThrottleManager(int threshold, TimeSpan delayLowerBound, TimeSpan delayUpperBound)
    : threshold_(threshold),
      delayLowerBound_(delayLowerBound),
      delayUpperBound_(delayUpperBound),
      continuousCount_(0),
      lastActivity_(DateTime::Zero),
      delay_(TimeSpan::Zero)
{
}

void ThrottleManager::Add()
{
    DateTime now = DateTime::Now();

    if (delay_ > TimeSpan::Zero)
    {
        if (now > lastActivity_ + delay_ + delay_)
        {
            delay_ = TimeSpan::Zero;
        }
        else
        {
            delay_ = min(TimeSpan::FromTicks(delay_.Ticks * 3 / 2), delayUpperBound_);
        }
    }

    if (delay_ == TimeSpan::Zero)
    {
        if (now > lastActivity_ + delayLowerBound_ + delayLowerBound_)
        {
            continuousCount_ = 0;
        }

        continuousCount_++;
        if (continuousCount_ >= threshold_)
        {
            delay_ = min(TimeSpan::FromTicks(delayLowerBound_.Ticks * 3 / 2), delayUpperBound_);
        }
    }

    lastActivity_ = now;
}

TimeSpan ThrottleManager::CheckDelay() const
{
    return lastActivity_ + delay_ - DateTime::Now();
}
