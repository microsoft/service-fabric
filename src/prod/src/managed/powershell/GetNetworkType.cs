// ----------------------------------------------------------------------
//  <copyright file="GetNetworkType.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNetworkType")]
    public sealed class GetNetworkType : CommonCmdletBase
    {
        // have to replace NetworkTypeQueryDescription with parameters depending on definition of "NetworkTypeQueryDescription" type in fabric client.
        [Parameter(Mandatory = true)]
        public string NetworkTypeQueryDescription
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetNetworkType\" function");
        }
    }
}