// ----------------------------------------------------------------------
//  <copyright file="GetNetworkName.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricNetworkName")]
    public sealed class GetNetworkName : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetNetworkName\" function");
        }
    }
}