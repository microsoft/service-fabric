// ----------------------------------------------------------------------
//  <copyright file="RemoveNetworkType.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricNetworkType")]
    public sealed class RemoveNetworkType : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NetworkTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NetworkTypeVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"RemoveNetworktype\" function");
        }
    }
}
