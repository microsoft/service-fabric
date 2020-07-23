// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;
using namespace NightWatchTXRService;

int GetConfigValueInt(
    IFabricConfigurationPackage * configPackage,
    wstring sectionName,
    wstring parameterName);


StringLiteral const TraceSource("V1RPS_Test");

static string throughputMeasurementName = "V1ReplPerfThroughput";
static string throughputMetricName = "ThroughputOpsPerSecond";
static string durationMeasurementName = "V1ReplPerfDuration";
static string durationMetricName = "DurationInSeconds";

#define WRITEINFO(message) Trace.WriteInfo(TraceSource, "{0} at Partition = {1} Replica = {2}", message, partitionId_, replicaId_)

Test::Test(
    NightWatchTestParameters const & parameters,
    ComPointer<IFabricStateReplicator> const & replicator)
    : numberOfOperations_(0)
    , inflightOperationCount_(0)
    , parameters_(parameters)
    , replicator_(replicator)
    , data_()
{
    for (ULONG i = 0; i < parameters_.MaxOutstandingOperations; i++)
    {
        data_.push_back(ServiceEntry(i));
    }

    Trace.WriteInfo(
        TraceSource,
        "Writing Value of length {0} in this test",
        parameters_.ValueSizeInBytes);
}

NightWatchTXRService::PerfResult::SPtr Test::Execute(NightWatchTestParameters const & testParameters, ComPointer<IFabricStateReplicator> const & replicator, KAllocator & allocator)
{
    Test t(testParameters, replicator);
    return t.Run(allocator);
}

NightWatchTXRService::PerfResult::SPtr Test::Run(KAllocator & allocator)
{
    Stopwatch watch;
    watch.Start();
    int64 startTicks = DateTime::Now().Ticks;

    for (ULONG i = 0; i < parameters_.MaxOutstandingOperations; i++)
    {
        auto ref = &data_[i];

        Threadpool::Post(
            [ref, this]
        {
            Trace.WriteInfo(
                TraceSource,
                "ServiceEntry {0} starting to replicate",
                ref->Key);

            StartReplicatingIfNeeded(*ref, *this);
        });
    }

    for (;;)
    {
        Sleep(1000);

        auto opCount = numberOfOperations_.load();
        auto inflightOpCount = inflightOperationCount_.load();
        if (opCount >= (long)parameters_.NumberOfOperations &&
            inflightOpCount == 0)
        {
            break;
        }

        Trace.WriteInfo(
            TraceSource,
            "Ops Completed = {0}. Inflight ops = {1}. Waiting until {2}",
            opCount,
            inflightOpCount,
            parameters_.NumberOfOperations);
    }

    watch.Stop();
    int64 endTicks = DateTime::Now().Ticks;

    double opsPerMillisecond = (double)numberOfOperations_.load() / (double)watch.ElapsedMilliseconds;
    double opsPerSecond = 1000 * opsPerMillisecond;

    Trace.WriteInfo(
        TraceSource,
        "Time taken {0}; Operations Per Second = {1}; Operations per Millisecond = {2}; numberOfOperations_.load()  = {3}; ElapsedMilliseconds = {4}",
        watch.Elapsed.ToString(),
        opsPerSecond,
        opsPerMillisecond,
        numberOfOperations_.load(),
        watch.ElapsedMilliseconds);

    try 
    {
        return PerfResult::Create(
            (ULONG)numberOfOperations_.load(),
            watch.ElapsedMilliseconds / 1000,
            opsPerSecond,
            opsPerMillisecond,
            opsPerSecond,
            1000.0 * (1.0 / (opsPerSecond / parameters_.MaxOutstandingOperations)),
            1,
            startTicks,
            endTicks,
            L"",
            TestStatus::Enum::Finished,
            nullptr,
            allocator);
    }
    catch (const exception & e)
    {
        Trace.WriteInfo(
            TraceSource,
            "Error writing perf file : {0}",
            e.what());
    }
    return PerfResult::Create(Data::Utilities::SharedException::Create(allocator).RawPtr(), allocator);
}

void Test::StartReplicatingIfNeeded(ServiceEntry & entry, Test & test)
{
    auto opCount = test.numberOfOperations_.load();
    if (opCount < (long)test.parameters_.NumberOfOperations)
    {
        ReplicateServiceEntry(entry, test);
    }
    else
    {
        Trace.WriteInfo(
            TraceSource,
            "ServiceEntry {0} stopped replicating",
            entry.Key);
    }
}
        
void Test::ReplicateServiceEntry(ServiceEntry & entry, Test & test)
{
    test.inflightOperationCount_ += 1;

    BOOLEAN completedSync = false;

    ComPointer<ComAsyncOperationCallbackHelper> callback = make_com<ComAsyncOperationCallbackHelper>(
        [&entry, &test](IFabricAsyncOperationContext * context)
    {
        BOOLEAN inCallbackCompletedSynchronously = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(context);
        if (!inCallbackCompletedSynchronously)
        {
            test.inflightOperationCount_ -= 1;
            FABRIC_SEQUENCE_NUMBER lsn;

            auto hr = test.replicator_->EndReplicate(context, &lsn);
            if (hr == S_OK)
            {
                test.numberOfOperations_ += 1;
            }

            StartReplicatingIfNeeded(entry, test);
        }
    });

    FABRIC_SEQUENCE_NUMBER newLsn;
    ComPointer<IFabricAsyncOperationContext> beginContext;
    HRESULT hr = entry.BeginReplicate(
        test.replicator_,
        callback,
        beginContext,
        newLsn,
        test.parameters_.ValueSizeInBytes);

    if (hr != S_OK)
    {
        test.inflightOperationCount_ -= 1;

        Threadpool::Post(
            [&entry, &test]
        {
            StartReplicatingIfNeeded(entry, test);
        });

        return;
    }

    completedSync = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(beginContext.GetRawPointer());

    if (completedSync)
    {
        --test.inflightOperationCount_;

        FABRIC_SEQUENCE_NUMBER lsn;
        hr = test.replicator_->EndReplicate(beginContext.GetRawPointer(), &lsn);
        if (hr == S_OK)
        {
            ++test.numberOfOperations_;
        }

        Threadpool::Post(
            [&entry, &test]
        {
            StartReplicatingIfNeeded(entry, test);
        });
    }
}

int GetConfigValueInt(
    IFabricConfigurationPackage * configPackage,
    wstring sectionName, 
    wstring parameterName)
{
    LPCWSTR configValue = nullptr;
    BOOLEAN isEncrypted = FALSE;

    ASSERT_IFNOT(
        configPackage->GetValue(sectionName.c_str(), parameterName.c_str(), &isEncrypted, &configValue) == S_OK,
        "Failed to get config value for section {0}, param {1}",
        sectionName,
        parameterName);
    
    wstring value(configValue);

    Trace.WriteInfo(
        TraceSource,
        "Test Parameter {0} = {1}, wstring={2}",
        parameterName,
        configValue,
        value);

    return Int32_Parse(value);
}
