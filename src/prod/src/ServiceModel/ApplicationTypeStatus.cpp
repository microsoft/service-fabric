// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

std::wstring ServiceModel::ApplicationTypeStatus::ToString(Enum const & val)
{
    wstring applicationTypeStatus;
    StringWriter(applicationTypeStatus).Write(val);
    return applicationTypeStatus;
}   

void ServiceModel::ApplicationTypeStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid:
            w << L"Invalid";
            return;
        case Provisioning: 
            w << L"Provisioning";
            return;
        case Available: 
            w << L"Available";
            return;
        case Unprovisioning: 
            w << L"Unprovisioning";
            return;
        case Failed:
            w << L"Failed";
            return;
        default: 
            w << L"Unknown Application Type Status {0}" << static_cast<int>(val);
            return;
    }
}

::FABRIC_APPLICATION_TYPE_STATUS ApplicationTypeStatus::ToPublicApi(ApplicationTypeStatus::Enum toConvert)
{
    switch(toConvert)
    {
    case Provisioning:
        return ::FABRIC_APPLICATION_TYPE_STATUS_PROVISIONING;
    case Available:
        return ::FABRIC_APPLICATION_TYPE_STATUS_AVAILABLE;
    case Unprovisioning:
        return ::FABRIC_APPLICATION_TYPE_STATUS_UNPROVISIONING;
    case Failed:
        return ::FABRIC_APPLICATION_TYPE_STATUS_FAILED;
    case Invalid:
    default:
        return ::FABRIC_APPLICATION_TYPE_STATUS_INVALID;
    }
}

ApplicationTypeStatus::Enum ApplicationTypeStatus::FromPublicApi(FABRIC_APPLICATION_TYPE_STATUS toConvert)
{
    switch(toConvert)
    {
    case FABRIC_APPLICATION_TYPE_STATUS_PROVISIONING:
        return Enum::Provisioning;
    case FABRIC_APPLICATION_TYPE_STATUS_AVAILABLE:
        return Enum::Available;
    case FABRIC_APPLICATION_TYPE_STATUS_UNPROVISIONING:
        return Enum::Unprovisioning;
    case FABRIC_APPLICATION_TYPE_STATUS_FAILED:
        return Enum::Failed;
    case FABRIC_APPLICATION_TYPE_STATUS_INVALID:
    default:
        return Enum::Invalid;
    }
}
