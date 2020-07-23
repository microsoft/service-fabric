// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System.Threading;
    using Microsoft.ServiceFabric.Services.Runtime;

    internal sealed class ServiceHost
    {
        private static void Main(string[] args)
        {
            ServiceRuntime.RegisterServiceAsync(
                "FabricMonitoringServiceType",
                context => new Service(context)).GetAwaiter().GetResult();

            Thread.Sleep(Timeout.Infinite);
        }
    }
}