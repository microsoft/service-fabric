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
    /// This class will implement "Send-PartitionHealthReport" cmdlet.
    /// Required parameters: PartitionId, HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// <summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricPartitionHealthReport")]
    public sealed class SendPartitionHealthReport : SendHealthReport
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0,
                   HelpMessage = "Name of the concerned Partition.")]
        public Guid PartitionId
        {
            get;
            set;
        }

        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendPartitionHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            return new PartitionHealthReport(this.PartitionId, healthInformation);
        }
    }
}