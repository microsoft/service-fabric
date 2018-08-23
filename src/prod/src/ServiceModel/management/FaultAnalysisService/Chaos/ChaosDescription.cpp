
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ChaosDescription");

ChaosDescription::ChaosDescription()
    : chaosParametersUPtr_()
    , status_()
    , scheduleStatus_()
{
}

ErrorCode ChaosDescription::FromPublicApi(
    FABRIC_CHAOS_DESCRIPTION const & publicDescription)
{
    ChaosParameters chaosParameters;
    auto error = chaosParameters.FromPublicApi(*publicDescription.ChaosParameters);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ChaosDescription::FromPublicApi/Failed at chaosParameters.FromPublicApi");
        return error;
    }

    chaosParametersUPtr_ = make_unique<ChaosParameters>(move(chaosParameters));

    error = ChaosStatus::FromPublicApi(publicDescription.Status, status_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosStatus::FromPublicApi failed, error: {0}", error);
        return error;
    }

    error = ChaosScheduleStatus::FromPublicApi(publicDescription.ScheduleStatus, scheduleStatus_);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "ChaosScheduleStatus::FromPublicApi failed, error: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ChaosDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_DESCRIPTION & result) const
{
    auto publicChaosParametersPtr = heap.AddItem<FABRIC_CHAOS_PARAMETERS>();
    chaosParametersUPtr_->ToPublicApi(heap, *publicChaosParametersPtr);
    result.ChaosParameters = publicChaosParametersPtr.GetRawPointer();

    result.Status = ChaosStatus::ToPublicApi(status_);

    result.ScheduleStatus = ChaosScheduleStatus::ToPublicApi(scheduleStatus_);

    return ErrorCodeValue::Success;
}