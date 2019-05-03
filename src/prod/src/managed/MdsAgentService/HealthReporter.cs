// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Fabric;
    using System.Fabric.Health;
    
    internal class HealthReporter
    {
        private readonly FabricClient fabricClient;
        private readonly Guid partitionId;
        private readonly long instanceId;
        private readonly string serviceTypeName;

        public HealthReporter(Guid partitionId, long instanceId, string serviceTypeName)
        {
            this.partitionId = partitionId;
            this.instanceId = instanceId;
            this.serviceTypeName = serviceTypeName;
            this.fabricClient = new FabricClient();
        }

        public void ReportHealth(HealthState healthState, string property, string description)
        {
            this.fabricClient.HealthManager.ReportHealth(
                 new StatelessServiceInstanceHealthReport(
                     this.partitionId,
                     this.instanceId,
                     new HealthInformation(
                         this.serviceTypeName,
                         property,
                         healthState)
                     {
                         Description = description
                     })); 
        }
    }
}