
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosScheduleTimeRangeUtc");

ChaosScheduleTimeRangeUtc::ChaosScheduleTimeRangeUtc()
    : startTime_()
    , endTime_()
{
}

ErrorCode ChaosScheduleTimeRangeUtc::FromPublicApi(
    FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC const & publicDescription)
{
    auto error = startTime_.FromPublicApi(*publicDescription.StartTime);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosScheduleTimeRangeUtc::FromPublicApi/Failed at startTime_.FromPublicApi");
        return error;
    }

    error = endTime_.FromPublicApi(*publicDescription.EndTime);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosScheduleTimeRangeUtc::FromPublicApi/Failed at endTime_.FromPublicApi");
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosScheduleTimeRangeUtc::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC & result) const
{
    auto publicStartTimePtr = heap.AddItem<FABRIC_CHAOS_SCHEDULE_TIME_UTC>();
    auto publicEndTimePtr = heap.AddItem<FABRIC_CHAOS_SCHEDULE_TIME_UTC>();
    startTime_.ToPublicApi(heap, *publicStartTimePtr);
    endTime_.ToPublicApi(heap, *publicEndTimePtr);

    result.StartTime = publicStartTimePtr.GetRawPointer();
    result.EndTime = publicEndTimePtr.GetRawPointer();

    return ErrorCodeValue::Success;
}
