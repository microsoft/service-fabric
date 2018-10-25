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

ComFabricChaosEventsSegmentResult::ComFabricChaosEventsSegmentResult(
    IChaosEventsSegmentResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_CHAOS_EVENTS_SEGMENT * STDMETHODCALLTYPE ComFabricChaosEventsSegmentResult::get_ChaosEventsSegmentResult()
{
    auto eventsSegment = impl_->GetChaosEvents();
    auto chaosEventsSegmentPtr = heap_.AddItem<FABRIC_CHAOS_EVENTS_SEGMENT>();
    eventsSegment->ToPublicApi(heap_, *chaosEventsSegmentPtr);

    return chaosEventsSegmentPtr.GetRawPointer();
}
