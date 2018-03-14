// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComGetServiceNameResult::ComGetServiceNameResult(
    ServiceNameQueryResult && serviceName)
    : heap_()
    , serviceName_()
{
    serviceName_ = heap_.AddItem<FABRIC_SERVICE_NAME_QUERY_RESULT>();
    serviceName.ToPublicApi(heap_, *serviceName_);
}

ComGetServiceNameResult::~ComGetServiceNameResult()
{
}

const FABRIC_SERVICE_NAME_QUERY_RESULT * STDMETHODCALLTYPE ComGetServiceNameResult::get_ServiceName()
{
    return serviceName_.GetRawPointer();
}
