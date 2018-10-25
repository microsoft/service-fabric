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

StringLiteral const TraceComponent("ChaosScheduleDescription");

ChaosScheduleDescription::ChaosScheduleDescription()
    : version_()
    , scheduleUPtr_()
{
}

ErrorCode ChaosScheduleDescription::FromPublicApi(
    FABRIC_CHAOS_SCHEDULE_DESCRIPTION const & publicDescription)
{
    ChaosSchedule chaosSchedule;
    auto error = chaosSchedule.FromPublicApi(*publicDescription.Schedule);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosScheduleDescription::FromPublicApi/Failed at chaosSchedule.FromPublicApi");
    }

    scheduleUPtr_ = make_unique<ChaosSchedule>(move(chaosSchedule));

    version_ = publicDescription.Version;

    return ErrorCodeValue::Success;
}

ErrorCode ChaosScheduleDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SCHEDULE_DESCRIPTION & result) const
{
    auto publicChaosSchedulePtr = heap.AddItem<FABRIC_CHAOS_SCHEDULE>();
    scheduleUPtr_->ToPublicApi(heap, *publicChaosSchedulePtr);
    result.Schedule = publicChaosSchedulePtr.GetRawPointer();

    result.Version = version_;

    return ErrorCodeValue::Success;
}