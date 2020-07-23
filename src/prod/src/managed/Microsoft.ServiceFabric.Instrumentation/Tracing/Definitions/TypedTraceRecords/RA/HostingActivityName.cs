// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    public enum HostingActivityName : uint
    {
        ServiceTypeRegistered = 0,

        AppHostDown = 1,

        RuntimeDown = 2
    }
}