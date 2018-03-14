// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

ComGatewayInformationResult::ComGatewayInformationResult(Naming::GatewayDescription const & impl)
    : impl_(impl)
{
}

ComGatewayInformationResult::~ComGatewayInformationResult()
{
}

const FABRIC_GATEWAY_INFORMATION * STDMETHODCALLTYPE ComGatewayInformationResult::get_GatewayInformation()
{
    auto publicInformation = heap_.AddItem<FABRIC_GATEWAY_INFORMATION>();
    impl_.ToPublicApi(heap_, *publicInformation);

    return publicInformation.GetRawPointer();
}
