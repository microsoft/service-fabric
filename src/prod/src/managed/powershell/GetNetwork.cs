// ----------------------------------------------------------------------
//  <copyright file="GetNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNetwork")]
    public sealed class GetNetwork : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string Name
        {
            get;
            set;
        }

        // Continuation token is respected.
        [Parameter(Mandatory = false)]
        public string ContinuationToken
        {
            get;
            set;
        }

        // You need to select GetSinglePage for this to work
        [Parameter(Mandatory = false)]
        [ValidateRange(0, long.MaxValue)]
        public long? MaxResults
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter GetSinglePage
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (!this.GetSinglePage.IsPresent || !this.GetSinglePage.ToBool())
                {
                    this.GetNetworks(clusterConnection);
                }
                else
                {
                    if (this.MaxResults.HasValue)
                    {
                        this.WriteWarning(StringResources.Powershell_MaxResultsIgnored);
                    }

                    this.GetNetworks(clusterConnection);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetNetworkErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            //--------
            // Make the output cleaner.
            if (output is LocalNetworkInformation)
            {
                var item = output as LocalNetworkInformation;
                var powershellObj = new PSObject();
                powershellObj.Properties.Add(new PSNoteProperty(Constants.NetworkDescriptionNetworkNamePropertyName, item.NetworkName));
                powershellObj.Properties.Add(new PSNoteProperty(Constants.NetworkDescriptionNetworkTypePropertyName, item.NetworkType));
                powershellObj.Properties.Add(new PSNoteProperty(Constants.NetworkDescriptionNetworkAddressPrefixPropertyName, item.NetworkConfiguration.NetworkAddressPrefix));
                powershellObj.Properties.Add(new PSNoteProperty(Constants.NetworkDescriptionNetworkStatusPropertyName, item.NetworkStatus));

                return powershellObj;
            }

            return base.FormatOutput(output);
        }
        
        private NetworkQueryDescription GetNetworkQueryDescription()
        {
            var queryDescription = new NetworkQueryDescription();

            if (!string.IsNullOrEmpty(this.Name))
            {
                queryDescription.NetworkNameFilter = this.Name;
            }

            if (this.MaxResults.HasValue)
            {
                queryDescription.MaxResults = this.MaxResults.Value;
            }

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                queryDescription.ContinuationToken = this.ContinuationToken;
            }

            return queryDescription;
        }

        private void GetNetworks(IClusterConnection clusterConnection)
        {
            // If the results do not fit a message, the continuation token is set.
            // Keep getting data until all entries are received.
            bool morePages = true;
            string currentContinuationToken = null;

            currentContinuationToken = this.ContinuationToken;

            var queryDescription = this.GetNetworkQueryDescription();

            while (morePages)
            {
                var queryResult = clusterConnection.FabricClient.NetworkManager.GetNetworkListAsync(
                    queryDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }

                morePages = Helpers.ResultHasMorePages(this, queryResult.ContinuationToken, out currentContinuationToken);

                //--------
                // Keep going on if single result.
                if (!this.GetSinglePage.IsPresent || !this.GetSinglePage.ToBool())
                {
                    morePages = false;
                    break;
                }

                queryDescription.ContinuationToken = currentContinuationToken;
            }
        }
    }
}