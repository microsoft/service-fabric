// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define PERF_COUNTER_OBJECT_PROCESS L"Process"
#define PERF_COUNTER_THREAD_COUNT L"Thread Count"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceThrottle("Throttle");
static const StringLiteral MonitorTimerTag("PerfMonitor");
static PerfMonitorEventSource const perfMonitorTrace;

const size_t MaxSample = 120;

Throttle::SampleHistory::SampleHistory()
    : index1_(0)
    , index2_(0)
    , total1_(0)
    , total2_(0)
{
}

void Throttle::SampleHistory::Initialize(size_t samplesPerCycle)
{
    size_t capacity;
    if (samplesPerCycle > MaxSample)
    {
        capacity = MaxSample;
        samplePerEntry_ = samplesPerCycle / MaxSample;
        boundaryIndex_ = (samplePerEntry_ + 1) * MaxSample - samplesPerCycle;
    }
    else
    {
        capacity = samplesPerCycle;
        boundaryIndex_ = samplesPerCycle;
        samplePerEntry_ = 1;
    }

    data_.reserve(capacity);
}

int64 Throttle::SampleHistory::AddSample(int64 sample)
{
    total2_ += sample;
    index2_++;

    size_t count = (index1_ < boundaryIndex_ ? samplePerEntry_ : samplePerEntry_ + 1);
    if (index2_ >= count)
    {
        size_t avg = total2_ / count;
        if (index1_ < data_.size())
        {
            total1_ -= data_[index1_];
            data_[index1_] = avg;
        }
        else
        {
            data_.push_back(avg);
        }
        total1_ += avg;
        
        index1_++;
        if (index1_ >= data_.capacity())
        {
            index1_ = 0;
        }

        index2_ = 0;
        total2_ = 0;
    }

    return (data_.size() > 0 ? total1_ / data_.size() : total2_ / index2_);
}

Throttle::Throttle()
    : throttleEnabled_(false)
#ifdef PLATFORM_UNIX
    , processInfo_(*ProcessInfo::GetSingleton())
#endif
    , threadCount_(1)
    , threadThrottle_(0)
    , threadThrottleStartedTime_(StopwatchTime::MaxValue)
    , throttleTestAssertThreshold_(TimeSpan::MaxValue)
    , memoryThrottleInterval_(TimeSpan::MaxValue)
    , memorySize_(0)
    , memoryThrottle_(0)
    , memoryThrottleUpperLimit_(0)
    , memoryThrottleIncrementRatio_(0.0)
    , perMonitorTraceRatio_(CommonConfig::GetConfig().PerfMonitorTraceRatio)
    , perfQueryCount_(0)
    , monitorThreadpool_(Threadpool::CreateCustomPool(1, 10))
{
}

void Throttle::Enable()
{
    GetThrottle()->EnableInternal();
}

void Throttle::Test_Reload()
{
    auto & throttle = *GetThrottle();
    throttle.ReloadThreadThrottle();
    throttle.ReloadMemoryThrottle();
    throttle.ReloadMemoryThrottleUpperLimit();
    throttle.ReloadMonitorInterval();
    throttle.ReloadMonitorTraceRatio();
}

void Throttle::EnableInternal()
{
    TimeSpan sampleInterval = CommonConfig::GetConfig().PerfMonitorInterval;

    TransportConfig const & config = TransportConfig::GetConfig();

    threadThrottle_ = static_cast<DWORD>(config.ThreadThrottle);
    throttleTestAssertThreshold_ = config.ThrottleTestAssertThreshold;
    memoryThrottle_ = static_cast<int64>(config.MemoryThrottleInMB) * 1024 * 1024;
    memoryThrottleInterval_ = config.MemoryThrottleInterval;
    memoryThrottleUpperLimit_ = static_cast<int64>(config.MemoryThrottleUpperLimitInMB) * 1024 * 1024;
    memoryThrottleIncrementRatio_ = config.MemoryThrottleIncrementRatio;

    shortMemoryHistory_.Initialize(static_cast<size_t>(config.MemoryThrottleInterval.Ticks / sampleInterval.Ticks));
    longMemoryHistory_.Initialize(static_cast<size_t>(config.MemoryStableInterval.Ticks / sampleInterval.Ticks));

    LONG threadTestLimit = static_cast<LONG>(config.ThreadTestLimit);
    if (threadTestLimit > 0)
    {
        FabricSetThreadTestLimit(threadTestLimit);
    }

    TimeSpan perfMonitorInterval = CommonConfig::GetConfig().PerfMonitorInterval;
    WriteInfo(
        TraceThrottle,
        "PerfMonitorInterval = {0}, PerfMonitorTraceRatio = {1}, ThreadThrottle = {2}, ThreadTestLimit = {3}, MemoryThrottle = {4}",
        perfMonitorInterval,
        perMonitorTraceRatio_,
        threadThrottle_,
        threadTestLimit,
        memoryThrottle_);

    if (perfMonitorInterval <= TimeSpan::Zero)
    {
        WriteInfo(TraceThrottle, "Perf monitoring is disabled");
        return;
    }

    perfCounters_ = PerfCounters::CreateInstance(wformatString("{0}", ::GetCurrentProcessId()));

    monitorTimer_ = Timer::Create(
        MonitorTimerTag,
        [this](TimerSPtr const&) { this->TimerCallback(); },
        true,
        monitorThreadpool_->CallbackEnvPtr());

    monitorTimer_->Change(TimeSpan::Zero, sampleInterval);

    config.ThreadThrottleEntry.AddHandler([this](EventArgs const&) { ReloadThreadThrottle(); });
    config.MemoryThrottleInMBEntry.AddHandler([this](EventArgs const&) { ReloadMemoryThrottle(); });
    config.MemoryThrottleUpperLimitInMBEntry.AddHandler([this](EventArgs const&) { ReloadMemoryThrottleUpperLimit(); });
    CommonConfig::GetConfig().PerfMonitorIntervalEntry.AddHandler([this](EventArgs const&) { ReloadMonitorInterval(); });
    CommonConfig::GetConfig().PerfMonitorTraceRatioEntry.AddHandler([this](EventArgs const&) { ReloadMonitorTraceRatio(); });

    throttleEnabled_ = true;
}

PTP_CALLBACK_ENVIRON Throttle::MonitorCallbackEnv()
{
    return monitorThreadpool_->CallbackEnvPtr();
}

void Throttle::ReloadMonitorTraceRatio()
{
    auto oldValue = perMonitorTraceRatio_;
    perMonitorTraceRatio_ = CommonConfig::GetConfig().PerfMonitorTraceRatio;
    WriteInfo(TraceThrottle, "perMonitorTraceRatio_ updated: {0} -> {1}", oldValue, perMonitorTraceRatio_);
}

void Throttle::ReloadThreadThrottle()
{
    auto oldValue = threadThrottle_;
    threadThrottle_ = static_cast<DWORD>(TransportConfig::GetConfig().ThreadThrottle);
    WriteInfo(TraceThrottle, "ThreadThrottle updated: {0} -> {1}", oldValue, threadThrottle_);
}

void Throttle::ReloadMemoryThrottle()
{
    auto oldValue = memoryThrottle_;
    memoryThrottle_ = static_cast<int64>(TransportConfig::GetConfig().MemoryThrottleInMB) * 1024 * 1024;
    WriteInfo(TraceThrottle, "MemoryThrottle updated: {0} bytes -> {1} bytes", oldValue, memoryThrottle_);
}

void Throttle::ReloadMemoryThrottleUpperLimit()
{
    auto oldValue = memoryThrottleUpperLimit_;
    memoryThrottleUpperLimit_ = static_cast<int64>(TransportConfig::GetConfig().MemoryThrottleUpperLimitInMB) * 1024 * 1024;
    WriteInfo(TraceThrottle, "MemoryThrottleUpperLimit updated: {0} bytes -> {1} bytes", oldValue, memoryThrottleUpperLimit_);

}

void Throttle::ReloadMonitorInterval()
{
    auto newInterval = CommonConfig::GetConfig().PerfMonitorInterval;
    WriteInfo(TraceThrottle, "PerfMonitorInterval updated to {0}", newInterval);
    if (newInterval <= TimeSpan::Zero)
    {
        newInterval = TimeSpan::MaxValue;
    }

    monitorTimer_->Change(newInterval, newInterval);
    if (newInterval == TimeSpan::MaxValue)
    {
        WriteInfo(TraceThrottle, "monitor timer disabled");
    }
}

static int GetThreadCount()
{
#ifdef PLATFORM_UNIX
    return 0; // don't want to pay the cost of reading /proc file to get thread count,
    // as thread count is not used for throttling anymore, it is only for tracing
#else
    return FabricGetThreadCount(); // exported by FabricCommon.dll
#endif
}

bool Throttle::ShouldThrottle(Message const & message, bool mayThrottleReply)
{
    if (!throttleEnabled_ || message.HighPriority || (!message.RelatesTo.IsEmpty() && !mayThrottleReply))
    {
        return false;
    }

    auto activeCallbackCount = Threadpool::ActiveCallbackCount();
    bool shouldThrottle = IsThreadThresholdExceeded(activeCallbackCount) || IsMemoryThresholdExceeded();
    if (shouldThrottle)
    {
        trace.Throttle(
            threadCount_,
            activeCallbackCount,
            threadThrottle_,
            memorySize_,
            memoryThrottle_,
            message.TraceId(),
            message.Actor,
            message.Action);
    }

    return shouldThrottle;
}

void Throttle::TimerCallback()
{
    QueryPerfData();
}

void Throttle::GetMemorySize()
{
#ifdef PLATFORM_UNIX
    memorySize_ = processInfo_.RefreshDataSize();
#else
    PROCESS_MEMORY_COUNTERS counters;

    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(PROCESS_MEMORY_COUNTERS)))
    {
        memorySize_ = static_cast<int64>(counters.PagefileUsage);
    }
#endif
}

void Throttle::IncreaseMemoryThrottle(int64 baseValue, int64 upperLimit)
{
    int64 newValue = static_cast<int64>(baseValue * (1 + memoryThrottleIncrementRatio_));
    if (upperLimit > 0 && newValue > upperLimit)
    {
        newValue = upperLimit;
    }

    if (newValue > memoryThrottle_)
    {
        trace.MemoryThrottleIncresed(memoryThrottle_, newValue);
        memoryThrottle_ = newValue;
    }
}

void Throttle::QueryPerfData()
{
    // uncomment if we use perf counter again
    // this->threadCount_ = ...
    // this->lastQueryTime_ = DateTime::Now();
    ++perfQueryCount_;

    auto  activeCallbackCount = Threadpool::ActiveCallbackCount();
    perfCounters_->NumberOfActiveCallbacks.Value = activeCallbackCount;

    threadCount_ = GetThreadCount();
    GetMemorySize();

    int64 shortAverage, longAverage;
    {
        AcquireExclusiveLock grab(lock_);

        shortAverage = shortMemoryHistory_.AddSample(memorySize_);
        longAverage = longMemoryHistory_.AddSample(memorySize_);
    }

    if ((perMonitorTraceRatio_ > 0) && ((perfQueryCount_ % perMonitorTraceRatio_) == 0))
    {
        perfMonitorTrace.Sample(threadCount_, activeCallbackCount, memorySize_, shortAverage, longAverage);
    }

    if (memoryThrottle_ > 0)
    {
        if (shortAverage > memoryThrottle_)
        {
            ASSERT_IF(
                memoryThrottleUpperLimit_ > 0 && memoryThrottle_ >= memoryThrottleUpperLimit_,
                "Memory size {0} exceeded upper limit {1} for {2}",
                memorySize_, memoryThrottleUpperLimit_, memoryThrottleInterval_);

            IncreaseMemoryThrottle(shortAverage, memoryThrottleUpperLimit_);
        }
        else if (longAverage * (1 + memoryThrottleIncrementRatio_) > memoryThrottle_)
        {
            IncreaseMemoryThrottle(memoryThrottle_, static_cast<int64>(memoryThrottleUpperLimit_ * (1 - memoryThrottleIncrementRatio_)));
        }
    }

    if (throttleTestAssertThreshold_ > TimeSpan::Zero)
    {
        if (IsThreadThresholdExceeded(activeCallbackCount))
        {
            if (threadThrottleStartedTime_ != StopwatchTime::MaxValue)
            {
                TimeSpan duration = Stopwatch::Now() - threadThrottleStartedTime_;
                if (duration > throttleTestAssertThreshold_)
                {
                    trace.ThrottleExceeded(duration.TotalSeconds(), threadCount_, threadThrottle_, memorySize_, memoryThrottle_);
                    Assert::TestAssert();
                }
            }
            else
            {
                threadThrottleStartedTime_ = Stopwatch::Now();
            }
        }
        else
        {
            threadThrottleStartedTime_ = StopwatchTime::MaxValue;
        }
    }
}

bool Throttle::IsEnabled()
{
    return GetThrottle()->throttleEnabled_;
}

bool Throttle::IsThreadThresholdExceeded(LONG count)
{
    return (0 < threadThrottle_ && threadThrottle_ <= count);
}

bool Throttle::IsMemoryThresholdExceeded()
{
    return (0 < memoryThrottle_ && memoryThrottle_ <= memorySize_);
}

INIT_ONCE Throttle::initOnce_ = INIT_ONCE_STATIC_INIT;
Throttle* Throttle::singleton_ = nullptr;

BOOL CALLBACK Throttle::InitFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = new Throttle();
    return TRUE;
}

Throttle * Throttle::GetThrottle()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = FALSE; 

    bStatus = ::InitOnceExecuteOnce(
        &Throttle::initOnce_,
        Throttle::InitFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize Throttle singleton");
    return Throttle::singleton_;
}
