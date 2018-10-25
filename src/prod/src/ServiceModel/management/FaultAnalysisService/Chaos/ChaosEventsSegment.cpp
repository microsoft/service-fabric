
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosEventsSegment");

ChaosEventsSegment::ChaosEventsSegment()
    : continuationToken_(L"")
    , history_()
{
}

ChaosEventsSegment::ChaosEventsSegment(ChaosReport && other)
    : continuationToken_(move(other.ContinuationToken))
    , history_(move(other.History))
{
}

ErrorCode ChaosEventsSegment::FromPublicApi(
    FABRIC_CHAOS_EVENTS_SEGMENT const & publicEventsSegment)
{
    TRY_PARSE_PUBLIC_STRING_OUT(publicEventsSegment.ContinuationToken, continuationToken_, true);

    auto error = PublicApiHelper::FromPublicApiList<ChaosEvent, FABRIC_CHAOS_EVENT_LIST>(
        publicEventsSegment.History,
        history_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosEventsSegment::FromPublicApi/Failed to retrieve the chaos event list: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosEventsSegment::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENTS_SEGMENT & result) const
{
    result.ContinuationToken = heap.AddString(continuationToken_);

    // Start of ToPublic of vector of chaos events
    auto publicChaosEventList = heap.AddItem<FABRIC_CHAOS_EVENT_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<ChaosEvent, FABRIC_CHAOS_EVENT, FABRIC_CHAOS_EVENT_LIST>(
        heap,
        history_,
        *publicChaosEventList);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosEventsSegment::ToPublicApi/Failed to publish the list of Chaos events with error: {0}", error);
        return error;
    }

    result.History = publicChaosEventList.GetRawPointer();

    return ErrorCodeValue::Success;
}
