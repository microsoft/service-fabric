// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNode", DefaultParameterSetName = "AllPages")]
    public sealed class GetNode : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "SinglePage")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        public NodeStatusFilter? StatusFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "SinglePage")]
        public SwitchParameter GetSinglePage
        {
            get;
            set;
        }

        // Continuation token is respected as long as it's given, regardless of parameter set.
        [Parameter(Mandatory = false, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        public string ContinuationToken
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        [ValidateRange(0, long.MaxValue)]
        public long? MaxResults
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (this.GetSinglePage)
                {
                    this.GetResultsWithPagedQuery(clusterConnection);
                }
                else
                {
                    this.GetAllQueryResultPages(clusterConnection);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetNodeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is Node)
            {
                var item = output as Node;

                var itemPSObj = new PSObject(item);

                if (item.NodeDeactivationInfo != null)
                {
                    var deactivationInfoPSObj = new PSObject(item.NodeDeactivationInfo);
                    deactivationInfoPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                        new PSNoteProperty(
                            Constants.NodeDeactivationInfoProperyName,
                            deactivationInfoPSObj));
                }

                return itemPSObj;
            }
            else if (output is NodeList)
            {
                var item = output as NodeList;
                var itemPSObj = new PSObject(item);

                if (!string.IsNullOrEmpty(item.ContinuationToken))
                {
                    var parametersPropertyPSObj = new PSObject(item.ContinuationToken);
                    parametersPropertyPSObj.Members.Add(
                        new PSCodeMethod(
                            Constants.ToStringMethodName,
                            typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                    itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ContinuationTokenPropertyName,
                        parametersPropertyPSObj));
                }

                return itemPSObj;
            }

            return base.FormatOutput(output);
        }

        // Returns a query description object based on user input.
        // Including max results is optional, since we don't want to set this value for get all pages together option (the default, not GetSinglePage)
        private NodeQueryDescription GetQueryDescription(bool includeMaxResults)
        {
            var queryDescription = new NodeQueryDescription();

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                queryDescription.ContinuationToken = this.ContinuationToken;
            }

            var nodeStatusFilter = NodeStatusFilter.Default;
            if (this.StatusFilter.HasValue)
            {
                nodeStatusFilter = this.StatusFilter.Value;
            }

            queryDescription.NodeStatusFilter = nodeStatusFilter;

            if (!string.IsNullOrEmpty(this.NodeName))
            {
                queryDescription.NodeNameFilter = this.NodeName;
            }

            if (this.MaxResults.HasValue && includeMaxResults)
            {
                queryDescription.MaxResults = this.MaxResults.Value;
            }

            return queryDescription;
        }

        private void GetResultsWithPagedQuery(IClusterConnection clusterConnection)
        {
            var queryDescription = this.GetQueryDescription(true);

            var queryResult = clusterConnection.GetNodeListAsync(
                      queryDescription,
                      this.GetTimeout(),
                      this.GetCancellationToken()).Result;

            // Continuation token is not added as a PsObject because it breaks the pipeline scenarios
            // like Get-ServiceFabricNode | Get-ServiceFabricNodeHealth
            this.WriteObject(this.FormatOutput(queryResult));

            // If the user selects the "Verbose" option, then print the continuation token
            this.WriteVerbose("Continuation Token: " + queryResult.ContinuationToken);
        }

        private void GetAllQueryResultPages(IClusterConnection clusterConnection)
        {
            var queryDescription = this.GetQueryDescription(false);

            // If the results do not fit a message,
            // GetNodeListAsync returns a page of results and continuation token.
            // Keep getting data until all nodes are received.
            bool morePages = true;
            string currentContinuationToken = this.ContinuationToken;
            while (morePages)
            {
                // Updates the continuation token to get the next page
                queryDescription.ContinuationToken = currentContinuationToken;

                var queryResult = clusterConnection.GetNodeListAsync(
                          queryDescription,
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
    }
}