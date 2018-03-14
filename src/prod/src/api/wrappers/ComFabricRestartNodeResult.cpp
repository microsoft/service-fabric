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

ComFabricRestartNodeResult::ComFabricRestartNodeResult(
    IRestartNodeResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_NODE_RESULT * STDMETHODCALLTYPE ComFabricRestartNodeResult::get_Result()
{
    auto result = impl_->GetNodeResult();
    auto restartNodeResultPtr = heap_.AddItem<FABRIC_NODE_RESULT>();
    result->ToPublicApi(heap_, *restartNodeResultPtr);

    return restartNodeResultPtr.GetRawPointer();
}
