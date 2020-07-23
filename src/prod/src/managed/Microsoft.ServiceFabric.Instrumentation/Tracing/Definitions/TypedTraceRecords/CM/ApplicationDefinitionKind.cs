// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM
{
    public enum ApplicationDefinitionKind
    {
        ServiceFabricApplicationDescription = 0x0,
        Compose = 0x1,
        MeshApplicationDescription = 0x2,
        ApplicationDefinitionKind65535 = 0xffff,
    }
}
