// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric;
    using System.Linq;
    using System.Globalization;
    using System.Text;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class UpgradeSystemServiceFactory : IStatefulServiceFactory
    {
        private static readonly TraceType TraceType = new TraceType("UpgradeSystemServiceFactory");

        internal void RegisterUpgradeSystemService(FabricRuntime runtime)
        {
            runtime.RegisterStatefulServiceFactory(Constants.UpgradeServiceType, this);

            Trace.WriteInfo(
                TraceType,
                "Registered service type : {0}",
                Constants.UpgradeServiceType);
        }

        IStatefulServiceReplica IStatefulServiceFactory.CreateReplica(
            string serviceTypeName, 
            Uri serviceName, 
            byte[] initializationData, 
            Guid partitionId, 
            long replicaId)
        {
            var initParams = new StatefulServiceInitializationParameters(FabricRuntime.GetActivationContext()) 
            { 
                ServiceTypeName = serviceTypeName,
                ServiceName = serviceName,
                InitializationData = initializationData,
                PartitionId = partitionId,
                ReplicaId = replicaId,
            };

            return UpgradeSystemService.CreateAndInitialize(initParams);
        }
    }
}