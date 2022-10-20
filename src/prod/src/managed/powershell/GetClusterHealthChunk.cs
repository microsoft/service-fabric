// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricClusterHealthChunk")]
    public sealed class GetClusterHealthChunk : CommonCmdletBase
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
        public ApplicationHealthPolicyMap ApplicationHealthPolicies
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

        [Parameter(Mandatory = false)]
        public List<ApplicationHealthStateFilter> ApplicationFilters
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public List<NodeHealthStateFilter> NodeFilters
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryDescription = new ClusterHealthChunkQueryDescription();

                if (this.ConsiderWarningAsError.HasValue ||
                    this.MaxPercentUnhealthyNodes.HasValue ||
                    this.MaxPercentUnhealthyApplications.HasValue ||
                    this.ApplicationTypeHealthPolicyMap != null)
                {
                    queryDescription.ClusterHealthPolicy = new ClusterHealthPolicy()
                    {
                        ConsiderWarningAsError = this.ConsiderWarningAsError.GetValueOrDefault(),
                        MaxPercentUnhealthyApplications = this.MaxPercentUnhealthyApplications.GetValueOrDefault(),
                        MaxPercentUnhealthyNodes = this.MaxPercentUnhealthyNodes.GetValueOrDefault()
                    };

                    if (this.ApplicationTypeHealthPolicyMap != null)
                    {
                        foreach (var entry in this.ApplicationTypeHealthPolicyMap)
                        {
                            queryDescription.ClusterHealthPolicy.ApplicationTypeHealthPolicyMap.Add(entry.Key, entry.Value);
                        }
                    }
                }

                if (this.ApplicationFilters != null)
                {
                    foreach (var filter in this.ApplicationFilters)
                    {
                        queryDescription.ApplicationFilters.Add(filter);
                    }
                }

                if (this.NodeFilters != null)
                {
                    foreach (var filter in this.NodeFilters)
                    {
                        queryDescription.NodeFilters.Add(filter);
                    }
                }

                if (this.ApplicationHealthPolicies != null)
                {
                    foreach (var entry in this.ApplicationHealthPolicies)
                    {
                        queryDescription.ApplicationHealthPolicies.Add(entry.Key, entry.Value);
                    }
                }

                var clusterHealthChunk = clusterConnection.GetClusterHealthChunkAsync(
                                             queryDescription,
                                             this.GetTimeout(),
                                             this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(clusterHealthChunk));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterHealthChunkErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ClusterHealthChunk)
            {
                var item = output as ClusterHealthChunk;

                var itemPSObj = new PSObject(item);

                if (item.NodeHealthStateChunks != null)
                {
                    var nodeHealthStateChunksPropertyPSObj = new PSObject(item.NodeHealthStateChunks);
                    nodeHealthStateChunksPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.NodeHealthStateChunksPropertyName,
                            nodeHealthStateChunksPropertyPSObj));
                }

                if (item.ApplicationHealthStateChunks != null)
                {
                    var applicationHealthStateChunksPropertyPSObj = new PSObject(item.ApplicationHealthStateChunks);
                    applicationHealthStateChunksPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.ApplicationHealthStateChunksPropertyName,
                            applicationHealthStateChunksPropertyPSObj));
                }

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }
    }
}