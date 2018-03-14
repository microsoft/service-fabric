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

ComFabricInvokeDataLossProgressResult::ComFabricInvokeDataLossProgressResult(
    IInvokeDataLossProgressResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_PARTITION_DATA_LOSS_PROGRESS * STDMETHODCALLTYPE ComFabricInvokeDataLossProgressResult::get_Progress()
{
    auto result = impl_->GetProgress();
    auto invokeDataLossProgressPtr = heap_.AddItem<FABRIC_PARTITION_DATA_LOSS_PROGRESS>();
    result->ToPublicApi(heap_, *invokeDataLossProgressPtr);

    return invokeDataLossProgressPtr.GetRawPointer();
}
