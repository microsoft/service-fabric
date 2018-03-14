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

namespace Client
{
    namespace SequenceStreamState
    {
        void WriteToTextWriter(Common::TextWriter & w, SequenceStreamState::Enum const & val)
        {
            switch (val)
            {
            case Uninitialized: w << "Uninitialized"; return;
            case PreInitializing: w << "PreInitializing"; return;
            case PreInitialized: w << "PreInitialized"; return;
            case Initialized: w << "Initialized"; return;
            default:
                w << wformatString("Unknown Sequence Stream State {0}", static_cast<int>(val)); return;
            }
        }

        ENUM_STRUCTURED_TRACE(SequenceStreamState, Uninitialized, LAST_STATE);
    }
}

// ==============================================
// Inner class definition
// Health reports map used by the HealthReportingClient.SequenceStream to hold client reports.
// ==============================================
class HealthReportingComponent::SequenceStream::HealthReportsMap
{
    DENY_COPY(HealthReportsMap)
public:
    HealthReportsMap(
        std::wstring const & traceContext,
        std::wstring const & ownerId);
    ~HealthReportsMap();

    __declspec(property(get = get_ReportsSize)) size_t ReportsSize;
    size_t get_ReportsSize() const { return reportsSerializationSize_; }

    void SetUseSequenceStreamProtocol();

    void Clear();
    size_t Size() const { return reports_.size(); }

    Common::ErrorCode ProcessReport(
        ServiceModel::HealthReport && report,
        __inout HealthReportWrapperPtr & insertedReport,
        __out long & updateCount);

    Common::ErrorCode ProcessReportResult(
        Management::HealthManager::HealthReportResult const & reportResult,
        __out long & updateCount);

    void GetContent(
        size_t maxNumberOfReports,
        Common::TimeSpan const & sendRetryInterval,
        __inout std::vector<ServiceModel::HealthReport> & reports) const;

    void GetSortedContent(
        size_t maxNumberOfReports,
        FABRIC_SEQUENCE_NUMBER lastAcceptedSN,
        __out std::vector<ServiceModel::HealthReport> & reports,
        __out size_t & currentReportCount,
        __out FABRIC_SEQUENCE_NUMBER & lastLsn) const;
    
private:
    using ReportsIterator = std::map<std::wstring, HealthReportWrapperUPtr>::const_iterator;

    static size_t EstimateHealthReportMapEntrySize(HealthReportWrapper const & report);

    Common::ErrorCode CanAcceptReport(
        ServiceModel::HealthReport const & report,
        __out bool & found) const;

    Common::ErrorCode GetReport(
        std::wstring const & reportEntityPropertyId,
        __inout ReportsIterator & reportsIter) const;

    void AddSortedHealthReport(__in HealthReportWrapperPtr const reportPtr);
    void RemoveSortedHealthReport(__in HealthReportWrapperPtr const reportPtr);

    void RemoveEntityHealthReport(__in std::vector<HealthReportWrapperPtr> & entityReports, __in HealthReportWrapperPtr const report);
    void ReplaceEntityHealthReport(__in std::vector<HealthReportWrapperPtr> & entityReports, __in HealthReportWrapperPtr const report, __in HealthReportWrapperPtr const newReport);

    void CleanEntityReports(__in std::vector<HealthReportWrapperPtr> & entityReports, __out long & updateCount);
    void RemoveReport(HealthReportsMap::ReportsIterator reportsIter, __in std::vector<HealthReportWrapperPtr> & entityReports, __out long & updateCount);

private:
    // References kept alive by the parent sequence stream.
    std::wstring const & traceContext_;
    std::wstring const & ownerId_;

    // Map with reports per report Id.
    // Used for easy retrieval of reports based on report id, for example based on HealthReportResult information.
    std::map<std::wstring, HealthReportWrapperUPtr> reports_;

    // Health reports sorted by SequenceNumber.
    // The reports are pointers to actual reports kept alive in the reports map.
    HealthReportSortedList sortedHealthReports_;

    // Health reports per entity, sorted by SequenceNumber.
    // Used to search first by entity, for example when adding reports. Needed to determine whether the entity is stale.
    // Previous implementation kept a HealthReportSortedList. However, the expected number of reports in this list is expected to be small,
    // and a simple vector has better performance at this size.
    using EntityReportsMap = std::map<ServiceModel::EntityHealthInformationSPtr, std::vector<HealthReportWrapperPtr>, ServiceModel::EntityHealthInformation::LessThanComparitor>;
    EntityReportsMap entityHealthReports_;

    // If set, the health client must use SS protocol with HM, and it must send reports sorted by SN and without gap.
    bool useSequenceStreamProtocol_;

    mutable size_t reportsSerializationSize_;
};

HealthReportingComponent::SequenceStream::SequenceStream(
    HealthReportingComponent & owner,
    SequenceStreamId && sequenceStreamId,
    wstring const & traceContext)
    : owner_(owner)
    , sequenceStreamId_(move(sequenceStreamId))
    , instance_(0)
    , sequenceStreamIdString_()
    , traceContext_(traceContext)
    , ackedUpToLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , initialLsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , invalidateSequence_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , state_(SequenceStreamState::Uninitialized)
    , healthReports_()
    , sequenceRanges_(0)
    , skippedRanges_(0)
    , lock_()
{
    sequenceStreamIdString_ = sequenceStreamId_.ToString();

    healthReports_ = make_unique<HealthReportsMap>(traceContext_, sequenceStreamIdString_);
    HealthReportingComponent::Trace->SequenceStreamCtor(traceContext_, *this);
}

HealthReportingComponent::SequenceStream::~SequenceStream()
{
    HealthReportingComponent::Trace->SequenceStreamDtor(traceContext_, *this);
}

bool HealthReportingComponent::SequenceStream::get_IsPreInitializing() const
{
    AcquireReadLock grab(lock_);
    return state_ == SequenceStreamState::PreInitializing;
}

ErrorCode HealthReportingComponent::SequenceStream::PreInitialize(FABRIC_INSTANCE_ID instance, __out long & updateCount)
{
    AcquireWriteLock grab(lock_);

    HealthReportingComponent::Trace->PreInitialize(traceContext_, *this, instance);

    updateCount = -static_cast<long>(healthReports_->Size());

    ackedUpToLsn_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    initialLsn_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    invalidateSequence_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    healthReports_->SetUseSequenceStreamProtocol();

    sequenceRanges_.Clear();
    skippedRanges_.Clear();

    instance_ = instance;
    state_ = SequenceStreamState::PreInitializing;

    return ErrorCodeValue::Success;
}

ErrorCode HealthReportingComponent::SequenceStream::PostInitialize(FABRIC_SEQUENCE_NUMBER sequence, FABRIC_SEQUENCE_NUMBER invalidateSequence)
{
    AcquireWriteLock grab(lock_);

    ASSERT_IFNOT(state_ == SequenceStreamState::PreInitialized && sequence >= ackedUpToLsn_,
        "HealthReportingComponent::SequenceStream::PostInitialize invalid state {0}, sequence={1}", *this, sequence);

    VersionRange range(0, sequence);
    skippedRanges_.Remove(range);
    sequenceRanges_.Add(range);
    initialLsn_ = sequence;
    invalidateSequence_ = invalidateSequence;

    HealthReportingComponent::Trace->PostInitialize(traceContext_, sequence, invalidateSequence, *this);
    state_ = SequenceStreamState::Initialized;

    return ErrorCodeValue::Success;
}

ErrorCode HealthReportingComponent::SequenceStream::GetProgress(__out FABRIC_SEQUENCE_NUMBER & progress) const
{
    AcquireReadLock grab(lock_);

    ASSERT_IF(state_ == SequenceStreamState::Uninitialized,
        "HealthReportingComponent::SequenceStream::GetProgress invalid this={0}", *this);

    if (state_ == SequenceStreamState::PreInitializing)
    {
        return ErrorCodeValue::NotReady;
    }

    progress = ackedUpToLsn_;

    return ErrorCodeValue::Success;
}

ErrorCode HealthReportingComponent::SequenceStream::HealthSkipSequence(FABRIC_SEQUENCE_NUMBER sequenceToSkip)
{
    AcquireWriteLock grab(lock_);

    ASSERT_IF(state_ == SequenceStreamState::Uninitialized,
        "HealthReportingComponent::SequenceStream::HealthSkipSequence invalid state {0}", *this);

    if (sequenceRanges_.EndVersion > sequenceToSkip)
    {
        sequenceRanges_.Add(VersionRange(sequenceToSkip));
    }
    else
    {
        skippedRanges_.Add(VersionRange(sequenceToSkip));
    }

    return ErrorCodeValue::Success;
}

ErrorCode HealthReportingComponent::SequenceStream::ProcessReport(
    HealthReport && report,
    bool sendImmediately,
    __out long & updateCount)
{
    AcquireWriteLock grab(lock_);

    updateCount = 0;

    auto reportSequenceNumber = report.SequenceNumber;
    ASSERT_IF(reportSequenceNumber <= FABRIC_INVALID_SEQUENCE_NUMBER,
        "ProcessReport invalid sequence number {0}. this={1}", reportSequenceNumber, *this);

    if (state_ != SequenceStreamState::Uninitialized)
    {
        while (!skippedRanges_.IsEmpty && skippedRanges_.VersionRanges[0].StartVersion < reportSequenceNumber)
        {
            if (skippedRanges_.VersionRanges[0].EndVersion <= reportSequenceNumber)
            {
                sequenceRanges_.Add(skippedRanges_.VersionRanges[0]);
                skippedRanges_.RemoveFirstRange();
            }
            else
            {
                Assert::CodingError("{0} skipped range {1} contains {2}", *this, skippedRanges_, reportSequenceNumber);
            }
        }

        if (!sequenceRanges_.Add(VersionRange(reportSequenceNumber)))
        {
            ASSERT_IF(
                state_ == SequenceStreamState::Initialized && reportSequenceNumber >= initialLsn_,
                "Adding invalid report {0} to {1} with initialLsn {2}",
                reportSequenceNumber,
                *this,
                initialLsn_);

            return ErrorCodeValue::HealthStaleReport;
        }
    }

    HealthReportWrapperPtr insertedReport;
    auto error = healthReports_->ProcessReport(move(report), insertedReport, updateCount);
    if (error.IsSuccess())
    {
        HealthReportingComponent::Trace->AddReport(
            traceContext_,
            *this,
            insertedReport->Report.EntityPropertyId,
            insertedReport->Report.SequenceNumber,
            sendImmediately);
    }
    else
    {
        HealthReportingComponent::Trace->IgnoredReportDueToError(
            traceContext_,
            *this,
            report.EntityPropertyId,
            error,
            error.Message);
    }

    return error;
}

void HealthReportingComponent::SequenceStream::ProcessReportResult(HealthReportResult const & reportResult, __out long & updateCount)
{
    updateCount = 0;

    AcquireWriteLock grab(lock_);

    if (state_ == SequenceStreamState::PreInitializing || state_ == SequenceStreamState::PreInitialized)
    {
        return;
    }

    if (reportResult.Error != ErrorCodeValue::Success && 
        HealthReportingComponent::IsRetryableError(reportResult.Error))
    {
        // Ignore the report, there is nothing to be done
        HealthReportingComponent::Trace->IgnoreReportResultRetryableError(
            traceContext_,
            *this,
            reportResult.EntityPropertyId,
            reportResult.SequenceNumber,
            reportResult.Error);
        return;
    }

    auto error = healthReports_->ProcessReportResult(reportResult, updateCount);
    if (error.IsSuccess())
    {
        HealthReportingComponent::Trace->CompleteReport(
            traceContext_,
            *this,
            reportResult.EntityPropertyId,
            reportResult.SequenceNumber,
            reportResult.Error);
    }
    else
    {
        WriteInfo(
            Constants::HealthClientReportSource,
            traceContext_,
            "{0}: ignore result {1} sn={2}, error={3}: {4}{5}.",
            *this,
            reportResult.EntityPropertyId,
            reportResult.SequenceNumber,
            reportResult.Error,
            error,
            error.Message);
    }
}

void HealthReportingComponent::SequenceStream::ProcessSequenceResult(
    SequenceStreamResult const & sequenceResult,
    ActivityId const & activityId,
    bool isGetSequenceStreamProgress)
{
    AcquireWriteLock grab(lock_);

    if ((state_ == SequenceStreamState::Uninitialized) ||
        (isGetSequenceStreamProgress && (state_ != SequenceStreamState::PreInitializing)) ||
        (sequenceResult.Instance != instance_) ||
        (sequenceResult.UpToLsn != FABRIC_INVALID_SEQUENCE_NUMBER && ackedUpToLsn_ >= sequenceResult.UpToLsn))
    {
        // Must be stale, ignore
        HealthReportingComponent::Trace->IgnoreSequenceStreamResult(
            traceContext_,
            activityId,
            sequenceResult.Instance,
            sequenceResult.UpToLsn,
            *this);
        return;
    }

    if (state_ == SequenceStreamState::PreInitializing)
    {
        state_ = SequenceStreamState::PreInitialized;
    }
    else if (state_ == SequenceStreamState::Initialized)
    {
        int64 endLsn = (sequenceRanges_.IsEmpty ? 0 : sequenceRanges_.VersionRanges[0].EndVersion);
        ASSERT_IF(
            ackedUpToLsn_ > endLsn,
            "Invalid progress {0} received for {1}",
            sequenceResult.UpToLsn,
            *this);
    }

    ackedUpToLsn_ = sequenceResult.UpToLsn;

    HealthReportingComponent::Trace->SequenceStreamProcessed(
        traceContext_,
        activityId,
        sequenceResult.Instance,
        sequenceResult.UpToLsn,
        *this);
}

void HealthReportingComponent::SequenceStream::GetContent(
    int maxNumberOfReports,
    Common::TimeSpan const & sendRetryInterval,
    __inout vector<HealthReport> & reports,
    __inout vector<SequenceStreamInformation> & sequenceStreams)
{
    ASSERT_IF(maxNumberOfReports <= 0,
        "Invalid value for maxNumberOfReports {0}. this={1}", maxNumberOfReports, *this);

    size_t currentReportCount = 0;
    AcquireWriteLock grab(lock_);

    if (state_ == SequenceStreamState::Uninitialized)
    {
        healthReports_->GetContent(maxNumberOfReports, sendRetryInterval, reports);
        return;
    }
    else if (state_ != SequenceStreamState::Initialized)
    {
        // PostInitialize is not called yet
        WriteInfo(Constants::HealthClientSequenceStreamSource, traceContext_, "Sequence stream not ready: {0}", *this);
        return;
    }

    // Generate based on sequence stream
    if (!sequenceRanges_.IsEmpty)
    {
        FABRIC_SEQUENCE_NUMBER rangeEnd = sequenceRanges_.VersionRanges[0].EndVersion;
        FABRIC_SEQUENCE_NUMBER lastLsn;
        healthReports_->GetSortedContent(static_cast<size_t>(maxNumberOfReports), rangeEnd, reports, currentReportCount, lastLsn);

        if (lastLsn > ackedUpToLsn_)
        {
            WriteNoise(Constants::HealthClientReportSource, traceContext_, "{0}: Add SS {1} with {2} reports", *this, lastLsn, currentReportCount);
            sequenceStreams.push_back(
                SequenceStreamInformation(sequenceStreamId_.Kind, sequenceStreamId_.SourceId, instance_, ackedUpToLsn_, lastLsn, currentReportCount, invalidateSequence_));
        }
    }
}

void HealthReportingComponent::SequenceStream::WriteTo(TextWriter& writer, FormatOptions const &options) const
{
    UNREFERENCED_PARAMETER(options);
    writer.WriteLine("[{0}:{1}|{2}] acked: [0,{3}) ranges: {4}, reportsSize: {5}",
        sequenceStreamIdString_,
        instance_,
        state_,
        ackedUpToLsn_,
        sequenceRanges_,
        healthReports_->ReportsSize);
}

void HealthReportingComponent::SequenceStream::WriteToEtw(uint16 contextSequenceId) const
{
    HealthReportingComponent::Trace->SequenceStream(
        contextSequenceId,
        sequenceStreamIdString_,
        instance_,
        state_,
        ackedUpToLsn_,
        wformatString(sequenceRanges_),
        static_cast<uint64>(healthReports_->ReportsSize));
}

// ===========================================
// HealthReportsMap implementation
// ===========================================
HealthReportingComponent::SequenceStream::HealthReportsMap::HealthReportsMap(
    std::wstring const & traceContext,
    std::wstring const & ownerId)
    : traceContext_(traceContext)
    , ownerId_(ownerId)
    , reports_()
    , sortedHealthReports_()
    , entityHealthReports_()
    , useSequenceStreamProtocol_(false)
    , reportsSerializationSize_(0u)
{
}

HealthReportingComponent::SequenceStream::HealthReportsMap::~HealthReportsMap()
{
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::SetUseSequenceStreamProtocol()
{
    useSequenceStreamProtocol_ = true;
    Clear();
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::Clear()
{
    reports_.clear();
    sortedHealthReports_.Clear();
    entityHealthReports_.clear();
    reportsSerializationSize_ = 0;
}

Common::ErrorCode HealthReportingComponent::SequenceStream::HealthReportsMap::ProcessReport(
    ServiceModel::HealthReport && report,
    __inout HealthReportWrapperPtr & insertedReport,
    __out long & updateCount)
{
    // First, look for the report in the map of reports per entity
    bool replaceReport = false;
    auto entityIter = entityHealthReports_.find(report.EntityInformation);
    if (entityIter != entityHealthReports_.end())
    {
        // Check the instance of the new report against the instance of the entity already stored
        bool isDeleteReport = report.IsForDelete;
        bool cleanPreviousReports;
        auto error = report.EntityInformation->CheckAgainstInstance(entityIter->first->EntityInstance, isDeleteReport, cleanPreviousReports);
        if (!error.IsSuccess())
        {
            return error;
        }

        // Check whether a previous report for same entity property id exists, and if it exists,
        // if the new sequence number is higher.
        error = CanAcceptReport(report, replaceReport);
        if (!error.IsSuccess())
        {
            return error;
        }

        // Check whether the new report is for an entity instance that is currently pending delete. If so, reject.
        if (!isDeleteReport &&
            !report.EntityInformation->HasUnknownInstance() &&
            report.EntityInformation->EntityInstance == entityIter->first->EntityInstance &&
            entityIter->second.size() == 1 &&
            entityIter->second.back()->Report.IsForDelete)
        {
            wstring errorMessage = wformatString(HMResource::GetResources().HealthStaleReportDueToEntityDeleted, entityIter->first->EntityInstance);
            return ErrorCode(ErrorCodeValue::HealthStaleReport, move(errorMessage));
        }

        if (cleanPreviousReports)
        {
            HealthReportingComponent::Trace->CleanReports(traceContext_, ownerId_, report.EntityPropertyId, entityIter->first->EntityInstance, report.EntityInformation->EntityInstance, isDeleteReport);
            CleanEntityReports(entityIter->second, updateCount);

            // Replace the entity information with the new one since the instance changed
            entityHealthReports_.erase(entityIter);
            entityIter = entityHealthReports_.insert(make_pair(report.EntityInformation, vector<HealthReportWrapperPtr>())).first;

            replaceReport = false;
        }
    }
    else
    {
        // The entity doesn't exist; the report must not exist in the map of reports, so add it. No more checks needed.
#ifdef DBG
        ASSERT_IF(reports_.find(report.EntityPropertyId) != reports_.end(), "{0}: Report {1} found in reports map but not in entity map.", ownerId_, report.EntityPropertyId);
#endif
        entityIter = entityHealthReports_.insert(make_pair(report.EntityInformation, vector<HealthReportWrapperPtr>())).first;
    }

    auto reportPtr = make_unique<HealthReportWrapper>(move(report));
    insertedReport = reportPtr.get();

    // Add to list of reports
    if (replaceReport)
    {
        // The number of reports doesn't change.
        // The size changes by removing the size of the previous report and adding the size of the new report. The key is not changed.
        auto reportsIter = reports_.find(reportPtr->Report.EntityPropertyId);
        ASSERT_IF(reportsIter == reports_.end(), "{0}: HealthReportingComponent::SequenceStream::HealthReportsMap::ReplaceReport: there is no previous report for {0}", ownerId_, *reportPtr);

        reportsSerializationSize_ += reportPtr->Report.EstimateSize();

        auto const & previousReport = reportsIter->second;
        reportsSerializationSize_ -= previousReport->Report.EstimateSize();

        // Remove previous reportPtr from sorted list and add the new one
        RemoveSortedHealthReport(previousReport.get());
        AddSortedHealthReport(insertedReport);

        // Replace report in entity list
        ReplaceEntityHealthReport(entityIter->second, previousReport.get(), insertedReport);

        reportsIter->second = move(reportPtr);
    }
    else
    {
        reportsSerializationSize_ += EstimateHealthReportMapEntrySize(*reportPtr);

        // Add to sorted list
        AddSortedHealthReport(insertedReport);

        // Add to entity list
        entityIter->second.push_back(insertedReport);

        // Add to list of reports
        auto insertResult = reports_.insert(make_pair(reportPtr->Report.EntityPropertyId, move(reportPtr)));
        ASSERT_IFNOT(insertResult.second, "{0}: HealthReportingComponent::SequenceStream::HealthReportsMap::Add: Key already exists for {1}", ownerId_, insertedReport->Report);

        ++updateCount;
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthReportingComponent::SequenceStream::HealthReportsMap::ProcessReportResult(
    HealthReportResult const & reportResult,
    __out long & updateCount)
{
    HealthReportingComponent::SequenceStream::HealthReportsMap::ReportsIterator reportsIter;
    auto error = GetReport(reportResult.EntityPropertyId, reportsIter);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto const & report = reportsIter->second->Report;
    if (report.SequenceNumber != reportResult.SequenceNumber)
    {
        // Report found, but has different sn
        return ErrorCode(ErrorCodeValue::HealthStaleReport, wformatString(" current sn {0}", report.SequenceNumber));
    }

    // The report result should be either success or non-retryable error, checked by the owner
    auto entityIter = entityHealthReports_.find(report.EntityInformation);
    ASSERT_IF(entityIter == entityHealthReports_.end(), "{0}: Report {1} not found in entity map.", ownerId_, report.EntityInformation);

    if (reportResult.Error == ErrorCodeValue::HealthEntityDeleted)
    {
        // If the same instance has been deleted by HM, remove all pending reports on the entity
        HealthReportingComponent::Trace->CleanReportsHMEntityDeleted(traceContext_, ownerId_, reportResult.EntityPropertyId, entityIter->first->EntityInstance);
        CleanEntityReports(entityIter->second, updateCount);
        entityHealthReports_.erase(entityIter);
    }
    else
    {
        RemoveReport(reportsIter, entityIter->second, updateCount);
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthReportingComponent::SequenceStream::HealthReportsMap::CanAcceptReport(
    ServiceModel::HealthReport const & report,
    __out bool & found) const
{
    auto reportsIter = reports_.find(report.EntityPropertyId);
    if (reportsIter == reports_.end())
    {
        found = false;
    }
    else
    {
        // Check against sequence number for staleness
        found = true;
        auto const & previousReport = reportsIter->second->Report;
        if (previousReport.SequenceNumber >= report.SequenceNumber)
        {
            // The new report has lower sequence number, reject report
            wstring errorMessage = wformatString("{0} {1}, {2}.", HMResource::GetResources().HealthStaleReportDueToSequenceNumber, report.SequenceNumber, previousReport.SequenceNumber);
            return ErrorCode(ErrorCodeValue::HealthStaleReport, move(errorMessage));
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthReportingComponent::SequenceStream::HealthReportsMap::GetReport(
    std::wstring const & reportEntityPropertyId,
    __inout ReportsIterator & reportsIter) const
{
    reportsIter = reports_.find(reportEntityPropertyId);
    if (reportsIter == reports_.end())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    return ErrorCode::Success();
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::GetContent(
    size_t maxNumberOfReports,
    Common::TimeSpan const & sendRetryInterval,
    __inout vector<HealthReport> & reports) const
{
    ASSERT_IF(useSequenceStreamProtocol_, "{0}: GetContent: use sequence stream protocol, should not get reports from list of reports, but sorted reports list.", ownerId_);
    size_t currentReportCount = 0;
    auto now = Stopwatch::Now();
    for (auto it = reports_.begin(); it != reports_.end() && currentReportCount < maxNumberOfReports; ++it)
    {
        if (it->second->AddReportForSend(reports, now, sendRetryInterval))
        {
            ++currentReportCount;
        }
    }
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::GetSortedContent(
    size_t maxNumberOfReports,
    FABRIC_SEQUENCE_NUMBER lastAcceptedSN,
    __out std::vector<ServiceModel::HealthReport> & reports,
    __out size_t & currentReportCount,
    __out FABRIC_SEQUENCE_NUMBER & lastLsn) const
{
    ASSERT_IFNOT(useSequenceStreamProtocol_, "{0}: GetSortedContent: use sequence stream protocol is false, should not get reports from list of sorted reports list.", ownerId_);
    sortedHealthReports_.GetContentUpTo(maxNumberOfReports, lastAcceptedSN, reports, currentReportCount, lastLsn);
}

size_t HealthReportingComponent::SequenceStream::HealthReportsMap::EstimateHealthReportMapEntrySize(HealthReportWrapper const & report)
{
    return (report.Report.EntityPropertyId.size() + 1) * sizeof(wchar_t) + report.Report.EstimateSize();
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::AddSortedHealthReport(__in HealthReportWrapperPtr const report)
{
    if (useSequenceStreamProtocol_)
    {
        auto error = sortedHealthReports_.Add(report);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: Error adding {1} to sorted reports list: {2}", ownerId_, *report, error);
    }
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::RemoveSortedHealthReport(__in HealthReportWrapperPtr const report)
{
    if (useSequenceStreamProtocol_)
    {
        auto error = sortedHealthReports_.Remove(report);
        ASSERT_IFNOT(error.IsSuccess(), "{0}: Error removing {1} from sorted reports list: {2}", ownerId_, *report, error);
    }
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::RemoveEntityHealthReport(
    __in std::vector<HealthReportWrapperPtr> & entityReports,
    __in HealthReportWrapperPtr const report)
{
    // Find the report in the vector using pointer comparison. O(n)
    // To remove it, swap last entry with the found one, since order is not important. O(1)
    auto it = find(entityReports.begin(), entityReports.end(), report);
    ASSERT_IF(it == entityReports.end(), "{0}: Error removing {1} from entity reports list: not found", ownerId_, *report);
    swap(*it, entityReports.back());
    entityReports.pop_back();
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::ReplaceEntityHealthReport(
    __in std::vector<HealthReportWrapperPtr> & entityReports,
    __in HealthReportWrapperPtr const report,
    __in HealthReportWrapperPtr const newReport)
{
    auto it = find(entityReports.begin(), entityReports.end(), report);
    ASSERT_IF(it == entityReports.end(), "{0}: Error removing {1} from entity reports list: not found", ownerId_, *report);
    *it = newReport;
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::RemoveReport(
    HealthReportingComponent::SequenceStream::HealthReportsMap::ReportsIterator reportsIter,
    __in std::vector<HealthReportWrapperPtr> & entityReports,
    __out long & updateCount)
{
    HealthReportWrapperPtr oldReport = (reportsIter->second).get();

    // Remove old report from the entity map
    RemoveEntityHealthReport(entityReports, oldReport);

    // Remove the old report from the sorted list of reports
    RemoveSortedHealthReport(oldReport);

    // Remove from the list of reports
    reportsSerializationSize_ -= EstimateHealthReportMapEntrySize(*reportsIter->second);
    reports_.erase(reportsIter);

    --updateCount;
}

void HealthReportingComponent::SequenceStream::HealthReportsMap::CleanEntityReports(
    __in std::vector<HealthReportWrapperPtr> & entityReports,
    __out long & updateCount)
{
    for (auto const & oldReport : entityReports)
    {
        HealthReportingComponent::SequenceStream::WriteNoise(Constants::HealthClientReportSource, traceContext_, "Clean {0}", *oldReport);

        // Remove report from the list of reports
        auto reportsIter = reports_.find(oldReport->Report.EntityPropertyId);
        ASSERT_IF(reportsIter == reports_.end(), "{0}: HealthReportsMap::CleanEntityReports: {1} not found in reports map.", ownerId_, *oldReport);

        // Remove the old reports from the sorted list of reports
        RemoveSortedHealthReport(reportsIter->second.get());

        // Remove from the list of reports
        reportsSerializationSize_ -= EstimateHealthReportMapEntrySize(*reportsIter->second);
        reports_.erase(reportsIter);
    }

    updateCount -= static_cast<long>(entityReports.size());
    // Clear all entity reports
    entityReports.clear();
}
