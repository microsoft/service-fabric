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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplicationHealth")]
    public sealed class GetApplicationHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public Uri ApplicationName
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
        public byte? MaxPercentUnhealthyDeployedApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyServices
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
        public long? ServicesHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? ServicesFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? DeployedApplicationsHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? DeployedApplicationsFilter
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
                var queryDescription = new ApplicationHealthQueryDescription(this.ApplicationName);
                if (this.ConsiderWarningAsError.HasValue ||
                    this.MaxPercentUnhealthyDeployedApplications.HasValue ||
                    this.MaxPercentUnhealthyServices.HasValue ||
                    this.MaxPercentUnhealthyPartitionsPerService.HasValue ||
                    this.MaxPercentUnhealthyReplicasPerPartition.HasValue)
                {
                    queryDescription.HealthPolicy = new ApplicationHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
                        MaxPercentUnhealthyDeployedApplications = this.MaxPercentUnhealthyDeployedApplications.GetValueOrDefault(),
                        DefaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy()
                        {
                            MaxPercentUnhealthyServices = this.MaxPercentUnhealthyServices.GetValueOrDefault(),
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

                if (this.ServicesHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_ServicesHealthStateFilter_Deprecated);
                    if (!this.ServicesFilter.HasValue)
                    {
                        this.ServicesFilter = (HealthStateFilter)this.ServicesHealthStateFilter;
                    }
                }

                if (this.ServicesFilter.HasValue)
                {
                    queryDescription.ServicesFilter = new ServiceHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.ServicesFilter.Value,
                    };
                }

                if (this.DeployedApplicationsHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_DeployedApplicationsHealthStateFilter_Deprecated);
                    if (!this.DeployedApplicationsFilter.HasValue)
                    {
                        this.DeployedApplicationsFilter = (HealthStateFilter)this.DeployedApplicationsHealthStateFilter;
                    }
                }

                if (this.DeployedApplicationsFilter.HasValue)
                {
                    queryDescription.DeployedApplicationsFilter = new DeployedApplicationHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.DeployedApplicationsFilter.Value,
                    };
                }

                if (this.ExcludeHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new ApplicationHealthStatisticsFilter()
                    {
                        ExcludeHealthStatistics = this.ExcludeHealthStatistics
                    };
                }

                var applicationHealth = clusterConnection.GetApplicationHealthAsync(
                                            queryDescription,
                                            this.GetTimeout(),
                                            this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(applicationHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetApplicationHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ApplicationHealth)
            {
                var item = output as ApplicationHealth;

                var aggregatedServiceHealthStatesPropertyPSObj = new PSObject(item.ServiceHealthStates);
                aggregatedServiceHealthStatesPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var aggregatedDeployedApplicationHealthStatesPropertyPSObj = new PSObject(item.DeployedApplicationHealthStates);
                aggregatedDeployedApplicationHealthStatesPropertyPSObj.Members.Add(
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
                        Constants.ServiceHealthStatesPropertyName,
                        aggregatedServiceHealthStatesPropertyPSObj));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.DeployedApplicationHealthStatesPropertyName,
                        aggregatedDeployedApplicationHealthStatesPropertyPSObj));

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