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

ComFabricChaosDescriptionResult::ComFabricChaosDescriptionResult(
    IChaosDescriptionResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_CHAOS_DESCRIPTION * STDMETHODCALLTYPE ComFabricChaosDescriptionResult::get_ChaosDescriptionResult()
{
    auto chaosDescription = impl_->GetChaos();
    auto chaosDescriptionPtr = heap_.AddItem<FABRIC_CHAOS_DESCRIPTION>();
    chaosDescription->ToPublicApi(heap_, *chaosDescriptionPtr);

    return chaosDescriptionPtr.GetRawPointer();
}
