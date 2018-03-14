// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

ComFabricNodeTransitionProgressResult::ComFabricNodeTransitionProgressResult(
    INodeTransitionProgressResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_NODE_TRANSITION_PROGRESS * STDMETHODCALLTYPE ComFabricNodeTransitionProgressResult::get_Progress()
{
    auto progress = impl_->GetProgress();
    auto progressPtr = heap_.AddItem<FABRIC_NODE_TRANSITION_PROGRESS>();
    progress->ToPublicApi(heap_, *progressPtr);

    return progressPtr.GetRawPointer();
}
