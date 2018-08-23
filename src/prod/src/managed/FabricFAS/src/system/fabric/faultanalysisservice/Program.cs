// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using Microsoft.ServiceFabric.Services.Runtime;
    using Threading;

    public class Program
    {       
        private const string TraceType = "FaultAnalysisService";
        private const string FaultAnalysisServiceType = "FaultAnalysisServiceType";

        internal static FabricFaultAnalysisServiceAgent ServiceAgent
        {
            get;
            private set;
        }

        public static int Main(string[] args)
        {
            int status = 0;

            try
            {
                ServiceRuntime.RegisterServiceAsync(
                    FaultAnalysisServiceType,
                    ReadConfiguration).GetAwaiter().GetResult();

                Program.ServiceAgent = FabricFaultAnalysisServiceAgent.Instance;

                Thread.Sleep(Timeout.Infinite);
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "Host failed: " + e.ToString());
                throw;
            }

            return status;
        }

        private static StatefulServiceBase ReadConfiguration(StatefulServiceContext context)
        {
            System.Fabric.Common.NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            string faultAnalysisServiceReplicatorAddress = configStore.ReadUnencryptedString("FabricNode", "FaultAnalysisServiceReplicatorAddress");
            bool flag = EnableEndpointV2Utility.GetValue(configStore);
            
            return new FaultAnalysisService(context, faultAnalysisServiceReplicatorAddress, flag);
        }
    }
}