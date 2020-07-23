// ----------------------------------------------------------------------
//  <copyright file="UpdateNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricNetwork")]
    public sealed class UpdateNetwork : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri NetworkName
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

        // have to replace NetworkUpdateDescription with parameters depending on definition of "NetworkUpdateDescription" type in fabric client.
        [Parameter(Mandatory = true)]
        public string NetworkUpdateDescription
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"UpdateNetwork\" function");
        }
    }
}
