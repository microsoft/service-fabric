// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TestHooks;

namespace
{
    const int WaitPollIntervalMilliseconds = 2000;
}

Signal::Signal(FailTestFunctionPtr failTestFunctionPtr, wstring const & id)
: failTestFunctionPtr_(failTestFunctionPtr),
  id_(id)
{
}

bool Signal::IsSet()
{
    return !signalEvent_.IsSet();
}

void Signal::FailIfSignalHit(TimeSpan timeout)
{
    if (isSignalHitEvent_.WaitOne(timeout))
    {
        failTestFunctionPtr_(wformatString("Signal was it when it was expected it would not be hit{0}", id_));
        return;
    }
}

void Signal::WaitForSignalHit()
{
    const TimeSpan WaitForSignalHitTimeout = TimeSpan::FromSeconds(120);
    TimeoutHelper timeoutHelper(WaitForSignalHitTimeout);

    while (!timeoutHelper.IsExpired)
    {
        if (isSignalHitEvent_.WaitOne(WaitPollIntervalMilliseconds))
        {
            WriteInfo("Signal", id_, "Signal {0} has been hit", id_);
            return;
        }
    }

    failTestFunctionPtr_(wformatString("Nobody hit the signal in configured time {0}", id_));
}

void Signal::WaitForSignalReset()
{
    // Mark that somebody has hit the signal
    isSignalHitEvent_.Set();
    
    for (;;)
    {
        if (signalEvent_.WaitOne(WaitPollIntervalMilliseconds))
        {
            WriteInfo("Signal", id_, "Signal {0} is clear. API will resume now", id_);
            return;
        }

        WriteInfo("Signal", id_, "Signal {0} is set. API is currently blocking", id_);
    }
}

// Sets the block
void Signal::SetSignal()
{
    isSignalHitEvent_.Reset();
    signalEvent_.Reset();
    WriteInfo("Signal", id_, "Signal {0} is set. Next API will block", id_);
}

// Removes the block
void Signal::ResetSignal()
{
    WriteInfo("Signal", id_, "Signal {0} is reset (cleared). API will resume now", id_);
    signalEvent_.Set();
}
