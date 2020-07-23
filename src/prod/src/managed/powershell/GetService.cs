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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricService", DefaultParameterSetName = "Non-Adhoc-AllPages")]
    public sealed class GetService : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Adhoc")]
        public SwitchParameter Adhoc
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "Non-Adhoc-SinglePage")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "Non-Adhoc-AllPages")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Non-Adhoc-SinglePage")]
        public SwitchParameter GetSinglePage
        {
            get;
            set;
        }

        // Continuation token is respected as long as it's given, regardless of parameter set.
        [Parameter(Mandatory = false, ParameterSetName = "Non-Adhoc-AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "Non-Adhoc-SinglePage")]
        public string ContinuationToken
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Non-Adhoc-SinglePage")]
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
                        Constants.GetServiceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is Service)
            {
                var item = output as Service;
                return new PSObject(item);
            }
            else if (output is ServiceList)
            {
                var item = output as ServiceList;
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

        private ServiceQueryDescription GetQueryDescription(bool includeMaxResults)
        {
            var queryDescription = new ServiceQueryDescription(this.Adhoc ? null : this.ApplicationName);

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                queryDescription.ContinuationToken = this.ContinuationToken;
            }

            if (this.MaxResults.HasValue && includeMaxResults)
            {
                queryDescription.MaxResults = this.MaxResults.Value;
            }

            if (!string.IsNullOrEmpty(this.ServiceTypeName))
            {
                queryDescription.ServiceTypeNameFilter = this.ServiceTypeName;
            }

            if (this.ServiceName != null)
            {
                queryDescription.ServiceNameFilter = this.ServiceName;
            }

            return queryDescription;
        }

        private void GetResultsWithPagedQuery(IClusterConnection clusterConnection)
        {
            var queryDescription = this.GetQueryDescription(true);

            var queryResult = clusterConnection.GetServicePagedListAsync(
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

            // If the results do not fit a message, the continuation token is set.
            // Keep getting data until all entries are received.
            bool morePages = true;
            string currentContinuationToken = null;
            while (morePages)
            {
                var queryResult = clusterConnection.GetServicePagedListAsync(
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