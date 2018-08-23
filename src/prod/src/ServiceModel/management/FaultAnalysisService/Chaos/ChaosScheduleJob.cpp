
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosScheduleJob");

ChaosScheduleJob::ChaosScheduleJob()
    : chaosParametersReferenceKey_()
    , daysUPtr_()
    , times_()
{
}

ErrorCode ChaosScheduleJob::FromPublicApi(
    FABRIC_CHAOS_SCHEDULE_JOB const & publicJob)
{
    TRY_PARSE_PUBLIC_STRING_OUT(publicJob.ChaosParameters, chaosParametersReferenceKey_, true);

    ChaosScheduleJobActiveDays activeDays;
    auto error = activeDays.FromPublicApi(*publicJob.Days);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosScheduleJob::FromPublicApi/Failed at activeDays.FromPublicApi");
        return error;
    }
	daysUPtr_ = make_unique<ChaosScheduleJobActiveDays>(move(activeDays));

    error = PublicApiHelper::FromPublicApiList<ChaosScheduleTimeRangeUtc, FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC_LIST>(
        publicJob.Times,
        times_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosScheduleJob::FromPublicApi/Failed to retrieve the chaos schedule time range list: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosScheduleJob::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE_JOB & result) const
{
    result.ChaosParameters = heap.AddString(chaosParametersReferenceKey_);

    auto publicActiveDaysPtr = heap.AddItem<FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS>();
    daysUPtr_->ToPublicApi(heap, *publicActiveDaysPtr);
    result.Days = publicActiveDaysPtr.GetRawPointer();

    auto publicActiveTimes = heap.AddItem<FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<ChaosScheduleTimeRangeUtc, FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC, FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC_LIST>(
        heap,
        times_,
        *publicActiveTimes);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosScheduleJob::ToPublicApi/Failed to publish the list of Chaos schedule time ranges with error: {0}", error);
        return error;
    }

    result.Times = publicActiveTimes.GetRawPointer();
    return ErrorCodeValue::Success;
}