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

ComFabricMovePrimaryResult::ComFabricMovePrimaryResult(
    IMovePrimaryResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_MOVE_PRIMARY_RESULT * STDMETHODCALLTYPE ComFabricMovePrimaryResult::get_Result()
{
    auto result = impl_->GetMovePrimaryResult();
    auto movePrimaryResultPtr = heap_.AddItem<FABRIC_MOVE_PRIMARY_RESULT>();
    result->ToPublicApi(heap_, *movePrimaryResultPtr);

    return movePrimaryResultPtr.GetRawPointer();
}
