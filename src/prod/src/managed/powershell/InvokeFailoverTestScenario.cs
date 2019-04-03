// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricFailoverTestScenario")]
    [Obsolete("This cmdlet is deprecated.  Please use Chaos instead https://docs.microsoft.com/azure/service-fabric/service-fabric-controlled-chaos")]
    public sealed class InvokeFailoverTestScenario : PartitionSelectorOperationBase
    {
        [Parameter(Mandatory = true)]
        public uint MaxServiceStabilizationTimeoutSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public uint TimeToRunMinute
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public int? WaitTimeBetweenFaultsSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var partitionSelector = this.GetPartitionSelector();
                clusterConnection.InvokeFailoverTestScenarioAsync(
                    this.ServiceName,
                    partitionSelector,
                    this.TimeToRunMinute,
                    this.MaxServiceStabilizationTimeoutSec,
                    this.WaitTimeBetweenFaultsSec,
                    this.TimeoutSec,
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_FailoverTestScenarioSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.InvokeFailoverTestCommandErrorId,
                    clusterConnection);
            }
        }
    }
}