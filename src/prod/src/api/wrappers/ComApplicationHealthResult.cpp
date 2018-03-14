// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComApplicationHealthResult::ComApplicationHealthResult(
    ApplicationHealth const & applicationHealth)
    : IFabricApplicationHealthResult()
    , ComUnknownBase()
    , heap_()
    , applicationHealth_()
{
    applicationHealth_ = heap_.AddItem<FABRIC_APPLICATION_HEALTH>();
    applicationHealth.ToPublicApi(heap_, *applicationHealth_);
}

ComApplicationHealthResult::~ComApplicationHealthResult()
{
}

const FABRIC_APPLICATION_HEALTH * STDMETHODCALLTYPE ComApplicationHealthResult::get_ApplicationHealth()
{
    return applicationHealth_.GetRawPointer();
}
