// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal enum FabricPerformanceCounterCategoryType
    {
        // Summary:
        //     The instance functionality for the performance counter category is unknown.
        Unknown = -1,
        //
        // Summary:
        //     The performance counter category can have only a single instance.
        SingleInstance = 0,
        //
        // Summary:
        //     The performance counter category can have multiple instances.
        MultiInstance = 2,
    }
}