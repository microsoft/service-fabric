// ----------------------------------------------------------------------
//  <copyright file="CopyNetworkManifest.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Copy, "ServiceFabricNetworkManifest")]
    public sealed class CopyNetworkManifest : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NetworkManifestPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ImageStoreConnectionString
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string NetworkManifestPathInImageStore
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"CopyNetworkManifest\" functions");
        }
    }
}
