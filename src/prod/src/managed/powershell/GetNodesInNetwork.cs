// ----------------------------------------------------------------------
//  <copyright file="GetNodesInNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "NodesInServiceFabricNetwork")]
    public sealed class GetNodesInNetwork : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public Uri NetworkName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string NodeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetNodesInNetwork\" function");
        }
    }
}