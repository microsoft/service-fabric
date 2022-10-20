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
    /// This class will implement "Send-ServiceHealthReport" cmdlet.
    /// Required parameters: ServiceName, HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// <summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricServiceHealthReport")]
    public sealed class SendServiceHealthReport : SendHealthReport
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0,
                   HelpMessage = "Name of the concerned service.")]
        public Uri ServiceName
        {
            get;
            set;
        }

        /// Any exception during this cmdlet will be reported with following errorId.
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendServiceHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            return new ServiceHealthReport(this.ServiceName, healthInformation);
        }
    }
}