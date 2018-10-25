// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Management::HealthManager;

HealthEventStoreData::HealthEventStoreData()
    : StoreData()
    , sourceId_()
    , property_()
    , state_(FABRIC_HEALTH_STATE_INVALID)
    , lastModifiedUtc_(DateTime::Now())
    , sourceUtcTimestamp_(DateTime::Zero)
    , timeToLive_(TimeSpan::MaxValue)
    , description_()
    , reportSequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , removeWhenExpired_(false)
    , persistedIsExpired_(false)
    , cachedIsExpired_(false)
    , isPendingUpdateToStore_(false)
    , lastOkTransitionAt_(DateTime::Zero)
    , lastWarningTransitionAt_(DateTime::Zero)
    , lastErrorTransitionAt_(DateTime::Zero)
    , isInStore_(false)
    , diff_()
    , priority_(Priority::NotAssigned)
{
}

HealthEventStoreData::HealthEventStoreData(
    HealthInformation const & healthInfo,
    ServiceModel::Priority::Enum priority,
    Store::ReplicaActivityId const & activityId)
    : StoreData(activityId)
    , sourceId_(healthInfo.SourceId)
    , property_(healthInfo.Property)
    , state_(healthInfo.State)
    , lastModifiedUtc_(DateTime::Now())
    , sourceUtcTimestamp_(healthInfo.SourceUtcTimestamp)
    , timeToLive_(healthInfo.TimeToLive)
    , description_(healthInfo.Description)
    , reportSequenceNumber_(healthInfo.SequenceNumber)
    , removeWhenExpired_(healthInfo.RemoveWhenExpired)
    , persistedIsExpired_(false)
    , cachedIsExpired_(false)
    , isPendingUpdateToStore_(false)
    , lastOkTransitionAt_(DateTime::Zero)
    , lastWarningTransitionAt_(DateTime::Zero)
    , lastErrorTransitionAt_(DateTime::Zero)
    , isInStore_(false)
    , diff_()
    , priority_(priority)
{
    switch (healthInfo.State)
    {
    case FABRIC_HEALTH_STATE_OK:
        lastOkTransitionAt_ = lastModifiedUtc_;
        break;
    case FABRIC_HEALTH_STATE_WARNING:
        lastWarningTransitionAt_ = lastModifiedUtc_;
        break;
    case FABRIC_HEALTH_STATE_ERROR:
        lastErrorTransitionAt_ = lastModifiedUtc_;
        break;
    default:
        Assert::CodingError("Unsupported health state for event {0}", healthInfo.State);
        break;
    }
}

// Called to update an existing event.
// The existing event must be in store.
HealthEventStoreData::HealthEventStoreData(
    HealthEventStoreData const & previousValue,
    Store::ReplicaActivityId const & activityId)
    : StoreData(activityId)
    , sourceId_(previousValue.sourceId_)
    , property_(previousValue.property_)
    , state_(previousValue.state_)
    , lastModifiedUtc_(previousValue.LastModifiedUtc)
    , sourceUtcTimestamp_(previousValue.SourceUtcTimestamp)
    , timeToLive_(previousValue.timeToLive_)
    , description_(previousValue.description_)
    , reportSequenceNumber_(previousValue.ReportSequenceNumber)
    , removeWhenExpired_(previousValue.removeWhenExpired_)
    , persistedIsExpired_(previousValue.cachedIsExpired_) // Copy the cached value, not the persisted value
    , cachedIsExpired_(previousValue.cachedIsExpired_)
    , isPendingUpdateToStore_(previousValue.isPendingUpdateToStore_)
    , lastOkTransitionAt_(previousValue.lastOkTransitionAt_)
    , lastWarningTransitionAt_(previousValue.lastWarningTransitionAt_)
    , lastErrorTransitionAt_(previousValue.lastErrorTransitionAt_)
    , isInStore_(true)
    , diff_() // no diff
    , priority_(previousValue.priority_)
{
    ASSERT_IF(
        !previousValue.diff_ && previousValue.cachedIsExpired_ == previousValue.persistedIsExpired_,
        "{0}: create from diff: no changes {1}",
        activityId.ActivityId,
        previousValue);
}

HealthEventStoreData::HealthEventStoreData(HealthEventStoreData && other)
    : StoreData(move(other))
    , sourceId_(move(other.sourceId_))
    , property_(move(other.property_))
    , state_(move(other.state_))
    , lastModifiedUtc_(move(other.lastModifiedUtc_))
    , sourceUtcTimestamp_(move(other.sourceUtcTimestamp_))
    , timeToLive_(move(other.timeToLive_))
    , description_(move(other.description_))
    , reportSequenceNumber_(move(other.reportSequenceNumber_))
    , removeWhenExpired_(move(other.removeWhenExpired_))
    , persistedIsExpired_(move(other.persistedIsExpired_))
    , cachedIsExpired_(move(other.cachedIsExpired_))
    , isPendingUpdateToStore_(move(other.isPendingUpdateToStore_))
    , lastOkTransitionAt_(move(other.lastOkTransitionAt_))
    , lastWarningTransitionAt_(move(other.lastWarningTransitionAt_))
    , lastErrorTransitionAt_(move(other.lastErrorTransitionAt_))
    , isInStore_(move(other.isInStore_))
    , diff_(move(other.diff_))
    , priority_(move(other.priority_))
{
}

HealthEventStoreData & HealthEventStoreData::operator = (HealthEventStoreData && other)
{
    if (this != &other)
    {
        sourceId_ = move(other.sourceId_);
        property_ = move(other.property_);
        state_ = move(other.state_);
        lastModifiedUtc_ = move(other.lastModifiedUtc_);
        sourceUtcTimestamp_ = move(other.sourceUtcTimestamp_);
        timeToLive_ = move(other.timeToLive_);
        description_ = move(other.description_);
        reportSequenceNumber_ = move(other.reportSequenceNumber_);
        removeWhenExpired_ = move(other.removeWhenExpired_);
        persistedIsExpired_ = move(other.persistedIsExpired_);
        cachedIsExpired_ = move(other.cachedIsExpired_);
        isPendingUpdateToStore_ = move(other.isPendingUpdateToStore_);
        lastOkTransitionAt_ = move(other.lastOkTransitionAt_);
        lastErrorTransitionAt_ = move(other.lastErrorTransitionAt_);
        lastWarningTransitionAt_ = move(other.lastWarningTransitionAt_);
        isInStore_ = move(other.isInStore_);
        diff_ = move(other.diff_);
        priority_ = move(other.priority_);
    }

    StoreData::operator=(move(other));
    return *this;
}

HealthEventStoreData::~HealthEventStoreData()
{
}

bool HealthEventStoreData::get_IsSystemReport() const 
{ 
    return Common::StringUtility::StartsWithCaseInsensitive<std::wstring>(sourceId_, ServiceModel::Constants::EventSystemSourcePrefix); 
}

bool HealthEventStoreData::get_IsStatePropertyError() const 
{ 
    return state_ == FABRIC_HEALTH_STATE_ERROR && property_ == ServiceModel::Constants::HealthStateProperty; 
}

int64 HealthEventStoreData::get_ReportSequenceNumber() const
{
    return diff_ ? diff_->ReportSequenceNumber : reportSequenceNumber_;
}

Common::DateTime HealthEventStoreData::get_LastModifiedUtc() const
{
    return diff_ ? diff_->LastModifiedUtc : lastModifiedUtc_;
}

Common::DateTime HealthEventStoreData::get_SourceUtcTimestamp() const
{
    return diff_ ? diff_->SourceUtcTimestamp : sourceUtcTimestamp_;
}

Priority::Enum HealthEventStoreData::get_Priority() const
{
    ASSERT_IF(priority_ == Priority::NotAssigned, "{0}: Priority not set", *this);
    return priority_;
}

bool HealthEventStoreData::DoesNotImpactHealth() const
{
    // All user reports with HealthState OK and RemoveWhenExpired = true
    // All user reports with HealthState OK and TTL infinite
    return priority_ < Priority::High &&
           FABRIC_HEALTH_STATE_OK == state_ &&
           (removeWhenExpired_ || timeToLive_ == TimeSpan::MaxValue);
}

// After inconsistency between diff and current have been persisted to disk, move diff into current
void HealthEventStoreData::MoveDiffToCurrent(Store::ReplicaActivityId const & replicaActivityId)
{
    if (diff_)
    {
        lastModifiedUtc_ = diff_->LastModifiedUtc;
        sourceUtcTimestamp_ = diff_->SourceUtcTimestamp;
        reportSequenceNumber_ = diff_->ReportSequenceNumber;
        diff_.reset();
    }
    else
    {
        ASSERT_IF(persistedIsExpired_ == cachedIsExpired_, "{0}: MoveDiffToCurrent when there are no changes: {1}", replicaActivityId.ActivityId, *this);
    }

    persistedIsExpired_ = cachedIsExpired_;
    this->ReInitializeTracing(replicaActivityId);
}

FABRIC_HEALTH_STATE HealthEventStoreData::GetEvaluatedHealthState(bool considerWarningAsError) const
{
    // Use already set value for expired
    if (this->IsExpired)
    {
        // Consider expired events as error
        return FABRIC_HEALTH_STATE_ERROR;
    }

    if (considerWarningAsError && state_ == FABRIC_HEALTH_STATE_WARNING)
    {
        return FABRIC_HEALTH_STATE_ERROR;
    }

    return state_;
}

void HealthEventStoreData::Update(HealthEventStoreData && other)
{
#ifdef DBG
    ASSERT_IF(other.diff_, "HealthEventStoreData::Update: Can't update with event with diff. Old={0}, new={1}", *this, other);
    ASSERT_IFNOT(sourceId_ == other.sourceId_, "Update: SourceId changed, old={0}, new={1}", *this, other);
    ASSERT_IFNOT(property_ == other.property_, "Update: Property changed, old={0}, new={1}", *this, other);
#endif

    // Compute the transition state times if the state changed
    if (state_ != other.state_)
    {
        // Mark when the previous state ended
        switch (state_)
        {
        case FABRIC_HEALTH_STATE_OK:
            lastOkTransitionAt_ = other.lastModifiedUtc_;
            break;
        case FABRIC_HEALTH_STATE_WARNING:
            lastWarningTransitionAt_ = other.lastModifiedUtc_;
            break;
        case FABRIC_HEALTH_STATE_ERROR:
            lastErrorTransitionAt_ = other.lastModifiedUtc_;
            break;
        default:
            Assert::CodingError("Unsupported health state for event {0}", *this);
            break;
        }

        // Mark when the new state began
        switch (other.state_)
        {
        case FABRIC_HEALTH_STATE_OK:
            lastOkTransitionAt_ = move(other.lastOkTransitionAt_);
            break;
        case FABRIC_HEALTH_STATE_WARNING:
            lastWarningTransitionAt_ = move(other.lastWarningTransitionAt_);
            break;
        case FABRIC_HEALTH_STATE_ERROR:
            lastErrorTransitionAt_ = move(other.lastErrorTransitionAt_);
            break;
        default:
            Assert::CodingError("Unsupported health state for event {0}", other);
            break;
        }

        state_ = move(other.state_);
    }

    lastModifiedUtc_ = move(other.lastModifiedUtc_);
    sourceUtcTimestamp_ = move(other.sourceUtcTimestamp_);
    timeToLive_ = move(other.timeToLive_);
    description_ = move(other.description_);
    reportSequenceNumber_ = move(other.reportSequenceNumber_);
    removeWhenExpired_ = move(other.removeWhenExpired_);
    persistedIsExpired_ = move(other.persistedIsExpired_);
    cachedIsExpired_ = move(other.cachedIsExpired_);
    isPendingUpdateToStore_ = move(other.isPendingUpdateToStore_);
    diff_ = move(other.diff_);
}

bool HealthEventStoreData::UpdateExpired()
{
    if (!cachedIsExpired_)
    {
        // If diff_ is set, look at diff_ and ignore persisted value.
        // If there are any differences, they will be made consistent by writing diff to store
        // on next cleanup timer or report.
        if (this->LastModifiedUtc.AddWithMaxValueCheck(timeToLive_) <= DateTime::Now())
        {
            cachedIsExpired_= true;
        }
    }

    return cachedIsExpired_;
}

void HealthEventStoreData::UpdateOnLoadFromStore(FABRIC_HEALTH_REPORT_KIND healthInfoKind)
{
    // If store persisted data says expired, maintain the information
    if (!persistedIsExpired_ && !removeWhenExpired_)
    {
        // Otherwise, ignore the persisted last modified time and use the load store time instead
        diff_ = make_unique<HealthEventDiff>(sourceUtcTimestamp_, reportSequenceNumber_);
    }

    cachedIsExpired_ = persistedIsExpired_;
    priority_ = HealthReport::GetPriority(healthInfoKind, sourceId_, property_);
}

std::wstring HealthEventStoreData::GeneratePrefix(std::wstring const & entityId)
{
    return wformatString(
        "{0}{1}",
        entityId,
        Constants::TokenDelimeter);
}

void HealthEventStoreData::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1},{2},Generated@{3}, Updated@{4}, timeToLive={5}, reportLSN={6}, desc={7}, removeWhenExpired={8}, {9})",
        this->Type,
        this->Key,
        state_,
        this->SourceUtcTimestamp,
        this->LastModifiedUtc,
        timeToLive_,
        this->ReportSequenceNumber,
        description_,
        removeWhenExpired_,
        GetTransitionHistory());
}

void HealthEventStoreData::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->HealthEventStoreDataTrace(
        contextSequenceId,
        this->Type,
        this->Key,
        wformatString(state_),
        this->SourceUtcTimestamp,
        this->LastModifiedUtc,
        timeToLive_,
        this->ReportSequenceNumber,
        description_,
        removeWhenExpired_,
        GetTransitionHistory());
}

std::wstring HealthEventStoreData::GetTransitionHistory() const
{
    wstring history;
    StringWriter writer(history);
    if (lastOkTransitionAt_ != DateTime::Zero)
    {
        writer.Write("LastOkAt:{0}.", lastOkTransitionAt_);
    }

    if (lastWarningTransitionAt_ != DateTime::Zero)
    {
        writer.Write("LastWarningAt:{0}.", lastWarningTransitionAt_);
    }

    if (lastErrorTransitionAt_ != DateTime::Zero)
    {
        writer.Write("LastErrorAt:{0}.", lastErrorTransitionAt_);
    }
    
    return history;
}

bool HealthEventStoreData::CanUpdateEvent(
    FABRIC_SEQUENCE_NUMBER reportSequenceNumber) const
{
    // Check that the sequence number increased
    ASSERT_IF(reportSequenceNumber <= FABRIC_INVALID_SEQUENCE_NUMBER, "{0}: can't update with event with sn {1}", *this, reportSequenceNumber);
    if (reportSequenceNumber <= this->ReportSequenceNumber)
    {
        HMEvents::Trace->DropReportStaleSourceLSN(
            this->ReplicaActivityId,
            this->Key,
            reportSequenceNumber_,
            reportSequenceNumber);
        return false;
    }        

    return true;
}

bool HealthEventStoreData::HasSameFields(ServiceModel::HealthReport const & report) const
{
#ifdef DBG
    ASSERT_IFNOT(sourceId_ == report.SourceId, "Update: SourceId changed, old={0}, new={1}", *this, report);
    ASSERT_IFNOT(property_ == report.Property, "Update: Property changed, old={0}, new={1}", *this, report);
#endif

    // Do not check lastModifiedUtc, sourceUtc, and sequence number
    return state_ == report.State &&
           timeToLive_ == report.TimeToLive &&
           description_ == report.Description &&
           removeWhenExpired_ == report.RemoveWhenExpired;
}

bool HealthEventStoreData::TryUpdateDiff(ServiceModel::HealthReport const & report)
{
    if (persistedIsExpired_)
    {
        // The report was persisted to disk as expired, need to write it again
        return false;
    }
    
    if (removeWhenExpired_)
    {
        // Always persist transient events
        return false;
    }

    if (!diff_)
    {
        diff_ = make_unique<HealthEventDiff>(report.SourceUtcTimestamp, report.SequenceNumber);
    }
    else
    {
        diff_->Update(report);
    }

    cachedIsExpired_ = false;
    return true;
}

HealthEvent HealthEventStoreData::GenerateEvent() const
{
    return HealthEvent(
        sourceId_,
        property_,
        timeToLive_,
        state_,
        description_,
        reportSequenceNumber_, // return persisted value, not in-memory one
        sourceUtcTimestamp_, // return persisted value, not in-memory one
        this->LastModifiedUtc,  // return last modified value, the in-memory one
        cachedIsExpired_, // previously computed value of expired
        removeWhenExpired_,
        lastOkTransitionAt_,
        lastWarningTransitionAt_,
        lastErrorTransitionAt_);
}


// ==========================================
// Internal class
// ==========================================

HealthEventStoreData::HealthEventDiff::HealthEventDiff(
    Common::DateTime const sourceUtcTimestamp,
    FABRIC_SEQUENCE_NUMBER reportSequenceNumber)
    : lastModifiedUtc_(DateTime::Now())
    , sourceUtcTimestamp_(sourceUtcTimestamp)
    , reportSequenceNumber_(reportSequenceNumber)
{
}

HealthEventStoreData::HealthEventDiff::~HealthEventDiff()
{
}

void HealthEventStoreData::HealthEventDiff::Update(ServiceModel::HealthReport const & report)
{
    lastModifiedUtc_ = DateTime::Now();
    sourceUtcTimestamp_ = report.SourceUtcTimestamp;
    reportSequenceNumber_ = report.SequenceNumber;
}
