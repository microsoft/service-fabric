// ----------------------------------------------------------------------
//  <copyright file="StopNetworkOnNode.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Stop, "ServiceFabricNetworkOnNode")]
    public sealed class StopNetworkOnNode : CommonCmdletBase
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
            this.WriteVerbose("Implementation to be added for \"StopNetworkOnNode\" function");
        }
    }
}
