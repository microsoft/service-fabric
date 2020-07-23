// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricReplica")]
    public sealed class GetReplica : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public long ReplicaOrInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ServiceReplicaStatusFilter? ReplicaStatusFilter
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var replicaStatusFilter = ServiceReplicaStatusFilter.Default;
                if (this.ReplicaStatusFilter.HasValue)
                {
                    replicaStatusFilter = this.ReplicaStatusFilter.Value;
                }

                // If the results do not fit a message, the continuation token is set.
                // Keep getting data until all entries are received.
                bool morePages = true;
                string currentContinuationToken = null;
                while (morePages)
                {
                    var queryResult = clusterConnection.GetReplicaListAsync(
                                          this.PartitionId,
                                          this.ReplicaOrInstanceId,
                                          replicaStatusFilter,
                                          currentContinuationToken,
                                          this.GetTimeout(),
                                          this.GetCancellationToken()).Result;

                    foreach (var item in queryResult)
                    {
                        this.WriteObject(this.FormatOutput(item));
                    }

                    // Continuation token is not added as a PsObject because it breaks the pipeline scenarios
                    // like Get-ServiceFabricNode | Get-ServiceFabricNodeHealth
                    morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is StatefulServiceReplica)
            {
                var item = output as StatefulServiceReplica;
                var result = new PSObject(item);

                result.Properties.Add(
                    new PSAliasProperty(Constants.ReplicaIdPropertyName, Constants.IdPropertyName));
                result.Properties.Add(
                    new PSAliasProperty(Constants.ReplicaOrInstanceIdPropertyName, Constants.IdPropertyName));
                result.Properties.Add(
                    new PSNoteProperty(Constants.PartitionIdPropertyName, this.PartitionId.ToString()));

                return result;
            }

            if (output is StatelessServiceInstance)
            {
                var item = output as StatelessServiceInstance;
                var result = new PSObject(item);

                result.Properties.Add(
                    new PSAliasProperty(Constants.InstanceIdPropertyName, Constants.IdPropertyName));
                result.Properties.Add(
                    new PSAliasProperty(Constants.ReplicaOrInstanceIdPropertyName, Constants.IdPropertyName));
                result.Properties.Add(
                    new PSNoteProperty(Constants.PartitionIdPropertyName, this.PartitionId.ToString()));

                return result;
            }

            return base.FormatOutput(output);
        }
    }
}