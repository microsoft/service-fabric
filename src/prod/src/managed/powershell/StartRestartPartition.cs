// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricPartitionRestart")]
    public sealed class StartRestartPartition : PartitionSelectorOperationBase
    {
        [Parameter(Mandatory = true)]
        public Guid OperationId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public RestartPartitionMode RestartPartitionMode
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

                clusterConnection.RestartPartitionAsync(
                    this.OperationId,
                    partitionSelector,
                    this.RestartPartitionMode,
                    this.TimeoutSec,
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.StartRestartPartitionCommandErrorId,
                    clusterConnection);
            }
        }
    }
}