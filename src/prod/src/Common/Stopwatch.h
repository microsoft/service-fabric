// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/DateTime.h"
#include "Common/StopwatchTime.h"

namespace Common
{
    class Stopwatch
    {
    public:
        Stopwatch()
        {
            Reset();
        }

        __declspec (property(get=get_IsRunning))            bool IsRunning;
        __declspec (property(get=get_Elapsed))              TimeSpan Elapsed;
        __declspec (property(get=get_ElapsedMilliseconds))  int64 ElapsedMilliseconds;
        __declspec (property(get=get_ElapsedMicroseconds))  int64 ElapsedMicroseconds;
        __declspec (property(get=get_ElapsedTicks))         int64 ElapsedTicks;

        void Start();
        void Stop();
        void Reset();
        void Restart();
        bool get_IsRunning() const;
        TimeSpan get_Elapsed() const;
        int64 get_ElapsedMilliseconds() const;
        int64 get_ElapsedMicroseconds() const;
        int64 get_ElapsedTicks() const;

        static StopwatchTime Now();
        static int64 GetTimestamp();

        static LARGE_INTEGER const Frequency;
        static double const TickFrequency;
        static int64 const Offset;

        static int64 ConvertTicksToMicroseconds(int64 ticks);

        static int64 ConvertTicksToMilliseconds(int64 ticks);

        static double ConvertTicksToMilliseconds(double ticks);

        // Convert value returned by QueryPerformanceCounter into ticks
        static double ConvertTimestampToTicks(int64 timeStamp);

    private:
        static LARGE_INTEGER InitializeFrequency();
        static double InitializeTickFrequency();
        static int64 InitializeOffset();

        static int64 const TicksPerMicrosecond = 10;
        static int64 const TicksPerMillisecond = 10000;
        static int64 const TicksPerSecond = TicksPerMillisecond * 1000;

        int64 timeElapsed_;
        int64 startTimestamp_;
        bool isRunning_;
    };
};
