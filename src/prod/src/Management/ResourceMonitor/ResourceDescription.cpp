// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ResourceMonitor;

ResourceDescription::ResourceDescription(
    Hosting2::CodePackageInstanceIdentifier const & codePackageIdentifier,
    std::wstring const & appName,
    bool isLinuxContainerIsolated)
    :resourceUsage_(),
    codePackageIdentifier_(codePackageIdentifier),
    appName_(appName),
    isLinuxContainerIsolated_(isLinuxContainerIsolated),
    appHosts_()
{
}

void ResourceDescription::UpdateAppHosts(std::wstring const& appHost, bool isAdd)
{
    if (isAdd)
    {
        appHosts_.insert(appHost);
    }
    else 
    {
        appHosts_.erase(appHost);
    }
}
