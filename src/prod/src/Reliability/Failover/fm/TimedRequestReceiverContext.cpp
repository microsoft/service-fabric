// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;
using namespace Transport;

TimedRequestReceiverContext::TimedRequestReceiverContext(
    FailoverManager & fm,
    RequestReceiverContextUPtr && context,
    wstring const& action)
    : fm_(fm)
    , context_(move(context))
    , action_(action)
    , stopwatch_()
{
    stopwatch_.Start();
}

void TimedRequestReceiverContext::Reject(ErrorCode const& error, ActivityId const& activityId)
{
    TraceIfSlow();

    context_->Reject(error, activityId);
}

void TimedRequestReceiverContext::Reply(MessageUPtr && reply)
{
    TraceIfSlow();

    context_->Reply(move(reply));
}

void TimedRequestReceiverContext::TraceIfSlow() const
{
    fm_.FailoverUnitCounters->MessageProcessDurationBase.Increment();
    fm_.FailoverUnitCounters->MessageProcessDuration.IncrementBy(static_cast<PerformanceCounterValue>(stopwatch_.ElapsedMilliseconds));

    if (stopwatch_.ElapsedMilliseconds > FailoverConfig::GetConfig().MessageProcessTimeLimit.TotalMilliseconds())
    {
        fm_.MessageEvents.SlowMessageProcess(action_, stopwatch_.ElapsedMilliseconds);
    }
}
