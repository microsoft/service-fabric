// ----------------------------------------------------------------------
//  <copyright file="StartNetworkOnNode.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricNetworkOnNode")]
    public sealed class StartNetworkOnNode : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public Uri NetworkName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NodeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"StartNetworkOnNode\" function");
        }
    }
}
