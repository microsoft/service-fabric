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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedApplication", DefaultParameterSetName = "UsePagedAPI")]
    public sealed class GetDeployedApplication : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "UsePagedAPI")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "UseUnpagedAPI")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "UsePagedAPI")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "UseUnpagedAPI")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public SwitchParameter UsePaging
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public SwitchParameter GetSinglePage
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public SwitchParameter IncludeHealthState
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        [ValidateRange(0, long.MaxValue)]
        public long? MaxResults
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public string ContinuationToken
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            // Have different behavior depending on UsePagingAPI is selected
            if (this.UsePaging || !string.IsNullOrEmpty(this.ContinuationToken) || this.MaxResults.HasValue || this.GetSinglePage || this.IncludeHealthState)
            {
                this.ProcessRecordPaged();
            }
            else
            {
                this.ProcessRecordUnpaged();
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedApplication)
            {
                var item = output as DeployedApplication;
                var result = new PSObject(item);

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.NodeNamePropertyName,
                        this.NodeName));

                return result;
            }
            else if (output is DeployedApplicationPagedList)
            {
                var item = output as DeployedApplicationPagedList;
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

        private void ProcessRecordPaged()
        {
            var clusterConnection = this.GetClusterConnection();

            var queryDescription = new PagedDeployedApplicationQueryDescription(this.NodeName);

            queryDescription.IncludeHealthState = this.IncludeHealthState;

            if (this.MaxResults.HasValue)
            {
                if (this.GetSinglePage)
                {
                    queryDescription.MaxResults = this.MaxResults.Value;
                }
                else
                {
                    this.WriteWarning(StringResources.Powershell_MaxResultsIgnored);
                }
            }

            if (this.ApplicationName != null && !string.IsNullOrEmpty(this.ApplicationName.OriginalString))
            {
                queryDescription.ApplicationNameFilter = this.ApplicationName; // defaults null
            }

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                queryDescription.ContinuationToken = this.ContinuationToken; // defaults null
            }

            try
            {
                if (this.GetSinglePage)
                {
                    var queryResult = this.GetPage(queryDescription);

                    this.WriteObject(this.FormatOutput(queryResult));

                    // If the user selects the "Verbose" option, then print the continuation token
                    this.WriteVerbose("Continuation Token: " + queryResult.ContinuationToken);
                }
                else
                {
                    // Get all pages
                    bool morePages = true;
                    string currentContinuationToken;

                    while (morePages)
                    {
                        var queryResult = this.GetPage(queryDescription);

                        // Write results to PowerShell
                        foreach (var item in queryResult)
                        {
                            this.WriteObject(this.FormatOutput(item));
                        }

                        // Continuation token is not added as a PsObject because it breaks the pipeline scenarios
                        // like Get-ServiceFabricApplicationType | Query_Which_Takes_AppType
                        morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);

                        // Update continuation token
                        queryDescription.ContinuationToken = currentContinuationToken;
                    }
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedServiceManifestErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        private void ProcessRecordUnpaged()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var queryResult = clusterConnection.GetDeployedApplicationListAsync(
                                      this.NodeName,
                                      this.ApplicationName,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedServiceManifestErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        private DeployedApplicationPagedList GetPage(PagedDeployedApplicationQueryDescription queryDescription)
        {
            return this.GetClusterConnection().GetDeployedApplicationPagedListAsync(
                              queryDescription,
                              this.GetTimeout(),
                              this.GetCancellationToken()).Result;
        }
    }
}