// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "Common/Stopwatch.h"

namespace Common
{
    LARGE_INTEGER const Stopwatch::Frequency = Stopwatch::InitializeFrequency();
    double const Stopwatch::TickFrequency = Stopwatch::InitializeTickFrequency();
    int64 const Stopwatch::Offset = Stopwatch::InitializeOffset();

    StopwatchTime Stopwatch::Now()
    {
#ifdef PLATFORM_UNIX
        return StopwatchTime(static_cast<int64>(GetTimestamp() * TickFrequency));
#else
        return StopwatchTime(static_cast<int64>(GetTimestamp() * TickFrequency) + Offset);
#endif
    }

    int64 Stopwatch::GetTimestamp()
    {
        LARGE_INTEGER currentTimestamp;
        QueryPerformanceCounter(&currentTimestamp);
        return currentTimestamp.QuadPart;
    }

    LARGE_INTEGER Stopwatch::InitializeFrequency()
    {
        LARGE_INTEGER performanceFrequency;
        QueryPerformanceFrequency(&performanceFrequency);
        return performanceFrequency;
    }

    double Stopwatch::InitializeTickFrequency()
    {
        return static_cast<double>(Stopwatch::TicksPerSecond)/static_cast<double>(Stopwatch::Frequency.QuadPart);
    }

    int64 Stopwatch::InitializeOffset()
    {
        return (DateTime::Now().Ticks - static_cast<int64>(GetTimestamp() * InitializeTickFrequency()));
    }

    void Stopwatch::Start()
    {
        if (!isRunning_)
        {
            startTimestamp_ = GetTimestamp();
            isRunning_ = true;
        }
    }

    void Stopwatch::Stop()
    {
        if (isRunning_)
        {
            int64 endTimestamp = GetTimestamp();
            int64 timeElapsedThisPeriod = endTimestamp - startTimestamp_;
            timeElapsed_ += timeElapsedThisPeriod;
            isRunning_ =false;
        }
    }

    void Stopwatch::Reset()
    {
        timeElapsed_ = 0;
        isRunning_ = false;
        startTimestamp_ = 0;
    }

    void Stopwatch::Restart()
    {
        timeElapsed_ = 0;
        startTimestamp_ = GetTimestamp();
        isRunning_ = true;
    }

    bool Stopwatch::get_IsRunning() const 
    { 
        return isRunning_;
    }

    TimeSpan Stopwatch::get_Elapsed() const 
    { 
        return TimeSpan::FromTicks(get_ElapsedTicks());
    }

    int64 Stopwatch::get_ElapsedMicroseconds() const 
    { 
        return Stopwatch::ConvertTicksToMicroseconds(get_ElapsedTicks());
    }

    int64 Stopwatch::get_ElapsedMilliseconds() const 
    { 
        return Stopwatch::ConvertTicksToMilliseconds(get_ElapsedTicks());
    }

    int64 Stopwatch::get_ElapsedTicks() const 
    { 
        int64 elapsed = timeElapsed_;

        if (isRunning_) 
        {
            int64 currentTimeStamp = GetTimestamp();                 
            int64 elapsedUntilNow = currentTimeStamp - startTimestamp_;
            elapsed += elapsedUntilNow;
        }

        return static_cast<int64>(Stopwatch::ConvertTimestampToTicks(elapsed));
    }

    int64 Stopwatch::ConvertTicksToMicroseconds(int64 ticks)
    {
        return ticks / TicksPerMicrosecond;
    }

    int64 Stopwatch::ConvertTicksToMilliseconds(int64 ticks)
    {
        return ticks / TicksPerMillisecond;
    }

    double Stopwatch::ConvertTicksToMilliseconds(double ticks)
    {
        return ticks / TicksPerMillisecond;
    }

    double Stopwatch::ConvertTimestampToTicks(int64 timeStamp)
    {
        return timeStamp * TickFrequency;
    }
} // end namespace Common
