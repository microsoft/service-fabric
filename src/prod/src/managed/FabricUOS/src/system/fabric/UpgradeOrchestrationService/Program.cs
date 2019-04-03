// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Net;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Services.Runtime;

    public class Program
    {
        private const string TraceType = "UpgradeOrchestrationService";
        private const string UpgradeOrchestrationServiceType = "UpgradeOrchestrationServiceType";

        internal static UpgradeOrchestrationServiceAgent ServiceAgent { get; set; }

        public static int Main(string[] args)
        {
            int status = 0;

            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;

            try
            {
                ServiceRuntime.RegisterServiceAsync(
                    UpgradeOrchestrationServiceType,
                    ReadConfiguration).GetAwaiter().GetResult();

                Program.ServiceAgent = UpgradeOrchestrationServiceAgent.Instance;

                Thread.Sleep(Timeout.Infinite);
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteError(TraceType, "Host failed: " + e.ToString());
                throw;
            }

            return status;
        }

        private static StatefulServiceBase ReadConfiguration(StatefulServiceContext context)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "ReadConfiguration");
            System.Fabric.Common.NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            string fabricUpgradeOrchestrationServiceReplicatorAddress = configStore.ReadUnencryptedString("FabricNode", "UpgradeOrchestrationServiceReplicatorAddress");
            bool flag = EnableEndpointV2Utility.GetValue(configStore);

            return new FabricUpgradeOrchestrationService(context, fabricUpgradeOrchestrationServiceReplicatorAddress, flag);
        }
    }
}