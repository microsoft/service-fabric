// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

std::wstring ServiceModel::ApplicationStatus::ToString(Enum const & val)
{
    wstring applicationStatus;
    StringWriter(applicationStatus).Write(val);
    return applicationStatus;
}   

void ServiceModel::ApplicationStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid:
            w << L"Invalid";
            return;
        case Ready: 
            w << L"Ready";
            return;
        case Upgrading: 
            w << L"Upgrading";
            return;
        case Creating: 
            w << L"Creating";
            return;
        case Deleting: 
            w << L"Deleting";
            return;
        case Failed:
            w << L"Failed";
            return;
        default: 
            w << L"Unknown Application Status {0}" << static_cast<int>(val);
            return;
    }
}

::FABRIC_APPLICATION_STATUS ApplicationStatus::ConvertToPublicApplicationStatus(ApplicationStatus::Enum toConvert)
{
    switch(toConvert)
    {
    case Ready:
        return ::FABRIC_APPLICATION_STATUS_READY;
    case Upgrading:
        return ::FABRIC_APPLICATION_STATUS_UPGRADING;
    case Creating:
        return ::FABRIC_APPLICATION_STATUS_CREATING;
    case Deleting:
        return ::FABRIC_APPLICATION_STATUS_DELETING;
    case Failed:
        return ::FABRIC_APPLICATION_STATUS_FAILED;
    case Invalid:
    default:
        return ::FABRIC_APPLICATION_STATUS_INVALID;
    }
}

ApplicationStatus::Enum ApplicationStatus::ConvertToServiceModelApplicationStatus(FABRIC_APPLICATION_STATUS toConvert)
{
    switch(toConvert)
    {
    case FABRIC_APPLICATION_STATUS_READY:
        return Enum::Ready;
    case FABRIC_APPLICATION_STATUS_UPGRADING:
        return Enum::Upgrading;
    case FABRIC_APPLICATION_STATUS_CREATING:
        return Enum::Creating;
    case FABRIC_APPLICATION_STATUS_DELETING:
        return Enum::Deleting;
    case FABRIC_APPLICATION_STATUS_FAILED:
        return Enum::Failed;
    case FABRIC_APPLICATION_STATUS_INVALID:
    default:
        return Enum::Invalid;
    }
}

