// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("HealthTracker");

ReplicatedStore::HealthTracker::HealthTracker(
    __in ReplicatedStore & store,
    ComPointer<IFabricStatefulServicePartition3> && healthReporter)
    : PartitionedReplicaTraceComponent(store.PartitionedReplicaId)
    , replicatedStore_(store)
    , healthReporter_(move(healthReporter))
    , slowCommitLock_()
    , slowCommitTimestamps_()
    , currentSlowCommitIndex_(0)
    , lastSlowCommitReportTimestamp_()
{
    this->InitializeTimestamps(StoreConfig::GetConfig().SlowCommitCountThreshold);
}

void ReplicatedStore::HealthTracker::InitializeTimestamps(int targetThreshold)
{
    slowCommitTimestamps_.clear();
    currentSlowCommitIndex_ = 0;

    if (targetThreshold > 0)
    {
        slowCommitTimestamps_.resize(targetThreshold);
    }
}

ErrorCode ReplicatedStore::HealthTracker::Create(
    __in ReplicatedStore & owner,
    ComPointer<IFabricStatefulServicePartition> const & partitionCPtr,
    __out shared_ptr<HealthTracker> & result)
{
    ComPointer<IFabricStatefulServicePartition3> derivedCPtr;
    auto hr = partitionCPtr->QueryInterface(IID_IFabricStatefulServicePartition3, derivedCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        owner.WriteWarning(
            TraceComponent,
            "{0} QueryInterface(IID_IFabricStatefulServicePartition3) failed: {1}",
            owner.TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    result = shared_ptr<HealthTracker>(new HealthTracker(owner, move(derivedCPtr)));

    return ErrorCodeValue::Success;
}

// A slow commit report is emitted if at least SlowCommitCountThreshold slow commits
// were observed within the last SlowCommitTimeThreshold. The TTL of the health
// report is also SlowCommitTimeThreshold and will be refreshed every SlowCommitTimeThreshold / 2
// if the above condition for slow health reports remains satisfied.
//
void ReplicatedStore::HealthTracker::OnSlowCommit()
{
    auto targetCountThreshold = StoreConfig::GetConfig().SlowCommitCountThreshold;

    AcquireWriteLock lock(slowCommitLock_);

    // Disabled via config
    //
    if (slowCommitTimestamps_.empty() && targetCountThreshold <= 0)
    {
        return;
    }

    // Dynamic config change of count threshold is supported (re-initialize lazily).
    //
    if (slowCommitTimestamps_.size() != targetCountThreshold)
    {
        this->InitializeTimestamps(targetCountThreshold);

        if (slowCommitTimestamps_.empty())
        {
            return;
        }
    }

    auto currentTimestamp = DateTime::Now();
    slowCommitTimestamps_[currentSlowCommitIndex_] = currentTimestamp;

    auto lowIndex = currentSlowCommitIndex_;
    if (++lowIndex >= slowCommitTimestamps_.size()) { lowIndex = 0; }

    auto slowCommitThreshold = StoreConfig::GetConfig().SlowCommitTimeThreshold;
    auto refreshHealthReportThreshold = this->ComputeRefreshTime(slowCommitThreshold);

    if (currentTimestamp.SubtractWithMaxValueCheck(lastSlowCommitReportTimestamp_) > refreshHealthReportThreshold)
    {
        auto lowTimestamp = slowCommitTimestamps_[lowIndex];
        auto range = currentTimestamp.SubtractWithMaxValueCheck(lowTimestamp);

        if (range <= slowCommitThreshold)
        {
            auto description = wformatString(GET_STORE_RC(Slow_Commit), slowCommitTimestamps_.size(), lowTimestamp, currentTimestamp);

            ScopedHeap heap;
            FABRIC_HEALTH_INFORMATION const * healthInformation;

            auto error = this->CreateHealthInformation(
                SystemHealthReportCode::REStore_SlowCommit, 
                description,
                StoreConfig::GetConfig().SlowCommitTimeThreshold,
                heap,
                &healthInformation);

            if (error.IsSuccess())
            {
                error = ErrorCode::FromHResult(healthReporter_->ReportReplicaHealth(healthInformation));
            }

            if (error.IsSuccess())
            {
                lastSlowCommitReportTimestamp_ = currentTimestamp;

                WriteInfo(
                    TraceComponent,
                    "{0}: reported slow commit health: time={1} [{2} ({3}), {4} ({5})] range={6} threshold={7}",
                    this->TraceId,
                    currentTimestamp,
                    lowIndex,
                    lowTimestamp,
                    currentSlowCommitIndex_,
                    currentTimestamp,
                    range,
                    slowCommitThreshold);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to report slow commit health: time={1} [{2} ({3}), {4} ({5})] range={6} threshold={7} error={8}",
                    this->TraceId,
                    currentTimestamp,
                    lowIndex,
                    lowTimestamp,
                    currentSlowCommitIndex_,
                    currentTimestamp,
                    range,
                    slowCommitThreshold,
                    error);
            }
        }
    }

    currentSlowCommitIndex_ = lowIndex;
}

TimeSpan ReplicatedStore::HealthTracker::ComputeRefreshTime(TimeSpan const threshold)
{
    return TimeSpan::FromMilliseconds(static_cast<double>(threshold.TotalPositiveMilliseconds()) / 2);
}

void ReplicatedStore::HealthTracker::ReportErrorDetails(ErrorCode const & error)
{
    this->ReportHealth(
        ToHealthReportCode(error),
        error.Message,
        StoreConfig::GetConfig().DefaultHealthReportTimeToLive);
}

void ReplicatedStore::HealthTracker::ReportHealth(
    SystemHealthReportCode::Enum healthCode,
    wstring const & description,
    TimeSpan const ttl)
{
    ScopedHeap heap;
    FABRIC_HEALTH_INFORMATION const * healthInformation;

    auto error = this->CreateHealthInformation(
        healthCode,
        description,
        ttl,
        heap,
        &healthInformation);

    if (error.IsSuccess())
    {
        error = ErrorCode::FromHResult(healthReporter_->ReportReplicaHealth(healthInformation));
    }

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: reported health: code={1} details={2} ttl={3}",
            this->TraceId,
            healthCode,
            description,
            ttl);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0}: failed to report health: code={1} details={2} ttl={3} error={4}",
            this->TraceId,
            healthCode,
            description,
            ttl,
            error);
    }
}

SystemHealthReportCode::Enum ReplicatedStore::HealthTracker::ToHealthReportCode(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::PathTooLong:
        return SystemHealthReportCode::REStore_PathTooLong;
    default:
        return SystemHealthReportCode::REStore_Unexpected;
    }
}

void ReplicatedStore::HealthTracker::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w << "index=" << currentSlowCommitIndex_ << " ";
    w << "lastReport=" << lastSlowCommitReportTimestamp_ << " ";
    w << "timestamps=[";
    for (auto const & timestamp : slowCommitTimestamps_)
    {
        w << timestamp << "Z ";
    }
    w << "]";
}

ErrorCode ReplicatedStore::HealthTracker::CreateHealthInformation(
    SystemHealthReportCode::Enum reportCode,
    wstring const & description,
    TimeSpan const ttl,
    __in ScopedHeap & heap,
    __out FABRIC_HEALTH_INFORMATION const ** healthInformationResult)
{
    auto healthInfo = EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
        const_cast<Guid&>(this->PartitionId),
        const_cast<FABRIC_REPLICA_ID&>(this->ReplicaId),
        FABRIC_INVALID_REPLICA_ID);

    auto internalReport = HealthReport::CreateSystemHealthReport(
        reportCode,
        move(healthInfo),
        L"", // dynamic property
        description, // description
        Common::SequenceNumber::GetNext(), // sequence number
        ttl,
        AttributeList());

    auto publicReport = heap.AddItem<FABRIC_HEALTH_REPORT>();
    auto error = internalReport.ToPublicApi(heap, *publicReport);
    if (!error.IsSuccess())
    {
         return error;
    }

    if (publicReport->Kind != FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
    {
        WriteWarning(
            TraceComponent,
            "{0}: ToPublicApi created unexpected health report kind: {1}",
            this->TraceId,
            static_cast<int>(publicReport->Kind));

        return ErrorCodeValue::OperationFailed;
    }

    *healthInformationResult = reinterpret_cast<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT*>(publicReport->Value)->HealthInformation;

    return ErrorCodeValue::Success;
}
