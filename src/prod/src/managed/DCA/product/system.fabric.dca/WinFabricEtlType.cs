// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    // Types of Windows Fabric ETLs supported
    internal enum WinFabricEtlType
    {
        // ETL type not specified
        Unspecified,

        // The default ETL files
        DefaultEtl,

        // ETL files containing important events to be uploaded to
        // queryable store
        QueryEtl,

        // ETL files containing operational events to be pushed to
        // events store
        OperationalEtl
    }
}