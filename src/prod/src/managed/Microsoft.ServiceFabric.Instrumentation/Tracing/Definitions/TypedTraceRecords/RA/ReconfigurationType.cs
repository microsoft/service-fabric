// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    public enum ReconfigurationType
    {
        Other = 0x0,
        SwapPrimary = 0x1,
        Failover = 0x2,
        None = 0x3,
    }
}