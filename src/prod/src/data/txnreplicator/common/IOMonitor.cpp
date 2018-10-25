// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace Common;

Common::StringLiteral const TraceComponent("IOMonitor");

IOMonitor::IOMonitor(
    __in PartitionedReplicaId const & traceId,
    __in std::wstring const & operationName,
    __in Common::SystemHealthReportCode::Enum healthReportCode,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , operationName_(operationName)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , slowOperationTimestamps_(GetThisAllocator())
    , lastSlowHealthReportTimestamp_()
    , currentSlowOpIndex_(0)
    , healthClient_(healthClient)
    , healthReportCode_(healthReportCode)
{
    if (IsValidConfiguration())
    {
        NTSTATUS status = InitializeIfNecessary();

        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }
    }

    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"IOMonitor",
        reinterpret_cast<uintptr_t>(this));
}

IOMonitor::~IOMonitor()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"IOMonitor",
        reinterpret_cast<uintptr_t>(this));
}

IOMonitor::SPtr IOMonitor::Create(
    __in PartitionedReplicaId const & traceId,
    __in std::wstring const & operationName,
    __in Common::SystemHealthReportCode::Enum healthReportCode,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in KAllocator & allocator)
{
    IOMonitor * pointer = _new(LOGIOTRACKER_TAG, allocator)IOMonitor(
        traceId,
        operationName,
        healthReportCode,
        transactionalReplicatorConfig,
        healthClient);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    if (!NT_SUCCESS(pointer->Status()))
    {
        throw ktl::Exception(pointer->Status());
    }

    return IOMonitor::SPtr(pointer);
}

TimeSpan IOMonitor::ComputeRefreshTime(__in Common::TimeSpan const & timeThreshold)
{
    return TimeSpan::FromMilliseconds(static_cast<double>(timeThreshold.TotalPositiveMilliseconds()) / 2);
}

void IOMonitor::OnSlowOperation()
{
    // Update dynamic configs if necessary
    if (!IsValidConfiguration())
    {
        // No-op validation failure
        return;
    }

    NTSTATUS status = InitializeIfNecessary();
    if (!NT_SUCCESS(status))
    {
        // No-op initialization failure
        return;
    }

    StopwatchTime currentTimeStamp = Stopwatch::Now();
    slowOperationTimestamps_[currentSlowOpIndex_] = currentTimeStamp;

    // Increment the current index
    // This returns the lowest value in the array of timestamps
    ULONG lowIndex = currentSlowOpIndex_;

    if (++lowIndex >= slowOperationTimestamps_.Count())
    {
        lowIndex = 0;
    }

    // Check if enough time has passed since the last health report
    if (currentTimeStamp - lastSlowHealthReportTimestamp_ > refreshHealthReportTimeThreshold_)
    {
        // Retrieve the oldest slow commit timestamp
        // If there is no low timestamp return a default value
        StopwatchTime lowTimestamp = slowOperationTimestamps_[lowIndex];
        TimeSpan range = currentTimeStamp - lowTimestamp;

        // Report health 
        if (range <= transactionalReplicatorConfig_->SlowLogIOTimeThreshold)
        {
            lastSlowHealthReportTimestamp_ = currentTimeStamp;

            std::wstring description = wformatString(
                L"Greater than {0} slow reports from {1} to {2}",
                transactionalReplicatorConfig_->SlowLogIOCountThreshold,
                lowTimestamp,
                currentTimeStamp);

            Common::ErrorCode error = healthClient_->ReportReplicatorHealth(
                SystemHealthReportCode::TR_SlowIO,
                operationName_,
                description,
                Common::SequenceNumber::GetNext(),
                transactionalReplicatorConfig_->SlowLogIOHealthReportTTL);

            TREventSource::Events->TRHealthTrackerWarning(
                TracePartitionId,
                ReplicaId,
                operationName_,
                description,
                error);
        }
    }

    // Update the current index
    currentSlowOpIndex_ = lowIndex;
}

bool IOMonitor::IsValidConfiguration()
{
    if (transactionalReplicatorConfig_->SlowLogIOCountThreshold == 0)
    {
        // Do not initialize if count is set to zero
        return false;;
    }

    if (healthClient_ == nullptr)
    {
        // System service components may pass a nullptr reference for IReplicatorHealthClientSPtr
        // Fail validation to ensure health report is not attempted
        return false;
    }

    return true;
}

NTSTATUS IOMonitor::InitializeIfNecessary()
{
    refreshHealthReportTimeThreshold_ = ComputeRefreshTime(transactionalReplicatorConfig_->SlowLogIOTimeThreshold);

    // Dynamic config change of count threshold is supported
    if (slowOperationTimestamps_.Max() != transactionalReplicatorConfig_->SlowLogIOCountThreshold)
    {
        // Clear the existing array
        slowOperationTimestamps_ = KArray<StopwatchTime>(
            GetThisAllocator(),
            (ULONG)(transactionalReplicatorConfig_->SlowLogIOCountThreshold),
            (ULONG)(transactionalReplicatorConfig_->SlowLogIOCountThreshold),
            0);

        NTSTATUS status = slowOperationTimestamps_.Status();

        if (NT_SUCCESS(status) == false)
        {
            return status;
        }

        // Reset the current index
        currentSlowOpIndex_ = 0;
    }

    return STATUS_SUCCESS;
}

void IOMonitor::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "Index=" << currentSlowOpIndex_ << " ";
    w << "LastReport=" << lastSlowHealthReportTimestamp_ << " ";
    w << "Timestamps=[";
    for (StopwatchTime const & timestamp : slowOperationTimestamps_)
    {
        w << timestamp << " ";
    }
    w << "]";
    w << "\n Count Threshold:  " << transactionalReplicatorConfig_->SlowLogIOCountThreshold << " Time Threshold: " << transactionalReplicatorConfig_->SlowLogIOTimeThreshold << " RefreshHealthReportThreshold: " << refreshHealthReportTimeThreshold_;
}
