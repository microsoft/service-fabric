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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricClusterHealth", DefaultParameterSetName = "IncludeStats")]
    public sealed class GetClusterHealth : CommonCmdletBase
    {
        [Parameter(Mandatory = false)]
        public bool? ConsiderWarningAsError
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyApplications
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, 100)]
        public byte? MaxPercentUnhealthyNodes
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
        public long? ApplicationsHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? ApplicationsFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? NodesHealthStateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public HealthStateFilter? NodesFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public ApplicationTypeHealthPolicyMap ApplicationTypeHealthPolicyMap
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "ExcludeStats")]
        public SwitchParameter ExcludeHealthStatistics
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "IncludeStats")]
        public SwitchParameter IncludeSystemApplicationHealthStatistics
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryDescription = new ClusterHealthQueryDescription();

                if (this.ConsiderWarningAsError.HasValue ||
                    this.MaxPercentUnhealthyNodes.HasValue ||
                    this.MaxPercentUnhealthyApplications.HasValue ||
                    this.ApplicationTypeHealthPolicyMap != null)
                {
                    queryDescription.HealthPolicy = new ClusterHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
                        MaxPercentUnhealthyApplications = this.MaxPercentUnhealthyApplications.GetValueOrDefault(),
                        MaxPercentUnhealthyNodes = this.MaxPercentUnhealthyNodes.GetValueOrDefault()
                    };

                    if (this.ApplicationTypeHealthPolicyMap != null)
                    {
                        foreach (var entry in this.ApplicationTypeHealthPolicyMap)
                        {
                            queryDescription.HealthPolicy.ApplicationTypeHealthPolicyMap.Add(entry.Key, entry.Value);
                        }
                    }
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

                if (this.NodesHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_NodesHealthStateFilter_Deprecated);
                    if (!this.NodesFilter.HasValue)
                    {
                        this.NodesFilter = (HealthStateFilter)this.NodesHealthStateFilter;
                    }
                }

                if (this.NodesFilter.HasValue)
                {
                    queryDescription.NodesFilter = new NodeHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.NodesFilter.Value,
                    };
                }

                if (this.ApplicationsHealthStateFilter.HasValue)
                {
                    this.WriteWarning(StringResources.PowerShell_ApplicationsHealthStateFilter_Deprecated);
                    if (!this.ApplicationsFilter.HasValue)
                    {
                        this.ApplicationsFilter = (HealthStateFilter)this.ApplicationsHealthStateFilter;
                    }
                }

                if (this.ApplicationsFilter.HasValue)
                {
                    queryDescription.ApplicationsFilter = new ApplicationHealthStatesFilter()
                    {
                        HealthStateFilterValue = this.ApplicationsFilter.Value,
                    };
                }

                if (this.ApplicationHealthPolicyMap != null)
                {
                    foreach (var entry in this.ApplicationHealthPolicyMap)
                    {
                        queryDescription.ApplicationHealthPolicyMap.Add(entry.Key, entry.Value);
                    }
                }

                if (this.ExcludeHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new ClusterHealthStatisticsFilter()
                    {
                        ExcludeHealthStatistics = this.ExcludeHealthStatistics
                    };
                }

                if (this.IncludeSystemApplicationHealthStatistics)
                {
                    queryDescription.HealthStatisticsFilter = new ClusterHealthStatisticsFilter()
                    {
                        IncludeSystemApplicationHealthStatistics = this.IncludeSystemApplicationHealthStatistics
                    };
                }

                var clusterHealth = clusterConnection.GetClusterHealthAsync(
                                        queryDescription,
                                        this.GetTimeout(),
                                        this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(clusterHealth));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterHealthErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ClusterHealth)
            {
                var item = output as ClusterHealth;

                var itemPSObj = new PSObject(item);

                if (item.NodeHealthStates != null)
                {
                    var nodeHealthStatesPropertyPSObj = new PSObject(item.NodeHealthStates);
                    nodeHealthStatesPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.NodeHealthStatesPropertyName,
                            nodeHealthStatesPropertyPSObj));
                }

                if (item.ApplicationHealthStates != null)
                {
                    var applicationHealthStatesPropertyPSObj = new PSObject(item.ApplicationHealthStates);
                    applicationHealthStatesPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.ApplicationHealthStatesPropertyName,
                            applicationHealthStatesPropertyPSObj));
                }

                if (item.HealthEvents != null)
                {
                    var healthEventsPropertyPSObj = new PSObject(item.HealthEvents);
                    healthEventsPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.HealthEventsPropertyName,
                            healthEventsPropertyPSObj));
                }

                Helpers.AddToPsObject(itemPSObj, item.UnhealthyEvaluations);

                Helpers.AddToPSObject(itemPSObj, item.HealthStatistics);

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}