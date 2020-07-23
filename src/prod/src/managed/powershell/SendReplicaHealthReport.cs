// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    /// <summary>
    /// This class will implement "Send-ReplicaHealthReport" cmdlet.
    /// This cmdlet is used for stateful service (ReplicaHealthReport) or stateless service (InstanceHealthReport)
    /// Required parameters: PartitionId or ReplicaId, HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// <summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricReplicaHealthReport",
            DefaultParameterSetName = Constants.SendReplicaHealthReportstatefulServiceParamSetName)]
    public sealed class SendReplicaHealthReport : SendHealthReport
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0,
                   HelpMessage = "PartitionId of the concerned Replica.")]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true,
                   ParameterSetName = Constants.SendReplicaHealthReportstatefulServiceParamSetName)]
        public long ReplicaId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true,
                   ParameterSetName = Constants.SendReplicaHealthReportStatelessServiceParamSetName)]
        public long InstanceId
        {
            get;
            set;
        }

        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendReplicaHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            HealthReport replicaOrInstanceHealthReport = null;
            switch (this.ParameterSetName)
            {
                /// Replica
            case Constants.SendReplicaHealthReportstatefulServiceParamSetName:
                replicaOrInstanceHealthReport =
                    new StatefulServiceReplicaHealthReport(this.PartitionId, this.ReplicaId, healthInformation);
                break;

                /// Instance
            case Constants.SendReplicaHealthReportStatelessServiceParamSetName:
                replicaOrInstanceHealthReport =
                    new StatelessServiceInstanceHealthReport(this.PartitionId, this.InstanceId, healthInformation);
                break;

            default:
                throw new ArgumentException(string.Format(
                                                CultureInfo.CurrentCulture,
                                                StringResources.Error_ReplicaHealthReportUnknownParamSet,
                                                ParameterSetName));
            }

            return replicaOrInstanceHealthReport;
        }
    }
}