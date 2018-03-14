// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceMeasurement.h"
#include "ResourceMeasurementResponse.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

ResourceMeasurementResponse::ResourceMeasurementResponse(std::map<std::wstring, ResourceMeasurement> && resourceMeasurementResponse)
    : resourceMeasurementResponse_(move(resourceMeasurementResponse))
{
}

