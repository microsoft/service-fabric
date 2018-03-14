// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireExclusiveLock;
using Common::AcquireReadLock;
using Common::Assert;
using Common::RwLock;
using Common::StringWriter;
using Common::Timer;
using Common::TimerSPtr;
using Common::TimeSpan;
using std::wstring;
using std::move;

BatchedHealthReporter::BatchedHealthReporter(
    Common::Guid const & partitionId, 
    ReplicationEndpointId const & endpointId,
    HealthReportType::Enum reportType,
    TimeSpan const & cacheInterval,
    IReplicatorHealthClientSPtr const & healthClient)
    : ComponentRoot()
    , reportType_(reportType)
    , healthClient_(healthClient)
    , timerActive_(false)
    , timerInterval_(cacheInterval)
    , lock_()
    , timer_()
    , lastReportedHealthState_(FABRIC_HEALTH_STATE_OK)
    , latestHealthState_(FABRIC_HEALTH_STATE_OK)
    , isActive_(false)
    , latestDescription_()
    , partitionId_(partitionId)
    , endpointUniqueId_(endpointId)
{
    ReplicatorEventSource::Events->BatchedHealthReporterCtor(
        partitionId_,
        endpointUniqueId_,
        reportType_,
        timerInterval_);
}

BatchedHealthReporter::~BatchedHealthReporter()
{
    AcquireExclusiveLock lock(lock_);
    ASSERT_IF(
        isActive_,
        "{0}: BatchedHealthReporter.dtor: isActive is true, Close should have been called",
        endpointUniqueId_);
    ASSERT_IF(
        timer_,
        "{0}: BatchedHealthReporter.dtor: Timer not null, Close should have been called",
        endpointUniqueId_);
}

void BatchedHealthReporter::Open()
{
    ASSERT_IF(timerInterval_ == TimeSpan::Zero, "{0}: BatchedHealthReporter.Open -  sender cache time must be non-zeo", endpointUniqueId_);

    AcquireExclusiveLock grab(lock_);
    auto root = this->CreateComponentRoot();
    timer_ = Timer::Create(
        "ReplicatorBatchedHealthReporter",
        [this, root](TimerSPtr const &)
        {
            this->OnTimerCallback();
        },
        true);

    isActive_ = true;

    ASSERT_IFNOT(timer_, "{0}: BatchedHealthReporter.Open()- Health Report Sender failed to create timer", endpointUniqueId_);
}

void BatchedHealthReporter::Close()
{
    bool shouldReportOK = false;

    {
        AcquireExclusiveLock grab(lock_);
        if (!isActive_)
        {
            return;
        }
        
        
        isActive_ = false;
        timer_->Cancel();
        timer_.reset();
        timerActive_ = false;

        // Report must be cleared only if reported
        shouldReportOK = (lastReportedHealthState_ != FABRIC_HEALTH_STATE_OK);

        // Reset health states if reported health state or latest state are unhealthy
        if(shouldReportOK || latestHealthState_ != FABRIC_HEALTH_STATE_OK)
        {
            latestHealthState_ = ::FABRIC_HEALTH_STATE_OK;
            lastReportedHealthState_ = ::FABRIC_HEALTH_STATE_OK;
        }
    }

    ReplicatorEventSource::Events->BatchedHealthReporterClose(
        partitionId_,
        endpointUniqueId_,
        reportType_);

    // Clear any active warnings outside the lock if latest health report was not OK
    if (shouldReportOK)
    {
        SendReport(
            partitionId_,
            endpointUniqueId_,
            reportType_,
            ::FABRIC_HEALTH_STATE_OK,
            L"",
            Common::SequenceNumber::GetNext(),
            healthClient_);
    }
}

void BatchedHealthReporter::ScheduleOKReport()
{
    AcquireExclusiveLock grab(lock_);
    ScheduleReportCallerHoldsLock(::FABRIC_HEALTH_STATE_OK, L"");
}

void BatchedHealthReporter::ScheduleWarningReport(wstring const & description)
{
    AcquireExclusiveLock grab(lock_);
    ScheduleReportCallerHoldsLock(::FABRIC_HEALTH_STATE_WARNING, description);
}

void BatchedHealthReporter::ScheduleReportCallerHoldsLock(
    ::FABRIC_HEALTH_STATE incomingReport,
    wstring const & description)
{
    // If we are closed or not enabled, do nothing
    if (!isActive_)
    {
        return;
    }
    
    // Update the description to have the latest information in the health report
    latestDescription_ = description;
    NextAction nextAction = GetNextAction(lastReportedHealthState_, latestHealthState_, incomingReport);

    switch (nextAction)
    {
    case NextAction::None:
        return;
    case NextAction::StartTimer:
        timer_->Change(timerInterval_);
        timerActive_ = true;
        break;
    case NextAction::CancelTimer:
        timer_->Change(TimeSpan::MaxValue);
        timerActive_ = false;
        break;
    }

    ReplicatorEventSource::Events->BatchedHealthReporterStateChange(
        partitionId_,
        endpointUniqueId_,
        reportType_,
        Common::wformatString(latestHealthState_),
        Common::wformatString(incomingReport));

    ASSERT_IF(nextAction == NextAction::None, "{0}: BatchedHealthReporter.ScheduleReportCallerHoldsLock - next action cannot be None", endpointUniqueId_);
    latestHealthState_ = incomingReport;
}

void BatchedHealthReporter::OnTimerCallback()
{
    ::FABRIC_HEALTH_STATE toReport = FABRIC_HEALTH_STATE_UNKNOWN;
    wstring description;

    {
        AcquireExclusiveLock grab(lock_);

        if (!isActive_)
        {
            return;
        }

        timerActive_ = false;
        
        toReport = latestHealthState_;
        lastReportedHealthState_ = latestHealthState_;
        description = latestDescription_;
    }

    ASSERT_IFNOT(toReport == ::FABRIC_HEALTH_STATE_WARNING || toReport == ::FABRIC_HEALTH_STATE_OK, "{0}: Replicator Health Report Sender invalid state", endpointUniqueId_);

    SendReport(
        partitionId_, 
        endpointUniqueId_, 
        reportType_, 
        toReport, 
        description,
        Common::SequenceNumber::GetNext(),    
        healthClient_);
}

BatchedHealthReporterSPtr BatchedHealthReporter::Create(
    Common::Guid const & partitionId, 
    ReplicationEndpointId const & endpointId,
    HealthReportType::Enum reportType,
    TimeSpan const & cacheTime,
    IReplicatorHealthClientSPtr const & healthClient)
{
    auto created = std::shared_ptr<BatchedHealthReporter>(new BatchedHealthReporter(partitionId, endpointId, reportType, cacheTime, healthClient));
    
    if (cacheTime != TimeSpan::Zero)
    {
        created->Open();
    }

    return created;
}

BatchedHealthReporter::NextAction BatchedHealthReporter::GetNextAction(
    ::FABRIC_HEALTH_STATE lastReportedHealthState,
    ::FABRIC_HEALTH_STATE latestHealthState,
    ::FABRIC_HEALTH_STATE incomingReport)
{
    // The order of these checks are significant

    if (incomingReport == latestHealthState)
    {
        return NextAction::None;
    }
    
    if (incomingReport == lastReportedHealthState)
    {
        return NextAction::CancelTimer;
    }

    return NextAction::StartTimer;
}

void BatchedHealthReporter::SendReport(
    Common::Guid const & partitionId,
    ReplicationEndpointId const & endpoint,
    HealthReportType::Enum reportType,
    ::FABRIC_HEALTH_STATE toReport,
    std::wstring const & extraDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    IReplicatorHealthClientSPtr & healthClient)
{
    wstring dynamicProperty = HealthReportType::GetHealthPropertyName(reportType);
    Common::SystemHealthReportCode::Enum reportCode = HealthReportType::GetHealthReportCode(toReport, reportType);
    TimeSpan ttl = HealthReportType::GetHealthReportTTL(toReport, reportType);
    
    if (toReport == FABRIC_HEALTH_STATE_OK)
    {
        ReplicatorEventSource::Events->BatchedHealthReporterSend(
            partitionId,
            endpoint,
            Common::wformatString(toReport),
            sequenceNumber,
            dynamicProperty,
            extraDescription);
    }
    else 
    {
        ReplicatorEventSource::Events->BatchedHealthReporterSendWarning(
            partitionId,
            endpoint,
            Common::wformatString(toReport),
            sequenceNumber,
            dynamicProperty,
            extraDescription);
    }

    ASSERT_IF(
        healthClient == nullptr,
        "{0}: BatchedHealthReporter.SendReport: HealthClient is null",
        endpoint);

    auto error = healthClient->ReportReplicatorHealth(
        reportCode,
        dynamicProperty,
        extraDescription,
        sequenceNumber,
        ttl);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
