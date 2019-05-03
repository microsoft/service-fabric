// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Client;
using namespace ServiceModel;
using namespace Management::HealthManager;
using namespace Reliability;
using namespace ClientServerTransport;

StringLiteral const ReportTimerTag("HealthClientReportTimer");

Global<HealthClientEventSource> HealthReportingComponent::Trace = make_global<HealthClientEventSource>();

// **********************************************
// Inner class - SequenceStreamMap
// **********************************************
HealthReportingComponent::SequenceStreamMap::SequenceStreamMap()
    : entries_()
{
}

HealthReportingComponent::SequenceStreamMap::~SequenceStreamMap()
{
}

std::vector<HealthReportingComponent::SequenceStreamSPtr> HealthReportingComponent::SequenceStreamMap::GetSequenceStreamList() const
{
    vector<HealthReportingComponent::SequenceStreamSPtr> sequenceStreams;
    for (auto & ss : entries_)
    {
        sequenceStreams.push_back(ss.second);
    }

    return move(sequenceStreams);
}

HealthReportingComponent::SequenceStreamSPtr HealthReportingComponent::SequenceStreamMap::GetSequenceStream(
    SequenceStreamId const & sequenceStreamId) const
{
    auto it = entries_.find(sequenceStreamId);
    if (it == entries_.end())
    {
        return nullptr;
    }

    return it->second;
}

HealthReportingComponent::SequenceStreamSPtr HealthReportingComponent::SequenceStreamMap::GetOrCreateSequenceStream(
    SequenceStreamId && sequenceStreamId,
    __in HealthReportingComponent & owner)
{
    // If the sequence stream is in the map, return it.
    // Otherwise create a new sequence stream.
    // Optimize for the case where the sequence stream doesn't exist.
    auto key = sequenceStreamId;
    auto ss = make_shared<SequenceStream>(owner, move(sequenceStreamId), owner.traceContext_);
    auto insertResult = entries_.insert(make_pair(move(key), move(ss)));
    return insertResult.first->second;
}

// **********************************************
// HealthReportingComponent class
// **********************************************
HealthReportingComponent::HealthReportingComponent(
    __in HealthReportingTransport & transport,
    FabricClientInternalSettingsHolder && settingsHolder,
    std::wstring const & traceContext)
    : ComponentRoot()
    , sequenceStreams_()
    , open_(false)
    , reportTimerSPtr_()
    , scheduledFireTime_(DateTime::MaxValue)
    , lock_()
    , transport_(transport)
    , settingsHolder_(move(settingsHolder))
    , transportRoot_()
    , traceContext_(traceContext)
    , isEnabled_(ClientConfig::GetConfig().IsHealthReportingEnabled)
    , maxNumberOfReports_(ClientConfig::GetConfig().MaxNumberOfHealthReports)
    , currentReportCount_(0)
    , isMessageLimitReached_(false)
    , consecutiveHMBusyCount_(0)
{
    Trace->Ctor(traceContext_);
}

HealthReportingComponent::HealthReportingComponent(
    __in HealthReportingTransport & transport,
    std::wstring const & traceContext,
    bool enableMaxNumberOfReportThrottle)
    : ComponentRoot()
    , sequenceStreams_()
    , open_(false)
    , reportTimerSPtr_()
    , scheduledFireTime_(DateTime::MaxValue)
    , lock_()
    , transport_(transport)
    , settingsHolder_(traceContext)
    , transportRoot_()
    , traceContext_(traceContext)
    , isEnabled_(ClientConfig::GetConfig().IsHealthReportingEnabled)
    , maxNumberOfReports_(enableMaxNumberOfReportThrottle ? ClientConfig::GetConfig().MaxNumberOfHealthReports : numeric_limits<int>::max())
    , currentReportCount_(0)
    , isMessageLimitReached_(false)
    , consecutiveHMBusyCount_(0)
{
    Trace->Ctor(traceContext_);
}

HealthReportingComponent::~HealthReportingComponent()
{
    Trace->Dtor(traceContext_);
    ASSERT_IF(reportTimerSPtr_, "{0}: HealthReportingComponent::~HealthReportingComponent timer is set", traceContext_);
}

ErrorCode HealthReportingComponent::OnOpen()
{
    Trace->Open(traceContext_);

    { // lock
        AcquireWriteLock grab(lock_);
        ASSERT_IF(open_, "{0}: HealthReportingComponent::Open called while open", traceContext_);

        transportRoot_ = transport_.Root.CreateComponentRoot();
        open_ = true;
                
        // Create the timer but do not enable it yet
        auto thisSPtr = this->CreateComponentRoot();
        reportTimerSPtr_ = Timer::Create(
            ReportTimerTag, 
            [thisSPtr, this] (TimerSPtr const &) { this->ReportHealthTimerCallback(); }, 
            true);
    } //endlock

    return ErrorCode::Success();
}

ErrorCode HealthReportingComponent::OnClose()
{
    { // lock
        AcquireWriteLock grab(lock_);
        if (open_)
        {
            open_ = false;

            uint64 pendingReportCount = static_cast<uint64>(currentReportCount_.load());
            if (pendingReportCount == 0)
            {
                Trace->Close(traceContext_);
            }
            else
            {
                Trace->CloseDropReports(traceContext_, pendingReportCount);
            }

            transportRoot_ = nullptr;

            if (reportTimerSPtr_ != nullptr)
            {
                reportTimerSPtr_->Cancel();
                reportTimerSPtr_ = nullptr;
                scheduledFireTime_ = DateTime::MaxValue;
            }

            sequenceStreams_.Clear();
        }
    } //endlock

    return ErrorCode::Success();
}

void HealthReportingComponent::OnAbort()
{
    OnClose().ReadValue();
}

void HealthReportingComponent::UpdateTimerIfSoonerCallerHoldsLock(TimeSpan const dueTime)
{
    if (!open_)
    {
        // Component has been shutdown
        return;
    }

    ASSERT_IFNOT(reportTimerSPtr_, "HealthReportingComponent::SetTimer: timer is not set");

    if (consecutiveHMBusyCount_ > 0)
    {
        // Let timer to existing value, do not move it closer to avoid overwhelming HM
        return;
    }

    DateTime newScheduledTime = DateTime::Now().AddWithMaxValueCheck(dueTime);
    if (newScheduledTime < scheduledFireTime_)
    {
        WriteNoise(Constants::HealthClientReportSource, traceContext_, "Set timer dueTime {0} (previous scheduled {1})", dueTime, scheduledFireTime_);
        reportTimerSPtr_->Change(dueTime);
        scheduledFireTime_ = newScheduledTime;
    }
}

void HealthReportingComponent::ReplaceTimerValueCallerHoldsLock(TimeSpan const dueTime)
{
    if (!open_)
    {
        // Component has been shutdown
        return;
    }

    ASSERT_IFNOT(reportTimerSPtr_, "HealthReportingComponent::SetTimer: timer is not set");

    TimeSpan delay;
    if (consecutiveHMBusyCount_ > 0)
    {
        delay = dueTime.AddWithMaxAndMinValueCheck(this->GetServiceTooBusyBackoff());
    }
    else
    {
        delay = dueTime;
    }

    DateTime newScheduledTime = DateTime::Now().AddWithMaxValueCheck(delay);
    reportTimerSPtr_->Change(delay);
    scheduledFireTime_ = newScheduledTime;
}

void HealthReportingComponent::ReportHealthTimerCallback()
{
    // Get the list of sequence streams using the reader lock
    vector<SequenceStreamSPtr> sequenceStreams;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return;
        }

        sequenceStreams = sequenceStreams_.GetSequenceStreamList();
    } // endlock

    vector<HealthReport> healthReports;
    vector<SequenceStreamInformation> sequenceStreamInfos;
    vector<GetSequenceStreamProgressMessageBody> getProgressMessages;
    ActivityId activityId;
    int maxNumberOfReportsPerMessage = ClientConfig::GetConfig().MaxNumberOfHealthReportsPerMessage;
    int remainingCount = maxNumberOfReportsPerMessage;

    // Look at the sequence streams outside of the lock to determine which messages need to be sent
    for (auto & sequenceStream : sequenceStreams)
    {
        if (sequenceStream->IsPreInitializing)
        {
            getProgressMessages.push_back(GetSequenceStreamProgressMessageBody(sequenceStream->Id, sequenceStream->Instance));
        }
        else if (remainingCount > 0)
        {
            sequenceStream->GetContent(remainingCount, settingsHolder_.Settings->HealthReportRetrySendInterval, healthReports, sequenceStreamInfos);
            remainingCount = maxNumberOfReportsPerMessage - static_cast<int>(healthReports.size());
            ASSERT_IF(
                remainingCount < 0,
                "{0}: ReportHealthTimerCallback remaining count cannot be negative {1}",
                traceContext_,
                remainingCount);
        }
    }

    // Update state under the write lock
    ComponentRootSPtr transportRoot;    
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return;
        }

        transportRoot = transportRoot_;
        isMessageLimitReached_ = (remainingCount == 0);

        ReplaceTimerValueCallerHoldsLock(settingsHolder_.Settings->HealthReportRetrySendInterval);
        
        Trace->Send(
            traceContext_, 
            activityId, 
            getProgressMessages.size(), 
            sequenceStreams_.Size(), 
            sequenceStreamInfos.size(),
            healthReports.size(), 
            currentReportCount_.load(), 
            isMessageLimitReached_, 
            scheduledFireTime_);
    } // endlock
    
    // Send message to HM outside the lock
    for (auto && getProgressMessage : getProgressMessages)
    {
        activityId.IncrementIndex();
        GetSequenceStreamProgress(activityId, move(getProgressMessage), transportRoot);
    }

    if (!healthReports.empty() || !sequenceStreamInfos.empty())
    {
        activityId.IncrementIndex();
        ReportHealth(activityId, ReportHealthMessageBody(move(healthReports), move(sequenceStreamInfos)), transportRoot);
    }
}

void HealthReportingComponent::ReportHealth(
    ActivityId const & activityId,
    ReportHealthMessageBody && messageBody, 
    ComponentRootSPtr const & transportRoot)
{
    auto request = HealthManagerTcpMessage::GetReportHealth(
        activityId,Common::make_unique<ReportHealthMessageBody>(move(messageBody)))->GetTcpMessage();
    request->Headers.Add(Transport::TimeoutHeader(settingsHolder_.Settings->HealthOperationTimeout));

    AsyncOperationSPtr forwardToServiceOperation = transport_.BeginReport(
        move(request),
        settingsHolder_.Settings->HealthOperationTimeout,
        [this](AsyncOperationSPtr const & operation) { this->OnReportHealthCompleted(operation, false); },
        CreateTransportOperationRoot(transportRoot));
    this->OnReportHealthCompleted(forwardToServiceOperation, true);
}

void HealthReportingComponent::OnReportHealthCompleted(
    AsyncOperationSPtr const & asyncOperation,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != asyncOperation->CompletedSynchronously)
    {
        return;
    }

    Transport::MessageUPtr reply;
    ErrorCode errorCode = transport_.EndReport(asyncOperation, reply);
    if (!errorCode.IsSuccess())
    {
        // Do not set timer in this case, remains to the existing value
        Trace->SendFailure(traceContext_, errorCode.ReadValue());
        return;
    }

    ReportHealthReplyMessageBody replyBody;
    if (!reply->GetBody(replyBody))
    {
        Trace->SendFailure(traceContext_, ErrorCodeValue::InvalidMessage);
        return;
    }

    auto activityId = Transport::FabricActivityHeader::FromMessage(*reply).ActivityId;
    
    vector<HealthReportResult> reportResults = replyBody.MoveEntries();
    vector<SequenceStreamResult> sequenceStreamResults = replyBody.MoveSequenceStreamResults();
    
    bool hasServiceTooBusy = false;

    // Look through the sequence streams and keep the ones that are relevant to the reply
    vector<pair<SequenceStreamSPtr, HealthReportResult>> ssReportResults;
    vector<pair<SequenceStreamSPtr, SequenceStreamResult>> ssSequenceStreamResults;

    // Get the list of sequence streams using the reader lock
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return;
        }
        
        // Get health report results per sequence stream
        for (auto && healthReportResult : reportResults)
        {
            if (RepresentsHMBusyError(healthReportResult.Error))
            {
                hasServiceTooBusy = true;
                continue;
            }

            if (IsReportResultErrorNonActionable(healthReportResult.Error))
            {
                // No need to look for sequence stream, since there is nothing to be done.
                // This optimizes taking the lock of the sequence stream without reason.
                Trace->IgnoreReportResultNonActionableError(
                    traceContext_,
                    activityId,
                    healthReportResult.EntityPropertyId,
                    healthReportResult.SequenceNumber,
                    healthReportResult.Error);
                continue;
            }

            if (!sequenceStreams_.TryAppendSequenceStreamWithResult<HealthReportResult>(move(healthReportResult), ssReportResults))
            {
                WriteInfo(
                    Constants::HealthClientReportSource,
                    traceContext_,
                    "{0}: Sequence stream not found for {1}",
                    activityId,
                    healthReportResult);
            }
        }

        // Get sequence stream results per sequence stream
        // Possible improvement: for sequence stream, HM doesn't reply with ServiceTooBusy, it just doesn't process result
        for (auto && sequenceStreamResult : sequenceStreamResults)
        {
            if (!sequenceStreams_.TryAppendSequenceStreamWithResult<SequenceStreamResult>(move(sequenceStreamResult), ssSequenceStreamResults))
            {
                WriteInfo(
                    Constants::HealthClientReportSource,
                    traceContext_,
                    "{0}: Sequence stream not found for {1}",
                    activityId,
                    sequenceStreamResult);
            }
        }

    } // endlock
    
    // Process the results outside of the health reporting component lock
    for (auto & entry : ssReportResults)
    {
        long updateCount = 0;
        entry.first->ProcessReportResult(entry.second, updateCount);
        UpdateReportCount(updateCount);
    }
    
    for (auto & entry : ssSequenceStreamResults)
    {
        entry.first->ProcessSequenceResult(entry.second, activityId, false);
    }

    // Update the health reporting component state under the writer lock
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return;
        }

        if (hasServiceTooBusy)
        {
            ++consecutiveHMBusyCount_;
            ReplaceTimerValueCallerHoldsLock(settingsHolder_.Settings->HealthReportRetrySendInterval);
        }
        else
        {
            consecutiveHMBusyCount_ = 0;
            if (isMessageLimitReached_)
            {
                // Schedule the timer for retry to fire immediately
                UpdateTimerIfSoonerCallerHoldsLock(TimeSpan::Zero);
            }
            else
            {
                UpdateTimerIfSoonerCallerHoldsLock(settingsHolder_.Settings->HealthReportRetrySendInterval);
            }
        }

        Trace->ProcessReply(
            traceContext_,
            activityId,
            sequenceStreams_.Size(),
            currentReportCount_.load(),
            consecutiveHMBusyCount_,
            scheduledFireTime_);
    } // endlock
}

ErrorCode HealthReportingComponent::AddHealthReport(
    HealthReport && report,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    if (!isEnabled_)
    {
        return ErrorCode::Success();
    }

    // Check whether queue full is reached, otherwise tentatively accept the report
    if (!AcceptReportPerMaxCount(report))
    {
        return ErrorCode(ErrorCodeValue::HealthMaxReportsReached);
    }

    // Get the sequence stream if it exists, using the reader lock.
    // Optimize for the case where most of the time the sequence stream exist, since a sequence stream usually sends many reports.
    // If it doesn't exist, create it using the writer lock (so on first call, take reader lock, check, then take writer lock).
    SequenceStreamSPtr sequenceStream;
    SequenceStreamId ssId(report.Kind, report.SourceId);
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetSequenceStream(ssId);
    } // endlock

    if (!sequenceStream)
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetOrCreateSequenceStream(move(ssId), *this);
    } // endlock

    // Process the report outside the lock
    long updateCount = 0;
    bool sendImmediately = sendOptions && sendOptions->Immediate;
    auto error = sequenceStream->ProcessReport(move(report), sendImmediately, updateCount);
    if (error.IsSuccess())
    {
        // If the report was accepted, the updateCount takes it into consideration, but it has been already counted.
        // Update counts with the rest of the changes.
        UpdateReportCount(updateCount - 1);

        { // lock
            AcquireWriteLock grab(lock_);
            if (sendImmediately)
            {
                UpdateTimerIfSoonerCallerHoldsLock(TimeSpan::Zero);
            }
            else
            {
                UpdateTimerIfSoonerCallerHoldsLock(settingsHolder_.Settings->HealthReportSendInterval);
            }
        } // endlock
    }
    else
    {
        // If the report was not accepted (error on processing), the count should decrease for the current report.
        UpdateReportCount(-1);
    }

    return error;
}

ErrorCode HealthReportingComponent::AddHealthReports(
    vector<HealthReport> && reports,
    ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
{
    if (!isEnabled_ || reports.empty())
    {
        return ErrorCode::Success();
    }

    // Check whether queue full is reached, otherwise tentatively accept the report
    long accepted = AcceptReportsPerMaxCount(reports);
    if (accepted <= 0)
    {
        // No report was accepted, no work needed
        return ErrorCode(ErrorCodeValue::HealthMaxReportsReached);
    }

    size_t acceptedCount = static_cast<size_t>(accepted);

    ASSERT_IF(acceptedCount > reports.size(), "{0}: can't accept more reports than provided in input: accepted {1}, total {2}", traceContext_, acceptedCount, reports.size());

    // Get the sequence streams that should process the reports. If they don't exist, create them.
    vector<pair<SequenceStreamSPtr, HealthReport>> sequenceStreams;
    for (size_t i = 0; i < acceptedCount; ++i)
    {
        // First populate with empty sequence streams
        sequenceStreams.push_back(make_pair(nullptr, move(reports[i])));
    }

    // Optimize for the case where most of the time the sequence stream exists, since a sequence stream usually sends many reports.
    // If it doesn't exist, create it using the writer lock (so on first call, take reader lock, check, then take writer lock).
    bool allFound = true;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        for (auto & entry : sequenceStreams)
        {
            SequenceStreamId ssId(entry.second.Kind, entry.second.SourceId);
            auto ss = sequenceStreams_.GetSequenceStream(ssId);
            if (ss)
            {
                entry.first = move(ss);
            }
            else
            {
                allFound = false;
            }
        }
    } // endlock

    if (!allFound)
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        for (auto & entry : sequenceStreams)
        {
            if (!entry.first)
            {
                SequenceStreamId ssId(entry.second.Kind, entry.second.SourceId);
                entry.first = sequenceStreams_.GetOrCreateSequenceStream(move(ssId), *this);
            }
        }
    } // endlock

    // Process the accepted reports outside the lock
    long totalUpdateCount = 0;
    bool reportsAdded = false;
    ErrorCode error(ErrorCodeValue::Success);
    bool sendImmediately = sendOptions && sendOptions->Immediate;
    for (auto & entry : sequenceStreams)
    {
        long updateCount = 0;
        auto innerError = entry.first->ProcessReport(move(entry.second), sendImmediately, updateCount);
        if (innerError.IsSuccess())
        {
            totalUpdateCount += updateCount;
            reportsAdded = true;
        }
        else
        {
            WriteInfo(
                Constants::HealthClientReportSource,
                traceContext_,
                "Error processing report {0}: {1}",
                entry.second,
                innerError);
            error.Overwrite(innerError);
        }
    }

    // The accepted reports were already counted, check whether there are other changes to be done
    UpdateReportCount(totalUpdateCount - accepted);

    if (reportsAdded)
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        if (sendImmediately)
        {
            UpdateTimerIfSoonerCallerHoldsLock(TimeSpan::Zero);
        }
        else
        {
            UpdateTimerIfSoonerCallerHoldsLock(settingsHolder_.Settings->HealthReportSendInterval);
        }

        WriteNoise(
            Constants::HealthClientReportSource,
            traceContext_,
            "Added reports: {0}, scheduledFireTime: {1}",
            currentReportCount_.load(),
            scheduledFireTime_);
    } // endlock

    // If some of the reports were not accepted, return max reports reached error
    if (acceptedCount < reports.size())
    {
        WriteInfo(
            Constants::HealthClientReportSource,
            traceContext_,
            "AddHealthReports: received {0} reports, accepted {1}, rejected the rest with HealthMaxReportsReached",
            reports.size(),
            acceptedCount);
        error.ReadValue();
        return ErrorCode(ErrorCodeValue::HealthMaxReportsReached);
    }

    return error;
}

ErrorCode HealthReportingComponent::HealthPreInitialize(wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_INSTANCE_ID instance)
{
    if (!isEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    SequenceStreamId ssId(kind, sourceId);
    GetSequenceStreamProgressMessageBody messageBody(ssId, instance);

    // Get the sequence stream or create it if it doesn't exist.
    // Since the preinitialize is called at the beginning, usually the stream doesn't exist.
    // Take the writer lock to optimize for this case.
    SequenceStreamSPtr sequenceStream;
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetOrCreateSequenceStream(move(ssId), *this);
    } // endlock

    long updateCount = 0;
    auto error = sequenceStream->PreInitialize(instance, updateCount);
    if (!error.IsSuccess())
    {
        return error;
    }

    UpdateReportCount(updateCount);

    ComponentRootSPtr transportRoot;
    { // lock
        AcquireWriteLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        transportRoot = transportRoot_;
        UpdateTimerIfSoonerCallerHoldsLock(settingsHolder_.Settings->HealthReportSendInterval);
    } // endlock

    ActivityId activityId;
    GetSequenceStreamProgress(activityId, move(messageBody), transportRoot);

    return ErrorCode::Success();
}

ErrorCode HealthReportingComponent::HealthPostInitialize(wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_SEQUENCE_NUMBER sequence, FABRIC_SEQUENCE_NUMBER invalidateSequence)
{
    if (!isEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    SequenceStreamSPtr sequenceStream;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetSequenceStream(SequenceStreamId(kind, sourceId));
    } // endlock

    ASSERT_IF(sequenceStream == nullptr, "HealthReportingComponent::HealthPostInitialize sequenceStream cannot be null");
    return sequenceStream->PostInitialize(sequence, invalidateSequence);
}

ErrorCode HealthReportingComponent::HealthGetProgress(
    wstring const & sourceId,
    ::FABRIC_HEALTH_REPORT_KIND kind,
    __out FABRIC_SEQUENCE_NUMBER & progress)
{
    if (!isEnabled_)
    {
        progress = 0;
        return ErrorCodeValue::Success;
    }

    SequenceStreamSPtr sequenceStream;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetSequenceStream(SequenceStreamId(kind, sourceId));
    } // endlock

    ASSERT_IF(sequenceStream == nullptr, "{0}: HealthGetProgress sequenceStream cannot be null", traceContext_);
    return sequenceStream->GetProgress(progress);
}

void HealthReportingComponent::GetSequenceStreamProgress(
    ActivityId const & activityId,
    GetSequenceStreamProgressMessageBody && messageBody, 
    ComponentRootSPtr const & transportRoot)
{
    AsyncOperationSPtr forwardToServiceOperation = transport_.BeginReport(
        HealthManagerTcpMessage::GetSequenceStreamProgressRequest(Common::make_unique<GetSequenceStreamProgressMessageBody>(move(messageBody)), activityId)->GetTcpMessage(),
        settingsHolder_.Settings->HealthOperationTimeout,
        [this, activityId](AsyncOperationSPtr const & operation) { this->OnGetSequenceStreamProgressCompleted(operation, activityId, false); },
        this->CreateTransportOperationRoot(transportRoot));
    this->OnGetSequenceStreamProgressCompleted(forwardToServiceOperation, activityId, true);
}

void HealthReportingComponent::OnGetSequenceStreamProgressCompleted(
    AsyncOperationSPtr const & asyncOperation, 
    ActivityId const & activityId, 
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != asyncOperation->CompletedSynchronously)
    {
        return;
    }

    Transport::MessageUPtr reply;
    auto error = transport_.EndReport(asyncOperation, reply);
    if (!error.IsSuccess())
    {
        WriteWarning(Constants::HealthClientSequenceStreamSource, traceContext_, "{0}: GetSequenceStreamProgress failed with {1}", activityId, error);
        return;
    }

    GetSequenceStreamProgressReplyMessageBody replyBody;
    if (!reply->GetBody(replyBody))
    {
        WriteWarning(Constants::HealthClientSequenceStreamSource, traceContext_, "{0}: GetSequenceStreamProgress failed with deserialization", activityId);
        return;
    }

    SequenceStreamResult sequenceStreamResult = replyBody.MoveSequenceStreamResult();

    SequenceStreamSPtr sequenceStream;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return;
        }

        sequenceStream = sequenceStreams_.GetSequenceStream(SequenceStreamId(sequenceStreamResult.Kind, sequenceStreamResult.SourceId));
    } // endlock

    if (sequenceStream != nullptr)
    {
        sequenceStream->ProcessSequenceResult(sequenceStreamResult, activityId, true);
    }
}

ErrorCode HealthReportingComponent::HealthSkipSequence(std::wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_SEQUENCE_NUMBER sequenceToSkip)
{
    if (!isEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    SequenceStreamSPtr sequenceStream;
    { // lock
        AcquireReadLock grab(lock_);
        if (!open_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        sequenceStream = sequenceStreams_.GetSequenceStream(SequenceStreamId(kind, sourceId));
    } // endlock

    ASSERT_IF(sequenceStream == nullptr, "HealthReportingComponent::HealthGetProgress sequenceStream cannot be null");
    return sequenceStream->HealthSkipSequence(sequenceToSkip);
}

void HealthReportingComponent::UpdateReportCount(long updateCount)
{
    if (updateCount != 0)
    {
        long previousCount = currentReportCount_.fetch_add(updateCount);
        ASSERT_IF(previousCount < 0, "HealthReportingComponent::UpdateReportCountCallerHoldsLock previousCount cannot be negative {0}", previousCount);
        ASSERT_IF(previousCount + updateCount < 0, "HealthReportingComponent::UpdateReportCountCallerHoldsLock reportcount cannot be negative {0}", previousCount + updateCount);
    }
}

bool HealthReportingComponent::AcceptReportPerMaxCount(ServiceModel::HealthReport const & report)
{
    long previousCount = currentReportCount_++;
    if (previousCount >= maxNumberOfReports_)
    {
        // Reached limit, the report can't be accepted, so revert the count
        --currentReportCount_;
        Trace->Reject(traceContext_, previousCount, report.SourceId, report.Property);
        return false;
    }
     
    // Accept report
    return true;
}

long HealthReportingComponent::AcceptReportsPerMaxCount(std::vector<ServiceModel::HealthReport> const & reports)
{
    // Optimize for the case where there is enough room to add reports because queue is not full
    long acceptedCount = 0;
    for (auto const & report : reports)
    {
        if (AcceptReportPerMaxCount(report))
        {
            ++acceptedCount;
        }
        else
        {
            break;
        }
    }

    return acceptedCount;
}

Common::AsyncOperationSPtr HealthReportingComponent::CreateTransportOperationRoot(
    Common::ComponentRootSPtr const & transportRoot) const
{
    std::vector<ComponentRootSPtr> roots;
    roots.push_back(transportRoot);
    roots.push_back(this->CreateComponentRoot());
    return CreateAsyncOperationMultiRoot(std::move(roots));
}

Common::TimeSpan HealthReportingComponent::GetServiceTooBusyBackoff() const
{
    int64 delaySec = consecutiveHMBusyCount_ * ClientConfig::GetConfig().HealthReportSendBackOffStepInSeconds;
    if (delaySec > ClientConfig::GetConfig().HealthReportSendMaxBackOffInterval.TotalSeconds())
    {
        return ClientConfig::GetConfig().HealthReportSendMaxBackOffInterval;
    }
    else
    {
        return TimeSpan::FromSeconds(static_cast<double>(delaySec));
    }
}

bool HealthReportingComponent::IsReportResultErrorNonActionable(Common::ErrorCodeValue::Enum const errorCodeValue)
{
    switch (errorCodeValue)
    {
    // HM returns HealthCheckPending when it received the same report twice.
    // It is processing one report, while the copy gets this error code.
    // Health client should process the result for the executing report, while HealthCheckPending is not actionable.
    case ErrorCodeValue::HealthCheckPending:
        return true;

    default:
        return false;
    }
}

bool HealthReportingComponent::IsRetryableError(ErrorCodeValue::Enum const errorCodeValue)
{
    switch (errorCodeValue)
    {
    case ErrorCodeValue::StaleRequest:
    case ErrorCodeValue::HealthEntityDeleted:
    case ErrorCodeValue::HealthStaleReport:
    case ErrorCodeValue::InvalidArgument:
    case ErrorCodeValue::AccessDenied:
        return false;

    default:
        return true;
    }
}

bool HealthReportingComponent::RepresentsHMBusyError(ErrorCodeValue::Enum const errorCodeValue)
{
    switch (errorCodeValue)
    {
    case ErrorCodeValue::ServiceTooBusy:
    case ErrorCodeValue::REQueueFull:

    // The message was processed successfully by HM, but this report wasn't processed in time.
    // This means that HM is experiencing some delays.
    case ErrorCodeValue::Timeout:
        return true;

    default:
        return false;
    }
}

int HealthReportingComponent::Test_GetCurrentReportCount(__out int & consecutiveServiceTooBusyCount) const
{
    { // lock
        AcquireReadLock grab(lock_);
        consecutiveServiceTooBusyCount = consecutiveHMBusyCount_;
    } //endlock

    return currentReportCount_.load();
}
