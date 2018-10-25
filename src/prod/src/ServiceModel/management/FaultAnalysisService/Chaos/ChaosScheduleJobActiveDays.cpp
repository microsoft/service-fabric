
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosScheduleJobActiveDays");

ChaosScheduleJobActiveDays::ChaosScheduleJobActiveDays()
    : sunday_()
    , monday_()
    , tuesday_()
    , wednesday_()
    , thursday_()
    , friday_()
    , saturday_()
{
}

ErrorCode ChaosScheduleJobActiveDays::FromPublicApi(
    FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS const & publicDays)
{
    sunday_ = publicDays.Sunday;
    monday_ = publicDays.Monday;
    tuesday_ = publicDays.Tuesday;
    wednesday_ = publicDays.Wednesday;
    thursday_ = publicDays.Thursday;
    friday_ = publicDays.Friday;
    saturday_ = publicDays.Saturday;

    return ErrorCodeValue::Success;
}

ErrorCode ChaosScheduleJobActiveDays::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS & result) const
{
    UNREFERENCED_PARAMETER(heap);

    result.Sunday = sunday_;
    result.Monday = monday_;
    result.Tuesday = tuesday_;
    result.Wednesday = wednesday_;
    result.Thursday = thursday_;
    result.Friday = friday_;
    result.Saturday = saturday_;

    return ErrorCodeValue::Success;
}