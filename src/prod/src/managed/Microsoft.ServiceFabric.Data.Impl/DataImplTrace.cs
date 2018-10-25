// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System.Fabric.Common.Tracing;

    internal static class DataImplTrace
    {
        internal static FabricEvents.ExtensionsEvents Source;

        static DataImplTrace()
        {
            TraceConfig.InitializeFromConfigStore();
            Source = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ServiceFramework);
        }
    }
}