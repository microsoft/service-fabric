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

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplicationType", DefaultParameterSetName = "UsePagedAPI")]
    public sealed class GetApplicationType : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "UsePagedAPI")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "UseUnpagedAPI")]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "UsePagedAPI")]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public ApplicationTypeDefinitionKindFilter? ApplicationTypeDefinitionKindFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UsePagedAPI")]
        public SwitchParameter ExcludeApplicationParameters
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

        /// <summary>
        /// Puts the results onto the output window
        /// </summary>
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (this.UsePaging || this.ExcludeApplicationParameters || !string.IsNullOrEmpty(this.ApplicationTypeVersion) || this.ApplicationTypeDefinitionKindFilter.HasValue)
                {
                    this.GetResultsWithPagedQuery(clusterConnection);
                }
                else
                {
                    this.GetResultsWithUnpagedQuery(clusterConnection);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetApplicationTypeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ApplicationType)
            {
                var item = output as ApplicationType;

                var parametersPropertyPSObj = new PSObject(item.DefaultParameters);
                parametersPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.DefaultParametersPropertyName,
                        parametersPropertyPSObj));

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }

        private void GetResultsWithPagedQuery(IClusterConnection clusterConnection)
        {
            var queryDescription = new PagedApplicationTypeQueryDescription();
            queryDescription.ExcludeApplicationParameters = this.ExcludeApplicationParameters;

            if (!string.IsNullOrEmpty(this.ApplicationTypeName))
            {
                queryDescription.ApplicationTypeNameFilter = this.ApplicationTypeName; // defaults null
            }

            if (!string.IsNullOrEmpty(this.ApplicationTypeVersion))
            {
                queryDescription.ApplicationTypeVersionFilter = this.ApplicationTypeVersion; // defaults null
            }

            if (this.ApplicationTypeDefinitionKindFilter.HasValue)
            {
                queryDescription.ApplicationTypeDefinitionKindFilter = this.ApplicationTypeDefinitionKindFilter.Value;
            }

            // If the results do not fit a message, the continuation token is set.
            // Keep getting data until all entries are received.
            bool morePages = true;
            string currentContinuationToken;

            while (morePages)
            {
                var queryResult = clusterConnection.GetApplicationTypePagedListAsync(
                                  queryDescription,
                                  this.GetTimeout(),
                                  this.GetCancellationToken()).Result;

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

        private void GetResultsWithUnpagedQuery(IClusterConnection clusterConnection)
        {
            var queryResult = clusterConnection.GetApplicationTypeListAsync(
                                      this.ApplicationTypeName,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

            foreach (var item in queryResult)
            {
                this.WriteObject(this.FormatOutput(item));
            }
        }
    }
}