// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosReportFilter");

ChaosReportFilter::ChaosReportFilter()
: startTimeUtc_()
, endTimeUtc_()
{
}

ChaosReportFilter::ChaosReportFilter(ChaosReportFilter const & other)
: startTimeUtc_(other.startTimeUtc_)
, endTimeUtc_(other.endTimeUtc_)
{
}

ChaosReportFilter::ChaosReportFilter(ChaosReportFilter && other)
: startTimeUtc_(move(other.startTimeUtc_))
, endTimeUtc_(move(other.endTimeUtc_))
{
}

ChaosReportFilter::ChaosReportFilter(
    DateTime const& startTimeUtc,
    DateTime const& endTimeUtc)
    : startTimeUtc_(startTimeUtc)
    , endTimeUtc_(endTimeUtc)
{
}

Common::ErrorCode ChaosReportFilter::FromPublicApi(
    FABRIC_CHAOS_REPORT_FILTER const & publicFilter)
{
    startTimeUtc_ = DateTime(publicFilter.StartTimeUtc);
    endTimeUtc_ = DateTime(publicFilter.EndTimeUtc);

    return ErrorCodeValue::Success;
}

void ChaosReportFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_REPORT_FILTER & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.StartTimeUtc = startTimeUtc_.AsFileTime;
    result.EndTimeUtc = endTimeUtc_.AsFileTime;
}

Common::ErrorCode ChaosReportFilter::FromPublicApi(
    FABRIC_CHAOS_EVENTS_SEGMENT_FILTER const & publicFilter)
{
    startTimeUtc_ = DateTime(publicFilter.StartTimeUtc);
    endTimeUtc_ = DateTime(publicFilter.EndTimeUtc);

    return ErrorCodeValue::Success;
}

void ChaosReportFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENTS_SEGMENT_FILTER & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.StartTimeUtc = startTimeUtc_.AsFileTime;
    result.EndTimeUtc = endTimeUtc_.AsFileTime;
}
