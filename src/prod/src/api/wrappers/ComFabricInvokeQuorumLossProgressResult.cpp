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

ComFabricInvokeQuorumLossProgressResult::ComFabricInvokeQuorumLossProgressResult(
    IInvokeQuorumLossProgressResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_PARTITION_QUORUM_LOSS_PROGRESS * STDMETHODCALLTYPE ComFabricInvokeQuorumLossProgressResult::get_Progress()
{
    auto progress = impl_->GetProgress();
    auto invokeQuorumLossProgressPtr = heap_.AddItem<FABRIC_PARTITION_QUORUM_LOSS_PROGRESS>();
    progress->ToPublicApi(heap_, *invokeQuorumLossProgressPtr);

    return invokeQuorumLossProgressPtr.GetRawPointer();
}

