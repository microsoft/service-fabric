// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class DiagnosticsConfig : ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(DiagnosticsConfig, "Diagnostics")

        // Maximum disk buffer allocated for writing lttng trace files
        INTERNAL_CONFIG_ENTRY(int, L"Diagnostics", LinuxTraceDiskBufferSizeInMB, 5120, ConfigEntryUpgradePolicy::Static);
        // Maximum disk buffer allocated for writing lttng trace files for trace sessions running inside container
        INTERNAL_CONFIG_ENTRY(int, L"Diagnostics", LinuxTraceDiskBufferSizeInsideContainerInMB, 512, ConfigEntryUpgradePolicy::Static);
    };
}
