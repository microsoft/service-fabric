// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;

    internal class TraceEventSourceFactory : ITraceEventSourceFactory
    {
        public FabricEvents.ExtensionsEvents CreateTraceEventSource(EventTask taskId)
        {
            return new FabricEvents.ExtensionsEvents(taskId);
        }
    }
}