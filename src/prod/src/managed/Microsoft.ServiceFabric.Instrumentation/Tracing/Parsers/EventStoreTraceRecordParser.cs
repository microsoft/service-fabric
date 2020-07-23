// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers
{
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public class EventStoreTraceRecordParser : BaseTraceRecordParser
    {
        private IList<TraceRecord> knownTraceRecords;

        public EventStoreTraceRecordParser(TraceRecordSession traceSession) : base(traceSession)
        {
            this.knownTraceRecords = new List<TraceRecord>
            {
                // FM Node Events
                new NodeUpTraceRecord(null),
                new NodeDownTraceRecord(null),
                new NodeAddedTraceRecord(null),
                new NodeRemovedTraceRecord(null),
                new NodeDeactivateStartTraceRecord(null),
                new NodeDeactivateCompleteTraceRecord(null),
                new NodeOpenedSuccessTraceRecord(null),
                new NodeOpenedFailedTraceRecord(null),
                new NodeClosedTraceRecord(null),
                new NodeAbortedTraceRecord(null),

                // App Events.
                new ApplicationCreatedTraceRecord(null),
                new ApplicationDeletedTraceRecord(null),
                new ApplicationUpgradeStartTraceRecord(null),
                new ApplicationUpgradeCompleteTraceRecord(null),
                new ApplicationUpgradeDomainCompleteTraceRecord(null),
                new ApplicationUpgradeRollbackStartTraceRecord(null),
                new ApplicationUpgradeRollbackCompleteTraceRecord(null),

                // CM Events.
                new ClusterUpgradeStartTraceRecord(null),
                new ClusterUpgradeCompleteTraceRecord(null),
                new ClusterUpgradeDomainCompleteTraceRecord(null),
                new ClusterUpgradeRollbackStartTraceRecord(null),
                new ClusterUpgradeRollbackCompleteTraceRecord(null),

                // Service Events from FM.
                new ServiceCreatedTraceRecord(null),
                new ServiceDeletedTraceRecord(null),

                // Health Events.
                new ApplicationHealthReportCreatedTraceRecord(null),
                new ApplicationHealthReportExpiredTraceRecord(null),
                new ClusterHealthReportCreatedTraceRecord(null),
                new ClusterHealthReportExpiredTraceRecord(null),
                new DeployedApplicationHealthReportCreatedTraceRecord(null),
                new DeployedApplicationHealthReportExpiredTraceRecord(null),
                new DeployedServiceHealthReportCreatedTraceRecord(null),
                new DeployedServiceHealthReportExpiredTraceRecord(null),
                new NodeHealthReportCreatedTraceRecord(null),
                new NodeHealthReportExpiredTraceRecord(null),
                new PartitionHealthReportCreatedTraceRecord(null),
                new PartitionHealthReportExpiredTraceRecord(null),
                new ServiceHealthReportCreatedTraceRecord(null),
                new ServiceHealthReportExpiredTraceRecord(null),
                new StatefulReplicaHealthReportCreatedTraceRecord(null),
                new StatefulReplicaHealthReportExpiredTraceRecord(null),
                new StatelessReplicaHealthReportCreatedTraceRecord(null),
                new StatelessReplicaHealthReportExpiredTraceRecord(null),

                // Hosting Events.
                new ContainerExitedTraceRecord(null),
                new ProcessExitedTraceRecord(null),

                // Partition
                new ReconfigurationCompletedTraceRecord(null),
                new PrimaryMoveAnalysisTraceRecord(null),

                // CorrelationTraceRecord
                new CorrelationTraceRecord(null),

                // Chaos Events
                new ChaosStartedTraceRecord(null),
                new ChaosStoppedTraceRecord(null),
                new ChaosMovePrimaryFaultScheduledTraceRecord(null),
                new ChaosMoveSecondaryFaultScheduledTraceRecord(null),
                new ChaosRemoveReplicaFaultScheduledTraceRecord(null),
                new ChaosRestartCodePackageFaultScheduledTraceRecord(null),
                new ChaosRestartNodeFaultScheduledTraceRecord(null),
                new ChaosRestartReplicaFaultScheduledTraceRecord(null)
            };
        }

        protected override IEnumerable<TraceRecord> GetKnownTraceRecords()
        {
            return this.knownTraceRecords;
        }
    }
}