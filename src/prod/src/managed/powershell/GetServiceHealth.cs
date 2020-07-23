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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricServiceHealth")]
    public sealed class GetServiceHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public Uri ServiceName
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
        public byte? MaxPercentUnhealthyPartitionsPerService
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
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
        public long? PartitionsHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? PartitionsFilter
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
                var queryDescription = new ServiceHealthQueryDescription(this.ServiceName);
                if (this.ConsiderWarningAsError.HasValue ||
                    this.MaxPercentUnhealthyPartitionsPerService.HasValue ||
                    this.MaxPercentUnhealthyReplicasPerPartition.HasValue)
                {
                    queryDescription.HealthPolicy = new ApplicationHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
                        DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy()
                        {
                            MaxPercentUnhealthyPartitionsPerService = this.MaxPercentUnhealthyPartitionsPerService.GetValueOrDefault(),
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

                if (this.PartitionsHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_PartitionsHealthStateFilter_Deprecated);
                    if (!this.PartitionsFilter.HasValue)
                    {
                        this.PartitionsFilter = (HealthStateFilter)this.PartitionsHealthStateFilter;
                    }
                }

                if (this.PartitionsFilter.HasValue)
                {
                    queryDescription.PartitionsFilter = new PartitionHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.PartitionsFilter.Value,
                    };
                }

                if (this.ExcludeHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new ServiceHealthStatisticsFilter()
                    {
                        ExcludeHealthStatistics = this.ExcludeHealthStatistics
                    };
                }

                var serviceHealth = clusterConnection.GetServiceHealthAsync(
                                        queryDescription,
                                        this.GetTimeout(),
                                        this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(serviceHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetServiceHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ServiceHealth)
            {
                var item = output as ServiceHealth;

                var aggregatedPartitionHealthStatesPropertyPSObj = new PSObject(item.PartitionHealthStates);
                aggregatedPartitionHealthStatesPropertyPSObj.Members.Add(
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
                        Constants.PartitionHealthStatesPropertyName,
                        aggregatedPartitionHealthStatesPropertyPSObj));

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