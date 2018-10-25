// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Common.Tracing;

    internal static class TestabilityTrace
    {
        private static FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.Testability);

        public static FabricEvents.ExtensionsEvents TraceSource
        {
            get
            {
                return TestabilityTrace.traceSource;
            }
        }
    }
}