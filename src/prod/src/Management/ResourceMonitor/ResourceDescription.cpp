// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ResourceMonitor;

ResourceDescription::ResourceDescription(Hosting2::CodePackageInstanceIdentifier const & codePackageIdentifier, std::wstring const & appName)
    :resourceUsage_(),
    codePackageIdentifier_(codePackageIdentifier),
    appName_(appName)
{
}

