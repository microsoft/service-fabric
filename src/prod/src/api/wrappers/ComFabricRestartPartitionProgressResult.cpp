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

ComFabricRestartPartitionProgressResult::ComFabricRestartPartitionProgressResult(
    IRestartPartitionProgressResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_PARTITION_RESTART_PROGRESS * STDMETHODCALLTYPE ComFabricRestartPartitionProgressResult::get_Progress()
{
    auto progress = impl_->GetProgress();
    auto restartPartitionProgressPtr = heap_.AddItem<FABRIC_PARTITION_RESTART_PROGRESS>();
    progress->ToPublicApi(heap_, *restartPartitionProgressPtr);

    return restartPartitionProgressPtr.GetRawPointer();
}
