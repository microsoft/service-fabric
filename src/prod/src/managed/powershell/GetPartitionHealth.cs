// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricPartitionHealth")]
    public sealed class GetPartitionHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public bool? ConsiderWarningAsError
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyReplicasPerPartition
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? EventsHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? EventsFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? ReplicasHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? ReplicasFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter ExcludeHealthStatistics
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryDescription = new PartitionHealthQueryDescription(this.PartitionId);
                if (this.ConsiderWarningAsError.HasValue ||
                    this.MaxPercentUnhealthyReplicasPerPartition.HasValue)
                {
                    queryDescription.HealthPolicy = new ApplicationHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
                        DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy()
                        {
                            MaxPercentUnhealthyReplicasPerPartition = this.MaxPercentUnhealthyReplicasPerPartition.GetValueOrDefault()
                        }
                    };
                }

                if (this.EventsHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_EventsHealthStateFilter_Deprecated);
                    if (!this.EventsFilter.HasValue)
                    {
                        this.EventsFilter = (HealthStateFilter)this.EventsHealthStateFilter;
                    }
                }

                if (this.EventsFilter.HasValue)
                {
                    queryDescription.EventsFilter = new HealthEventsFilter()
                    {
                        HealthStateFilterValue = this.EventsFilter.Value,
                    };
                }

                if (this.ReplicasHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_ReplicasHealthStateFilter_Deprecated);
                    if (!this.ReplicasFilter.HasValue)
                    {
                        this.ReplicasFilter = (HealthStateFilter)this.ReplicasHealthStateFilter;
                    }
                }

                if (this.ReplicasFilter.HasValue)
                {
                    queryDescription.ReplicasFilter = new ReplicaHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.ReplicasFilter.Value,
                    };
                }

                if (this.ExcludeHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new PartitionHealthStatisticsFilter()
                    {
                        ExcludeHealthStatistics = this.ExcludeHealthStatistics
                    };
                }

                var partitionHealth = clusterConnection.GetPartitionHealthAsync(
                                          queryDescription,
                                          this.GetTimeout(),
                                          this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(partitionHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetPartitionHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is PartitionHealth)
            {
                var item = output as PartitionHealth;

                var aggregatedReplicaHealthStatesPropertyPSObj = new PSObject(item.ReplicaHealthStates);
                aggregatedReplicaHealthStatesPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var healthEventsPropertyPSObj = new PSObject(item.HealthEvents);
                healthEventsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ReplicaHealthStatesPropertyName,
                        aggregatedReplicaHealthStatesPropertyPSObj));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.HealthEventsPropertyName,
                        healthEventsPropertyPSObj));

                Helpers.AddToPsObject(itemPSObj, item.UnhealthyEvaluations);

                Helpers.AddToPSObject(itemPSObj, item.HealthStatistics);

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}