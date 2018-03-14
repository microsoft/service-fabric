// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComDeployedServicePackageHealthResult::ComDeployedServicePackageHealthResult(
    DeployedServicePackageHealth const & deployedServicePackageHealth)
    : IFabricDeployedServicePackageHealthResult()
    , ComUnknownBase()
    , heap_()
    , deployedServicePackageHealth_()
{
    deployedServicePackageHealth_ = heap_.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH>();
    deployedServicePackageHealth.ToPublicApi(heap_, *deployedServicePackageHealth_);
}

ComDeployedServicePackageHealthResult::~ComDeployedServicePackageHealthResult()
{
}

const FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH * STDMETHODCALLTYPE ComDeployedServicePackageHealthResult::get_DeployedServicePackageHealth()
{
    return deployedServicePackageHealth_.GetRawPointer();
}
