
//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("SetChaosScheduleDescription");

SetChaosScheduleDescription::SetChaosScheduleDescription()
: chaosScheduleDescriptionUPtr_()
{
}

SetChaosScheduleDescription::SetChaosScheduleDescription(SetChaosScheduleDescription && other)
: chaosScheduleDescriptionUPtr_(move(other.chaosScheduleDescriptionUPtr_))
{
}

SetChaosScheduleDescription & SetChaosScheduleDescription::operator =(SetChaosScheduleDescription && other)
{
    if (this != &other)
    {
        chaosScheduleDescriptionUPtr_ = move(other.chaosScheduleDescriptionUPtr_);
    }

    return *this;
}

SetChaosScheduleDescription::SetChaosScheduleDescription(std::unique_ptr<ChaosScheduleDescription> && chaosScheduleDescription)
: chaosScheduleDescriptionUPtr_(move(chaosScheduleDescription))
{
}

Common::ErrorCode SetChaosScheduleDescription::FromPublicApi(
    FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION const & publicDescription)
{
    ChaosScheduleDescription chaosScheduleDescription;
    auto error = chaosScheduleDescription.FromPublicApi(*publicDescription.ChaosScheduleDescription);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "SetChaosScheduleDescription::FromPublicApi/Failed at chaosScheduleDescription.FromPublicApi");
        return error;
    }

    chaosScheduleDescriptionUPtr_ = make_unique<ChaosScheduleDescription>(move(chaosScheduleDescription));

    return ErrorCodeValue::Success;
}

void SetChaosScheduleDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION & result) const
{
    auto publicChaosScheduleDescriptionPtr = heap.AddItem<FABRIC_CHAOS_SCHEDULE_DESCRIPTION>();
    chaosScheduleDescriptionUPtr_->ToPublicApi(heap, *publicChaosScheduleDescriptionPtr);
    result.ChaosScheduleDescription = publicChaosScheduleDescriptionPtr.GetRawPointer();
}
