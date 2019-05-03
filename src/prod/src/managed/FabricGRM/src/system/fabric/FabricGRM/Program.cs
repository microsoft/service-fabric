// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
namespace System.Fabric.GatewayResourceManager.Service
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Threading;
    using Microsoft.ServiceFabric.Services.Runtime;

    public class Program
    {
        public const string TraceType = "GatewayResourceManager";
        private const string GatewayResourceManagerType = "GatewayResourceManagerType";

        internal static FabricGatewayResourceManagerAgent ServiceAgent
        {
            get;
            private set;
        }

        public static int Main(string[] args)
        {
            GatewayResourceManagerTrace.TraceSource.WriteInfo(TraceType, "Inside Main: ");
            int status = 0;

            try
            {
                ServiceRuntime.RegisterServiceAsync(
                    GatewayResourceManagerType,
                    ReadConfiguration).GetAwaiter().GetResult();

                Program.ServiceAgent = FabricGatewayResourceManagerAgent.Instance;

                Thread.Sleep(Timeout.Infinite);
            }
            catch (Exception e)
            {
                GatewayResourceManagerTrace.TraceSource.WriteInfo(TraceType, "Host failed: " + e.ToString());
                throw;
            }

            return status;
        }

        private static StatefulServiceBase ReadConfiguration(StatefulServiceContext context)
        {
            GatewayResourceManagerTrace.TraceSource.WriteInfo(TraceType, "ReadConfiguration");
            System.Fabric.Common.NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            string fabricGatewayResourceManagerReplicatorAddress = configStore.ReadUnencryptedString("FabricNode", "GatewayResourceManagerReplicatorAddress");
            return new FabricGatewayResourceManager(context, fabricGatewayResourceManagerReplicatorAddress);
        }
    }
}
