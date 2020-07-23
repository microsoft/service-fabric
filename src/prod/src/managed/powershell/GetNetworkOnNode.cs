// ----------------------------------------------------------------------
//  <copyright file="GetNetworkOnNode.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNetworkOnNode")]
    public sealed class GetNetworkOnNode : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public Uri NetworkName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetNetworkOnNode\" function");
        }
    }
}