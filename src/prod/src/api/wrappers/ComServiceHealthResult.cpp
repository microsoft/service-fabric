// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComServiceHealthResult::ComServiceHealthResult(
    ServiceHealth const & serviceHealth)
    : IFabricServiceHealthResult()
    , ComUnknownBase()
    , heap_()
    , serviceHealth_()
{
    serviceHealth_ = heap_.AddItem<FABRIC_SERVICE_HEALTH>();
    serviceHealth.ToPublicApi(heap_, *serviceHealth_);
}

ComServiceHealthResult::~ComServiceHealthResult()
{
}

const FABRIC_SERVICE_HEALTH * STDMETHODCALLTYPE ComServiceHealthResult::get_ServiceHealth()
{
    return serviceHealth_.GetRawPointer();
}
