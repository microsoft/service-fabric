// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting
{


    public enum EntryPointType
    {
        None = 0x0,
        Exe = 0x1,
        DllHost = 0x2,
        ContainerHost = 0x3,
    }

}
