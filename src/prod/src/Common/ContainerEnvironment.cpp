// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/ContainerEnvironment.h"

Common::GlobalWString Common::ContainerEnvironment::IsContainerHostEnvironmentVariableName = make_global<wstring>(L"Fabric_IsContainerHost");
Common::GlobalWString Common::ContainerEnvironment::ContainertracePathEnvironmentVariableName = make_global<wstring>(L"FabricLogRoot");
Common::GlobalWString Common::ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable = make_global<wstring>(L"Fabric_NetworkingMode");
Common::GlobalWString Common::ContainerEnvironment::ContainerNetworkingResourceEnvironmentVariable = make_global<wstring>(L"Fabric_NET-{0}-[{1}]");
Common::GlobalWString Common::ContainerEnvironment::ContainerNetworkName = make_global<wstring>(L"servicefabric_network");
Common::GlobalWString Common::ContainerEnvironment::ContainerOverlayNetworkTypeName = make_global<wstring>(L"Overlay");

#if defined(PLATFORM_UNIX)
Common::GlobalWString Common::ContainerEnvironment::ContainerUnderlayNetworkTypeName = make_global<wstring>(L"azure-vnet");
Common::GlobalWString Common::ContainerEnvironment::ContainerNatNetworkTypeName = make_global<wstring>(L"bridge");
#endif


bool Common::ContainerEnvironment::IsContainerHost()
{
    wstring envValue;
    bool envVarFound = Environment::GetEnvironmentVariable(ContainerEnvironment::IsContainerHostEnvironmentVariableName, envValue, Common::NOTHROW());
    bool isContainerHost = false;
    if (envVarFound)
    {
        if (StringUtility::ParseBool::Try(envValue, isContainerHost))
        {
            return isContainerHost;
        }
    }

    return false;
}

std::wstring Common::ContainerEnvironment::GetContainerTracePath()
{
    wstring envValue = L"";
    bool envVarFound = Environment::GetEnvironmentVariable(ContainerEnvironment::ContainertracePathEnvironmentVariableName, envValue, Common::NOTHROW());
    if (envVarFound)
    {
        return Path::Combine(envValue, L"Traces");
    }

    return envValue;
}

std::wstring Common::ContainerEnvironment::GetContainerNetworkingMode()
{
    wstring envValue = L"";
    Environment::GetEnvironmentVariable(ContainerEnvironment::ContainerNetworkingModeEnvironmentVariable, envValue, Common::NOTHROW());
    return envValue;
}
