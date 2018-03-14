// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

std::wstring ServiceModel::EntryPointStatus::ToString(Enum const & val)
{
    wstring entryPointStatus;
    StringWriter(entryPointStatus).Write(val);
    return entryPointStatus;
}   


void ServiceModel::EntryPointStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid: 
            w << L"Invalid";
            return;
        case Pending: 
            w << L"Pending";
            return;
        case Starting: 
            w << L"Starting";
            return;
        case Started: 
            w << L"Started";        
            return;
        case Stopping: 
            w << L"Stopping";
            return;
        case Stopped: 
            w << L"Stopped";
            return;
        default: 
            Common::Assert::CodingError("Unknown EntryPoint Status");
    }
}

FABRIC_ENTRY_POINT_STATUS EntryPointStatus::ToPublicApi(EntryPointStatus::Enum const & val)
{
    switch(val)
    {
        case Invalid: 
            return FABRIC_ENTRY_POINT_STATUS_INVALID;
        case Pending: 
            return FABRIC_ENTRY_POINT_STATUS_PENDING;
        case Starting: 
            return FABRIC_ENTRY_POINT_STATUS_STARTING;
        case Started: 
            return FABRIC_ENTRY_POINT_STATUS_STARTED;
        case Stopping: 
            return FABRIC_ENTRY_POINT_STATUS_STOPPING;
        case Stopped: 
            return FABRIC_ENTRY_POINT_STATUS_STOPPED;
        default: 
            Common::Assert::CodingError("Unknown Deployment Status");    
    }
}

ErrorCode EntryPointStatus::FromPublicApi(FABRIC_ENTRY_POINT_STATUS const & publicVal, __out Enum & val)
{
    switch(publicVal)
    {
    case FABRIC_APPLICATION_STATUS_INVALID:
        val = EntryPointStatus::Invalid;
        break;
    case FABRIC_ENTRY_POINT_STATUS_PENDING:
        val = EntryPointStatus::Pending;
        break;
    case FABRIC_ENTRY_POINT_STATUS_STARTING:
        val = EntryPointStatus::Starting;
        break;
    case FABRIC_ENTRY_POINT_STATUS_STARTED:
        val = EntryPointStatus::Started;
        break;
    case FABRIC_ENTRY_POINT_STATUS_STOPPING:
        val = EntryPointStatus::Stopping;
        break;
    case FABRIC_ENTRY_POINT_STATUS_STOPPED:
        val = EntryPointStatus::Stopped;
        break;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCodeValue::Success;
}

