// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServicePackage.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ServicePackage::FormatHeader = make_global<wstring>(L"Service package");

ServicePackage::ServicePackage(ServicePackageDescription && description)
    :description_(move(description))
{
}

ServicePackage::ServicePackage(ServicePackage && other)
    : description_(move(other.description_)),
    services_(move(other.services_))
{
}

bool Reliability::LoadBalancingComponent::ServicePackage::UpdateDescription(ServicePackageDescription && description)
{
    if (description_ != description)
    {
        description_ = move(description);
        return true;
    }

    return false;
}


