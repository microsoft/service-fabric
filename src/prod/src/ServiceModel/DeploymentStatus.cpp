// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

std::wstring ServiceModel::DeploymentStatus::ToString(Enum const & val)
{
    wstring deploymentStatus;
    StringWriter(deploymentStatus).Write(val);
    return deploymentStatus;
}   

void ServiceModel::DeploymentStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid: 
            w << L"Invalid";
            return;
        case Downloading: 
            w << L"Downloading";
            return;
        case Activating: 
            w << L"Activating";
            return;
        case Active: 
            w << L"Active";
            return;
        case Upgrading: 
            w << L"Upgrading";
            return;
        case Deactivating: 
            w << L"Deactivating";
            return;
        default: 
            Common::Assert::CodingError("Unknown Deployment Status");
    }
}

FABRIC_DEPLOYMENT_STATUS DeploymentStatus::ToPublicApi(DeploymentStatus::Enum const & val)
{
    switch(val)
    {
        case Invalid: 
            return FABRIC_DEPLOYMENT_STATUS_INVALID;
        case Downloading: 
            return FABRIC_DEPLOYMENT_STATUS_DOWNLOADING;
        case Activating: 
            return FABRIC_DEPLOYMENT_STATUS_ACTIVATING;
        case Active: 
            return FABRIC_DEPLOYMENT_STATUS_ACTIVE;
        case Upgrading: 
            return FABRIC_DEPLOYMENT_STATUS_UPGRADING;
        case Deactivating: 
            return FABRIC_DEPLOYMENT_STATUS_DEACTIVATING;
        default: 
            Common::Assert::CodingError("Unknown Deployment Status");    
    }
}

ErrorCode DeploymentStatus::FromPublicApi(FABRIC_DEPLOYMENT_STATUS const & publicVal, __out Enum & val)
{
    switch(publicVal)
    {
    case FABRIC_APPLICATION_STATUS_INVALID:
        val = DeploymentStatus::Invalid;
        break;
    case FABRIC_DEPLOYMENT_STATUS_DOWNLOADING:
        val = DeploymentStatus::Downloading;
        break;
    case FABRIC_DEPLOYMENT_STATUS_ACTIVATING:
        val = DeploymentStatus::Activating;
        break;
    case FABRIC_DEPLOYMENT_STATUS_ACTIVE:
        val = DeploymentStatus::Active;
        break;
    case FABRIC_DEPLOYMENT_STATUS_UPGRADING:
        val = DeploymentStatus::Upgrading;
        break;
    case FABRIC_DEPLOYMENT_STATUS_DEACTIVATING:
        val = DeploymentStatus::Deactivating;
        break;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCodeValue::Success;
}

