// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Health;
    using System.Management.Automation;

    /// <summary>
    /// This class will implement "Send-DeployedApplicationHealthReport" cmdlet.
    /// Required parameters: ApplicationName, NodeName, HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// </summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricDeployedApplicationHealthReport")]
    public sealed class SendDeployedApplicationHealthReport : SendHealthReport
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0,
                   HelpMessage = "Name of the concerned deployed Application. e.g. fabric:/myapps/calc")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1,
                   HelpMessage = "Name of the concerned node where the application is deployed.")]
        public string NodeName
        {
            get;
            set;
        }

        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendDeployedApplicationHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            return new DeployedApplicationHealthReport(
                       this.ApplicationName,
                       this.NodeName,
                       healthInformation);
        }
    }
}