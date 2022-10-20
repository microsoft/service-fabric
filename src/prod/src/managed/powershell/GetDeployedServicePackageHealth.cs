// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedServicePackageHealth")]
    public sealed class GetDeployedServicePackageHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        [ValidateNotNullOrEmpty]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 2)]
        [ValidateNotNullOrEmpty]
        public string NodeName
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
        public string ServicePackageActivationId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(
                    this.ApplicationName, 
                    this.NodeName, 
                    this.ServiceManifestName,
                    this.ServicePackageActivationId ?? string.Empty);

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

                var deployedServicePacakgeHealth = clusterConnection.GetDeployedServicePackageHealthAsync(
                                                       queryDescription,
                                                       this.GetTimeout(),
                                                       this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(deployedServicePacakgeHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedServicePackageHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedServicePackageHealth)
            {
                var item = output as DeployedServicePackageHealth;

                var healthEventsPropertyPSObj = new PSObject(item.HealthEvents);
                healthEventsPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.HealthEventsPropertyName,
                        healthEventsPropertyPSObj));

                Helpers.AddToPsObject(itemPSObj, item.UnhealthyEvaluations);

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}