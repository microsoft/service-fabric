// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    // Types of folders supported
    internal enum FolderProducerType
    {
        // The folder containing crash dumps of Windows Fabric binaries
        WindowsFabricCrashDumps,

        // The folder containing performance counters specified in the cluster manifest
        WindowsFabricPerformanceCounters,

        // A folder that the user specifies via the cluster manifest
        CustomFolder,

        // Invalid folder type
        Invalid
    }
}