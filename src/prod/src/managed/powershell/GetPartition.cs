// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricPartition", DefaultParameterSetName = "QueryByPartitionId")]
    public sealed class GetPartition : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "QueryByPartitionId")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "QueryByServiceName")]
        public Guid? PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "QueryByServiceName")]
        public Uri ServiceName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                // If the results do not fit a message, the continuation token is set.
                // Keep getting data until all entries are received.
                bool morePages = true;
                string currentContinuationToken = null;
                while (morePages)
                {
                    ServicePartitionList queryResult;
                    if ((this.ServiceName == null || string.IsNullOrEmpty(this.ServiceName.ToString())) && this.PartitionId.HasValue)
                    {
                        queryResult = clusterConnection.GetPartitionAsync(
                                          this.PartitionId.Value,
                                          this.GetTimeout(),
                                          this.GetCancellationToken()).Result;
                    }
                    else
                    {
                        queryResult = clusterConnection.GetPartitionListAsync(
                                          this.ServiceName,
                                          this.PartitionId,
                                          currentContinuationToken,
                                          this.GetTimeout(),
                                          this.GetCancellationToken()).Result;
                    }

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
            var result = new PSObject(output);

            if (output is Partition)
            {
                var partitionResult = output as Partition;
                result.Properties.Add(new PSNoteProperty(Constants.PartitionIdPropertyName, partitionResult.PartitionInformation.Id));
                result.Properties.Add(new PSNoteProperty(Constants.PartitionKindPropertyName, partitionResult.PartitionInformation.Kind));
                if (partitionResult.PartitionInformation.Kind == ServicePartitionKind.Int64Range)
                {
                    var info = partitionResult.PartitionInformation as Int64RangePartitionInformation;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionLowKeyPropertyName, info.LowKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionHighKeyPropertyName, info.HighKey));
                    }
                }
                else if (partitionResult.PartitionInformation.Kind == ServicePartitionKind.Named)
                {
                    var info = partitionResult.PartitionInformation as NamedPartitionInformation;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionNamePropertyName, info.Name));
                    }
                }

                if (output is System.Fabric.Query.StatefulServicePartition)
                {
                    var statefulServicePartition = output as System.Fabric.Query.StatefulServicePartition;
                    result.Properties.Add(new PSNoteProperty(Constants.DataLossNumberPropertyName, statefulServicePartition.PrimaryEpoch.DataLossNumber));
                    result.Properties.Add(new PSNoteProperty(Constants.ConfigurationNumberPropertyName, statefulServicePartition.PrimaryEpoch.ConfigurationNumber));
                }

                return result;
            }

            return base.FormatOutput(output);
        }
    }
}