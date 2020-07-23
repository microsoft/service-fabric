// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    public enum ReconfigurationResult : int
    {
        Completed = 0x1,
        AbortSwapPrimary = 0x2,
        ChangeConfiguration = 0x3,
        Aborted = 0x4,
        DemoteCompleted = 0x5,
    }
}