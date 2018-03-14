// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceMeasurementRequest.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

GlobalWString ResourceMeasurementRequest::ResourceMeasureRequestAction = make_global<wstring>(L"ResourceMeasureRequestAction");

ResourceMeasurementRequest::ResourceMeasurementRequest(std::vector<std::wstring> && hosts, std::wstring const & nodeId)
    : hosts_(move(hosts)),
      nodeId_(nodeId)
{
}
