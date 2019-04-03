// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Linq;
    using System.Net;
    using System.Threading;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class Program
    {
        public static void Main(string[] args)
        {
#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif

            using (FabricRuntime fabricRuntime = FabricRuntime.Create())
            {
                ServiceRuntime.RegisterServiceAsync("EventStoreServiceType", context =>
                    new EventStoreService(
                        context,
                        NativeConfigStore.FabricGetConfigStore().ReadUnencryptedString(
                            "FabricNode",
                            "EventStoreServiceReplicatorAddress")))
                            .GetAwaiter()
                            .GetResult();

                Thread.Sleep(Timeout.Infinite);
            }
        }
    }
}