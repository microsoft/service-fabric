//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NamingUri ContainerGroupDeploymentUtility::GetApplicationName(wstring const &deploymentName)
{
    return NamingUri(deploymentName);
}

NamingUri ContainerGroupDeploymentUtility::GetServiceName(wstring const &deploymentName, wstring const &instanceName)
{
    return NamingUri(wformatString("{0}{1}{2}", deploymentName, NamingUri::SegmentDelimiter, instanceName));
}

wstring ContainerGroupDeploymentUtility::GetServiceManifestName(wstring const &deploymentName)
{
    return wformatString("{0}Pkg", deploymentName);
}

wstring ContainerGroupDeploymentUtility::GetCodePackageName(wstring const &containerName)
{
    return wformatString("{0}Pkg", containerName);
}
