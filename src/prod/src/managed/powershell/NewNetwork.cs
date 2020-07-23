// ----------------------------------------------------------------------
//  <copyright file="NewNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricNetwork")]
    public sealed class NewNetwork : NetworkCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string NetworkName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NetworkAddressPrefix
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.NewNetwork(this.NetworkName, this.NetworkAddressPrefix);
            this.WriteVerbose("Implementation to be added for \"NewNetwork\" function");
        }
    }
}