//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

ComFabricChaosScheduleDescriptionResult::ComFabricChaosScheduleDescriptionResult(
    IChaosScheduleDescriptionResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_CHAOS_SCHEDULE_DESCRIPTION * STDMETHODCALLTYPE ComFabricChaosScheduleDescriptionResult::get_ChaosScheduleDescriptionResult()
{
    auto description = impl_->GetChaosSchedule();
    auto chaosScheduleDescriptionPtr = heap_.AddItem<FABRIC_CHAOS_SCHEDULE_DESCRIPTION>();
    description->ToPublicApi(heap_, *chaosScheduleDescriptionPtr);

    return chaosScheduleDescriptionPtr.GetRawPointer();
}
