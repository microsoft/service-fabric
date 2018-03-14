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

ComFabricRestartDeployedCodePackageResult::ComFabricRestartDeployedCodePackageResult(
    IRestartDeployedCodePackageResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_DEPLOYED_CODE_PACKAGE_RESULT * STDMETHODCALLTYPE ComFabricRestartDeployedCodePackageResult::get_Result()
{
    auto result = impl_->GetRestartDeployedCodePackageResult();
    auto restartDeployedCodePackageResultPtr = heap_.AddItem<FABRIC_DEPLOYED_CODE_PACKAGE_RESULT>();
    result->ToPublicApi(heap_, *restartDeployedCodePackageResultPtr);

    return restartDeployedCodePackageResultPtr.GetRawPointer();
}
