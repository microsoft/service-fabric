// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

FailoverThrottle::FailoverThrottle(Common::ConfigEntry<Common::TimeSpan> & throttleTime)
    : stopwatch_()
    , throttleTime_(throttleTime)
    , isFirstCall_(true)
{
}

bool FailoverThrottle::IsThrottling()
{
    if (!isFirstCall_) 
    {
        if (stopwatch_.Elapsed > throttleTime_.GetValue())
        {
            stopwatch_.Restart();
            return true;
        }

        return false;
    }

    isFirstCall_ = false;
    stopwatch_.Start();

    return true;
}
