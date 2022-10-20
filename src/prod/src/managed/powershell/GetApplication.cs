// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplication", DefaultParameterSetName = "AllPages")]
    public sealed class GetApplication : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "SinglePage")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "SinglePage")]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        public SwitchParameter ExcludeApplicationParameters
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

        // Continuation token is respected as long as it's given, regardless of single page or all page selection.
        [Parameter(Mandatory = false, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        public string ContinuationToken
        {
            get;
            set;
        }

        // You need to select GetSinglePage for this to work
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        [ValidateRange(0, long.MaxValue)]
        public long? MaxResults
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "AllPages")]
        [Parameter(Mandatory = false, ParameterSetName = "SinglePage")]
        public ApplicationDefinitionKindFilter? ApplicationDefinitionKindFilter
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
                    if (this.MaxResults.HasValue)
                    {
                        this.WriteWarning(StringResources.Powershell_MaxResultsIgnored);
                    }

                    this.GetAllQueryResultPages(clusterConnection);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetApplicationErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is Application)
            {
                var item = output as Application;

                var parametersPropertyPSObj = new PSObject(item.ApplicationParameters);
                parametersPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ApplicationParametersPropertyName,
                        parametersPropertyPSObj));

                return itemPSObj;
            }
            else if (output is ApplicationList)
            {
                var item = output as ApplicationList;
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
            else
            {
                return base.FormatOutput(output);
            }
        }

        private void GetResultsWithPagedQuery(IClusterConnection clusterConnection)
        {
            var queryDescription = new ApplicationQueryDescription();
            queryDescription.ExcludeApplicationParameters = this.ExcludeApplicationParameters;

            if (this.MaxResults.HasValue)
            {
                queryDescription.MaxResults = this.MaxResults.Value;
            }

            if (!string.IsNullOrEmpty(this.ApplicationTypeName))
            {
                queryDescription.ApplicationTypeNameFilter = this.ApplicationTypeName;
            }

            if ((this.ApplicationName != null) && (!string.IsNullOrEmpty(this.ApplicationName.OriginalString)))
            {
                queryDescription.ApplicationNameFilter = this.ApplicationName;
            }

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                queryDescription.ContinuationToken = this.ContinuationToken;
            }

            if (this.ApplicationDefinitionKindFilter.HasValue)
            {
                queryDescription.ApplicationDefinitionKindFilter = this.ApplicationDefinitionKindFilter.Value;
            }

            var queryResult = clusterConnection.GetApplicationPagedListAsync(
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
            // If the results do not fit a message, the continuation token is set.
            // Keep getting data until all entries are received.
            bool morePages = true;
            string currentContinuationToken = null;

            currentContinuationToken = this.ContinuationToken;

            var queryDescription = new ApplicationQueryDescription()
            {
                ApplicationNameFilter = this.ApplicationName,
                ApplicationTypeNameFilter = this.ApplicationTypeName,
                ContinuationToken = currentContinuationToken,
                ExcludeApplicationParameters = this.ExcludeApplicationParameters
            };

            if (this.ApplicationDefinitionKindFilter.HasValue)
            {
                queryDescription.ApplicationDefinitionKindFilter = this.ApplicationDefinitionKindFilter.Value;
            }

            while (morePages)
            {
                var queryResult = clusterConnection.GetApplicationPagedListAsync(
                    queryDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }

                morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);

                // Update continuation token
                queryDescription.ContinuationToken = currentContinuationToken;
            }
        }
    }
}