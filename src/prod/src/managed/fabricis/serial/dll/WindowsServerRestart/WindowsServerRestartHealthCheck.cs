// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class WindowsServerRestartHealthCheck : IHealthCheck
    {
        private readonly FabricClient fabricClient;
        public WindowsServerRestartHealthCheck(FabricClient fabricClient)
        {
            fabricClient.ThrowIfNull("fabricClient");
            this.fabricClient = fabricClient;
        }

        public async Task<bool> PerformCheckAsync(CancellationToken token)
        {
            var clusterHealth = await this.fabricClient.HealthManager.GetClusterHealthAsync(TimeSpan.FromMinutes(10), token);
            return clusterHealth.AggregatedHealthState == HealthState.Ok || clusterHealth.AggregatedHealthState == HealthState.Warning;
        }
    }

    internal class WindowsServerRestartHealthCheckFactory : IHealthCheckFactory
    {
       private readonly FabricClient fabricClient;

       public WindowsServerRestartHealthCheckFactory(FabricClient fabricClient)
        {
           fabricClient.ThrowIfNull("fabricClient");
           this.fabricClient = fabricClient;
        }

        public IHealthCheck Create()
        {
            return new WindowsServerRestartHealthCheck(this.fabricClient);
        }
    }
}