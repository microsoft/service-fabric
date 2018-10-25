
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosScheduleTimeUtc");

ChaosScheduleTimeUtc::ChaosScheduleTimeUtc()
    : hour_()
    , minute_()
{
}

ErrorCode ChaosScheduleTimeUtc::FromPublicApi(
	FABRIC_CHAOS_SCHEDULE_TIME_UTC const & publicTimeRange)
{
	hour_ = publicTimeRange.Hour;
	minute_ = publicTimeRange.Minute;
    return ErrorCodeValue::Success;
}

ErrorCode ChaosScheduleTimeUtc::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE_TIME_UTC & result) const
{
    (void) heap;

    result.Hour = hour_;
    result.Minute = minute_;

    return ErrorCodeValue::Success;
}
