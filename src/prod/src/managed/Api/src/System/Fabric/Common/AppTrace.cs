// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Common.Tracing;

    internal static class AppTrace
    {
        private static FabricEvents.ExtensionsEvents traceSource;

        static AppTrace()
        {
            // Try reading config
            TraceConfig.InitializeFromConfigStore();

            // Create the trace source
            AppTrace.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.SystemFabric);
        }
        
        public static FabricEvents.ExtensionsEvents TraceSource
        {
            get
            {
                return AppTrace.traceSource;
            }
        }
    }
}