// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class PaasConfig : ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(PaasConfig, "Paas")

        /* -------------- Configuration Security -------------- */

        // X509 certificate store used by fabric for configuration protection
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Paas", ClusterId, L"", ConfigEntryUpgradePolicy::NotAllowed);
    };
}
