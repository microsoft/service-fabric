// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
#define TIMER_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, Timer, trace_id, trace_level, __VA_ARGS__)

    class TimerEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(Created, uint64, StringLiteral, uint64); 
        DECLARE_STRUCTURED_TRACE(CreatedQueued, uint64, StringLiteral, uint64, uint64); 
        DECLARE_STRUCTURED_TRACE(Destructed, uint64, uint64); 
        DECLARE_STRUCTURED_TRACE(DestructedQueued, uint64, uint64); 
        DECLARE_STRUCTURED_TRACE(Scheduled, uint64, bool, TimeSpan, TimeSpan);
        DECLARE_STRUCTURED_TRACE(Scheduled2, uint64, bool, DateTime, TimeSpan);
        DECLARE_STRUCTURED_TRACE(CancelCalled, uint64, bool);
        DECLARE_STRUCTURED_TRACE(CancelWaitSkipped, uint64);
        DECLARE_STRUCTURED_TRACE(CancelWait, uint64);
        DECLARE_STRUCTURED_TRACE(EnterCallback, uint64);
        DECLARE_STRUCTURED_TRACE(LeaveCallback, uint64);

        TimerEventSource()
            : TIMER_STRUCTURED_TRACE(
                Created,
                4,
                Noise,
                "{0:x} tag='{1}', count={2}",
                "timerPtr",
                "tag",
                "count")
            , TIMER_STRUCTURED_TRACE(
                Destructed,
                5,
                Noise,
                "{0:x} destructed, count={1}",
                "timerPtr",
                "count")
            , TIMER_STRUCTURED_TRACE(
                Scheduled,
                6,
                Noise,
                "{0:x} scheduled = {1}, dueTime = {2}, period = {3}",
                "timerPtr",
                "set",
                "dueTime",
                "period")
            , TIMER_STRUCTURED_TRACE(
                CancelCalled,
                7,
                Noise,
                "{0:x} cancel called, waitForCallbackOnCancel = {1}",
                "timerPtr",
                "waitForCallbackOnCancel")
            , TIMER_STRUCTURED_TRACE(
                CancelWaitSkipped,
                8,
                Noise,
                "{0:x} cancel is called from callback",
                "timerPtr")
            , TIMER_STRUCTURED_TRACE(
                EnterCallback,
                9,
                Noise,
                "{0:x} enter callback",
                "timerPtr")
            , TIMER_STRUCTURED_TRACE(
                LeaveCallback,
                10,
                Noise,
                "{0:x} leave callback",
                "timerPtr")
            , TIMER_STRUCTURED_TRACE(
                Scheduled2,
                11,
                Noise,
                "{0:x} scheduled = {1}, dueTime = {2}, period = {3}",
                "timerPtr",
                "set",
                "dueTime",
                "period")
            , TIMER_STRUCTURED_TRACE(
                CancelWait,
                12,
                Noise,
                "{0:x} wait for cancel completion",
                "timerPtr")
            , TIMER_STRUCTURED_TRACE(
                CreatedQueued,
                13,
                Noise,
                "{0:x} tag='{1}', count={2}, queue={3:x}",
                "timerPtr",
                "tag",
                "count",
                "queue")
            , TIMER_STRUCTURED_TRACE(
                DestructedQueued,
                14,
                Noise,
                "{0:x} destructed, count={1}",
                "timerPtr",
                "count")
        {
        }
    };
}
