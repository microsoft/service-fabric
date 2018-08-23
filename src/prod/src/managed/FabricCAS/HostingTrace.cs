// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System.Fabric.Common.Tracing;

    internal static class HostingTrace
    {
        internal static FabricEvents.ExtensionsEvents Source { get; private set; }

        internal static void Initialize()
        {
            Source = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ManagedHosting);
        }
    }
}