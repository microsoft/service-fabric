// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ContainerEnvironment
    {
    public:
        
        static bool IsContainerHost();
        static std::wstring GetContainerTracePath();
        static std::wstring GetContainerNetworkingMode();

        static GlobalWString IsContainerHostEnvironmentVariableName;
        static GlobalWString ContainerNetworkingModeEnvironmentVariable;
        static GlobalWString ContainerNetworkingResourceEnvironmentVariable;
        static GlobalWString ContainerNetworkName;
        static GlobalWString ContainerOverlayNetworkTypeName;

#if defined(PLATFORM_UNIX)
        static GlobalWString ContainerUnderlayNetworkTypeName;
        static GlobalWString ContainerNatNetworkTypeName;
#endif

    private:
        static GlobalWString ContainertracePathEnvironmentVariableName;        
    };
}
