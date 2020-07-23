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
    /// This class will implement "Send-DeployedServicePackageHealthReport" cmdlet.
    /// Required parameters: ApplicationName, NodeName, ServiceManifestName, HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// <summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricDeployedServicePackageHealthReport")]
    public sealed class SendDeployedServicePackageHealthReport : SendHealthReport
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0,
                   HelpMessage = "Name of the concerned deployed Application. e.g. fabric:/myapps/calc")]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1,
                   HelpMessage = "ServiceManifest name of the concerned service package.")]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 2,
                   HelpMessage = "Name of the concerned node where the application is deployed.")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 3,
                   HelpMessage = "ActivationId of concerned service package.")]
        public string ServicePackageActivationId
        {
            get;
            set;
        }

        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendDeployedServicePackageHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            return new DeployedServicePackageHealthReport(
                       this.ApplicationName,
                       this.ServiceManifestName,
                       this.ServicePackageActivationId ?? string.Empty,
                       this.NodeName,
                       healthInformation);
        }
    }
}