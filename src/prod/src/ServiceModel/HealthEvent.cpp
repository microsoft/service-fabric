// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(HealthEvent)

HealthEvent::HealthEvent()
    : HealthInformation()
    , lastModifiedUtcTimestamp_(DateTime::Zero)
    , isExpired_(false)
    , lastOkTransitionAt_(DateTime::Zero)
    , lastWarningTransitionAt_(DateTime::Zero)
    , lastErrorTransitionAt_(DateTime::Zero)
{
}

HealthEvent::HealthEvent(
    std::wstring const & sourceId,
    std::wstring const & property,
    Common::TimeSpan const & timeToLive,
    FABRIC_HEALTH_STATE state,
    std::wstring const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::DateTime const & sourceUtcTimestamp,
    Common::DateTime const & lastModifiedUtcTimestamp,
    bool isExpired,
    bool removeWhenExpired,
    Common::DateTime const & lastOkTransitionAt,
    Common::DateTime const & lastWarningTransitionAt,
    Common::DateTime const & lastErrorTransitionAt)
    : HealthInformation(sourceId, property, timeToLive, state, description, sequenceNumber, sourceUtcTimestamp, removeWhenExpired)
    , lastModifiedUtcTimestamp_(lastModifiedUtcTimestamp)
    , isExpired_(isExpired)
    , lastOkTransitionAt_(lastOkTransitionAt)
    , lastWarningTransitionAt_(lastWarningTransitionAt)
    , lastErrorTransitionAt_(lastErrorTransitionAt)
{
}

ErrorCode HealthEvent::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_HEALTH_EVENT & healthEventInformation) const
{
    auto commonHealthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    auto error = this->ToCommonPublicApi(heap, *commonHealthInformation);
    if (!error.IsSuccess())
    {
        return error;
    }

    healthEventInformation.HealthInformation = commonHealthInformation.GetRawPointer();
    healthEventInformation.SourceUtcTimestamp = sourceUtcTimestamp_.AsFileTime;
    healthEventInformation.LastModifiedUtcTimestamp = lastModifiedUtcTimestamp_.AsFileTime;
    healthEventInformation.IsExpired = isExpired_ ? TRUE : FALSE;

    auto publicHealthEventEx1 = heap.AddItem<FABRIC_HEALTH_EVENT_EX1>();
    publicHealthEventEx1->LastOkTransitionAt = lastOkTransitionAt_.AsFileTime;
    publicHealthEventEx1->LastWarningTransitionAt = lastWarningTransitionAt_.AsFileTime;
    publicHealthEventEx1->LastErrorTransitionAt = lastErrorTransitionAt_.AsFileTime;

    healthEventInformation.Reserved = publicHealthEventEx1.GetRawPointer();

    return ErrorCode::Success();
}

ErrorCode HealthEvent::FromPublicApi(
    FABRIC_HEALTH_EVENT const & healthEventInformation)
{
    auto error = this->FromCommonPublicApi(*healthEventInformation.HealthInformation);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Replace sourceUtcTimestamp with the actual passed in value (initialized in FromCommonPublicApi to Now)
    sourceUtcTimestamp_ = Common::DateTime(healthEventInformation.SourceUtcTimestamp);
    lastModifiedUtcTimestamp_ = Common::DateTime(healthEventInformation.LastModifiedUtcTimestamp);
    isExpired_ = (healthEventInformation.IsExpired == TRUE) ? true : false;

    if (healthEventInformation.Reserved != NULL)
    {
        auto publicHealthEventEx1 = reinterpret_cast<FABRIC_HEALTH_EVENT_EX1*>(healthEventInformation.Reserved);
        lastOkTransitionAt_ = Common::DateTime(publicHealthEventEx1->LastOkTransitionAt);
        lastWarningTransitionAt_ = Common::DateTime(publicHealthEventEx1->LastWarningTransitionAt);
        lastErrorTransitionAt_ = Common::DateTime(publicHealthEventEx1->LastErrorTransitionAt);
    }

    return ErrorCode::Success();
}

void HealthEvent::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.Write("SourceId:'{0}' Property:'{1}' {2} ttl:{3} sn:{4} sourceUtc:{5} '{6}' lastModifiedUtc:{7} expired:{8} removeWhenExpired:{9}",
        sourceId_,
        property_,
        state_,
        timeToLive_,
        sequenceNumber_,
        sourceUtcTimestamp_,
        description_,
        lastModifiedUtcTimestamp_,
        isExpired_,
        removeWhenExpired_);

    if (lastOkTransitionAt_ != DateTime::Zero)
    {
        writer.Write(" LastOkAt:{0}.", lastOkTransitionAt_);
    }

    if (lastWarningTransitionAt_ != DateTime::Zero)
    {
        writer.Write(" LastWarningAt:{0}.", lastWarningTransitionAt_);
    }

    if (lastErrorTransitionAt_ != DateTime::Zero)
    {
        writer.Write(" LastErrorAt:{0}.", lastErrorTransitionAt_);
    }
}

wstring HealthEvent::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

// Cannot use fill even data here because compiler would not allow it (says this method isn't implemented).
void HealthEvent::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->HealthEventTrace(
        contextSequenceId,
        sourceId_,
        property_,
        state_,
        timeToLive_,
        sequenceNumber_,
        sourceUtcTimestamp_,
        description_,
        lastModifiedUtcTimestamp_,
        isExpired_,
        removeWhenExpired_,
        lastOkTransitionAt_,
        lastWarningTransitionAt_,
        lastErrorTransitionAt_);
}
