// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceType.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServiceType::ServiceType(ServiceTypeDescription && desc)
    : serviceTypeDesc_(move(desc))
{
}

ServiceType::ServiceType(ServiceType && other)
    : serviceTypeDesc_(move(other.serviceTypeDesc_))
{
}

bool ServiceType::UpdateDescription(ServiceTypeDescription && desc)
{
    if (serviceTypeDesc_ != desc)
    {
        serviceTypeDesc_ = move(desc);
        return true;
    }
    else
    {
        return false;
    }
}

void ServiceType::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0}", serviceTypeDesc_);
}
