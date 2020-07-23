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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedApplicationHealth")]
    public sealed class GetDeployedApplicationHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
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
        public long? DeployedServicePackagesHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? DeployedServicePackagesFilter
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
                var queryDescription = new DeployedApplicationHealthQueryDescription(this.ApplicationName, this.NodeName);
                if (this.ConsiderWarningAsError.HasValue)
                {
                    queryDescription.HealthPolicy = new ApplicationHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
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

                if (this.DeployedServicePackagesHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_DeployedServicePackagesHealthStateFilter_Deprecated);
                    if (!this.DeployedServicePackagesFilter.HasValue)
                    {
                        this.DeployedServicePackagesFilter = (HealthStateFilter)this.DeployedServicePackagesHealthStateFilter;
                    }
                }

                if (this.DeployedServicePackagesFilter.HasValue)
                {
                    queryDescription.DeployedServicePackagesFilter = new DeployedServicePackageHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.DeployedServicePackagesFilter.Value,
                    };
                }

                if (this.ExcludeHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new DeployedApplicationHealthStatisticsFilter()
                    {
                        ExcludeHealthStatistics = this.ExcludeHealthStatistics
                    };
                }

                var deployedApplicationHealth = clusterConnection.GetDeployedApplicationHealthAsync(
                                                    queryDescription,
                                                    this.GetTimeout(),
                                                    this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(deployedApplicationHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedApplicationHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedApplicationHealth)
            {
                var item = output as DeployedApplicationHealth;

                var aggregatedDeployedServicePackageHealthStatesPropertyPSObj = new PSObject(item.DeployedServicePackageHealthStates);
                aggregatedDeployedServicePackageHealthStatesPropertyPSObj.Members.Add(
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
                        Constants.DeployedServicePackageHealthStatesPropertyName,
                        aggregatedDeployedServicePackageHealthStatesPropertyPSObj));

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