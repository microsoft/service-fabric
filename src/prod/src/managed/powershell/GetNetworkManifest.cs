// ----------------------------------------------------------------------
//  <copyright file="GetNetworkManifest.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNetworkManifest")]
    public sealed class GetNetworkManifest : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NetworkType
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NetworkVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetNetworkManifest\" function");
        }
    }
}