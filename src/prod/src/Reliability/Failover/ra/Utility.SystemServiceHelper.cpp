// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ServiceModel;
using namespace Utility2;

SystemServiceHelper::HostType::Enum SystemServiceHelper::GetHostType(FailoverUnit const & ft)
{
    return GetHostType(ft.FailoverUnitDescription, ft.ServiceDescription.ApplicationName);
}

SystemServiceHelper::HostType::Enum SystemServiceHelper::GetHostType(
    FailoverUnitDescription const & ftDesc,
    wstring const & appName)
{
    // AppName is empty for all system services in the RA so first use the ft Id to identify host type
    if (ftDesc.ConsistencyUnitDescription.ConsistencyUnitId.IsRepairManager())
    {
        // Repair manager has a reserved guid but is not hosted within fabric.exe
        return HostType::Managed;
    }

    if (ftDesc.IsReservedCUID())
    {        
        return HostType::Fabric;
    }

    if (appName.empty())
    {
        return HostType::AdHoc;
    }

    return HostType::Managed;
}

SystemServiceHelper::ServiceType::Enum SystemServiceHelper::GetServiceType(
    std::wstring const & serviceName)
{
    return SystemServiceApplicationNameHelper::IsInternalServiceName(serviceName) ? ServiceType::System : ServiceType::User;
}

SystemServiceHelper::ServiceType::Enum SystemServiceHelper::GetServiceType(
    FailoverUnit const & ft)
{
    return GetServiceType(ft.ServiceDescription.Name);
}
