// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComDeployedApplicationHealthResult::ComDeployedApplicationHealthResult(
    DeployedApplicationHealth const & deployedApplicationHealth)
    : IFabricDeployedApplicationHealthResult()
    , ComUnknownBase()
    , heap_()
    , deployedApplicationHealth_()
{
    deployedApplicationHealth_ = heap_.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH>();
    deployedApplicationHealth.ToPublicApi(heap_, *deployedApplicationHealth_);
}

ComDeployedApplicationHealthResult::~ComDeployedApplicationHealthResult()
{
}

const FABRIC_DEPLOYED_APPLICATION_HEALTH * STDMETHODCALLTYPE ComDeployedApplicationHealthResult::get_DeployedApplicationHealth()
{
    return deployedApplicationHealth_.GetRawPointer();
}
