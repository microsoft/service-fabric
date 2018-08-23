//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ContainerGroupDeploymentUtility
    {
    public:
        static Common::NamingUri GetApplicationName(std::wstring const &deploymentName);
        static Common::NamingUri GetServiceName(std::wstring const &deploymentName, std::wstring const &instanceName);
        static std::wstring GetServiceManifestName(std::wstring const &deploymentName);
        static std::wstring GetCodePackageName(std::wstring const &containerName);
    };
}
