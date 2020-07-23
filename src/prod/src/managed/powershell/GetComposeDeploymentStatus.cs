// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricComposeDeploymentStatus")]
    public sealed class GetComposeDeploymentStatus : ComposeDeploymentCmdletBase
    {
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string DeploymentName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? MaxResults
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            
            var queryDescription = new ComposeDeploymentStatusQueryDescription()
            {
                DeploymentNameFilter = this.DeploymentName
            };

            if (this.MaxResults.HasValue)
            {
                queryDescription.MaxResults = this.MaxResults.Value;
            }

            try
            {
                this.GetAllPagesImpl(queryDescription, clusterConnection);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetComposeDeploymentStatusErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
        
        protected override object FormatOutput(object output)
        {
            if (output is ComposeDeploymentStatusResultItem)
            {
                var item = output as ComposeDeploymentStatusResultItem;

                var itemPSObj = new PSObject(item);
                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }

        private void GetAllPagesImpl(ComposeDeploymentStatusQueryDescription queryDescription, IClusterConnection clusterConnection)
        {
            bool morePages = true;
            string currentContinuationToken = queryDescription.ContinuationToken;
            
            while (morePages)
            {
                // Override continuation token
                queryDescription.ContinuationToken = currentContinuationToken;

                var queryResult = clusterConnection.GetComposeDeploymentStatusPagedListAsync(
                    queryDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }

                morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);
            }
        }
    }
}