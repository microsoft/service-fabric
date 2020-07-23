// ----------------------------------------------------------------------
//  <copyright file="GetApplicationsInNetwork.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ApplicationsInServiceFabricNetwork")]
    public sealed class GetApplicationsInNetwork : CommonCmdletBase
    {
        // have to replace NetworkApplicationQueryDescription with parameters depending on definition of "NetworkApplicationQueryDescription" type in fabric client.
        [Parameter(Mandatory = false, ParameterSetName = "Global")]
        public string NetworkApplicationQueryDescription
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Local")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Local")]
        public Uri NetworkName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Local")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteVerbose("Implementation to be added for \"GetApplicationsInNetwork\" function");
        }
    }
}