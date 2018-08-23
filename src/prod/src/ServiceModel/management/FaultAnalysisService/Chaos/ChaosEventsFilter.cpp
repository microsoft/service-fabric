//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosEventsFilter");

ChaosEventsFilter::ChaosEventsFilter()
: startTimeUtc_()
, endTimeUtc_()
{
}

ChaosEventsFilter::ChaosEventsFilter(
    DateTime const& startTimeUtc,
    DateTime const& endTimeUtc)
    : startTimeUtc_(startTimeUtc)
    , endTimeUtc_(endTimeUtc)
{
}

Common::ErrorCode ChaosEventsFilter::FromPublicApi(
    FABRIC_CHAOS_EVENTS_SEGMENT_FILTER const & publicFilter)
{
    startTimeUtc_ = DateTime(publicFilter.StartTimeUtc);
    endTimeUtc_ = DateTime(publicFilter.EndTimeUtc);

    return ErrorCodeValue::Success;
}

void ChaosEventsFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENTS_SEGMENT_FILTER & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.StartTimeUtc = startTimeUtc_.AsFileTime;
    result.EndTimeUtc = endTimeUtc_.AsFileTime;
}
