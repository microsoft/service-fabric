// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

void ServiceTypeStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

wstring ServiceTypeStatus::ToString(Enum const & val)
{
    switch(val)
    {
    case ServiceTypeStatus::Invalid:
        return L"Invalid";
    case ServiceTypeStatus::NotRegistered_Enabled:
        return L"NotRegistered_Enabled";
    case ServiceTypeStatus::InProgress_Enabled:
        return L"InProgress_Enabled";
    case ServiceTypeStatus::Registered_Enabled:
        return L"Registered_Enabled";
    case ServiceTypeStatus::NotRegistered_Disabled:
        return L"NotRegistered_Disabled";
    case ServiceTypeStatus::InProgress_Disabled:
        return L"InProgress_Disabled";
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
        return L"NotRegistered_DisableScheduled";
    case ServiceTypeStatus::Closed:
        return L"Closed";
    default:
        Assert::CodingError("Unknown ServiceTypeStatus value {0}", (int)val);
    }
}

FABRIC_SERVICE_TYPE_REGISTRATION_STATUS ServiceTypeStatus::ToPublicRegistrationStatus(ServiceTypeStatus::Enum serviceTypeStatus)
{
    switch(serviceTypeStatus)
    {
    case ServiceTypeStatus::Invalid:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_INVALID;
    case ServiceTypeStatus::NotRegistered_Enabled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED;
    case ServiceTypeStatus::InProgress_Enabled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED;
    case ServiceTypeStatus::Registered_Enabled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_REGISTERED;
    case ServiceTypeStatus::NotRegistered_Disabled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_DISABLED;
    case ServiceTypeStatus::InProgress_Disabled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_DISABLED;
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED;
    default:
        return FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_INVALID;
    }
}
