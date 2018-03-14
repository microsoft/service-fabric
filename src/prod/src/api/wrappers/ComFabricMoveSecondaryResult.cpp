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

ComFabricMoveSecondaryResult::ComFabricMoveSecondaryResult(
    IMoveSecondaryResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_MOVE_SECONDARY_RESULT * STDMETHODCALLTYPE ComFabricMoveSecondaryResult::get_Result()
{
    auto result = impl_->GetMoveSecondaryResult();
    auto moveSecondaryResultPtr = heap_.AddItem<FABRIC_MOVE_SECONDARY_RESULT>();
    result->ToPublicApi(heap_, *moveSecondaryResultPtr);

    return moveSecondaryResultPtr.GetRawPointer();
}
