// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM
{
    public enum NodeDeactivationIntent
    {
        None = 0x0,
        Pause = 0x1,
        Restart = 0x2,
        RemoveData = 0x3,
        RemoveNode = 0x4,
    }
}