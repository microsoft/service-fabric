// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    /// <summary>
    /// Encapsulates static mapping. This needs to be created for each Record that we can Read. Yes, this file will grow into a big one.
    /// </summary>
    public class Mapping
    {
        public const string AppTable = "ApplicationsOps";
        public const string NodeTable = "NodesOps";
        public const string ServiceTable = "ServicesOps";
        public const string ClusterTable = "ClusterOps";
        public const string PartitionTable = "PartitionsOps";
        public const string ReplicaTable = "ReplicasOps";
        public const string CorrelationTable = "CorrelationOps";

        public static readonly IList<TraceRecordAzureTableMetadata> TraceRecordAzureDataMap = new List<TraceRecordAzureTableMetadata>
        {
            // App Events.
            new TraceRecordAzureTableMetadata(typeof(ApplicationCreatedTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationCreatedOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationDeletedTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationDeletedOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationUpgradeStartTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationUpgradeStartOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationUpgradeCompleteTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationUpgradeCompleteOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationUpgradeDomainCompleteTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationUpgradeDomainCompleteOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationUpgradeRollbackStartTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationUpgradeRollbackStartOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationUpgradeRollbackCompleteTraceRecord), PartitionDataType.StaticData, AppTable, "ApplicationUpgradeRollbackCompleteOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ContainerExitedTraceRecord), PartitionDataType.StaticData, AppTable, "ContainerExitedOperational", TaskName.Hosting),
            new TraceRecordAzureTableMetadata(typeof(ProcessExitedTraceRecord), PartitionDataType.StaticData, AppTable, "ProcessExitedOperational", TaskName.Hosting),

            // Cluster Events.
            new TraceRecordAzureTableMetadata(typeof(ClusterUpgradeStartTraceRecord), PartitionDataType.StaticData, ClusterTable, "ClusterUpgradeStartOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ClusterUpgradeCompleteTraceRecord), PartitionDataType.StaticData, ClusterTable, "ClusterUpgradeCompleteOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ClusterUpgradeDomainCompleteTraceRecord), PartitionDataType.StaticData, ClusterTable, "ClusterUpgradeDomainCompleteOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ClusterUpgradeRollbackStartTraceRecord), PartitionDataType.StaticData, ClusterTable, "ClusterUpgradeRollbackStartOperational", TaskName.CM),
            new TraceRecordAzureTableMetadata(typeof(ClusterUpgradeRollbackCompleteTraceRecord), PartitionDataType.StaticData, ClusterTable, "ClusterUpgradeRollbackCompleteOperational", TaskName.CM),

            // Node Events from FM
            new TraceRecordAzureTableMetadata(typeof(NodeUpTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeUpOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(NodeDownTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeDownOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(NodeAddedTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeAddedOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(NodeRemovedTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeRemovedOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(NodeDeactivateStartTraceRecord), PartitionDataType.StaticData, NodeTable, "DeactivateNodeStartOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(NodeDeactivateCompleteTraceRecord), PartitionDataType.StaticData, NodeTable, "DeactivateNodeCompletedOperational", TaskName.FM),
            
            // Node Events from Fabric Node
            new TraceRecordAzureTableMetadata(typeof(NodeOpenedSuccessTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeOpenedSuccessOperational", TaskName.FabricNode),
            new TraceRecordAzureTableMetadata(typeof(NodeOpenedFailedTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeOpenedFailedOperational", TaskName.FabricNode),
            new TraceRecordAzureTableMetadata(typeof(NodeClosedTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeClosedOperational", TaskName.FabricNode),
            new TraceRecordAzureTableMetadata(typeof(NodeAbortedTraceRecord), PartitionDataType.StaticData, NodeTable, "NodeAbortedOperational", TaskName.FabricNode),

            // Service Events.
            new TraceRecordAzureTableMetadata(typeof(ServiceCreatedTraceRecord), PartitionDataType.StaticData, ServiceTable, "ServiceCreatedOperational", TaskName.FM),
            new TraceRecordAzureTableMetadata(typeof(ServiceDeletedTraceRecord), PartitionDataType.StaticData, ServiceTable, "ServiceDeletedOperational", TaskName.FM),

            // Health Events (all).
            new TraceRecordAzureTableMetadata(typeof(ApplicationHealthReportCreatedTraceRecord), PartitionDataType.StaticData, AppTable, "ProcessApplicationReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(ApplicationHealthReportExpiredTraceRecord), PartitionDataType.StaticData, AppTable, "ExpiredApplicationEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(DeployedApplicationHealthReportCreatedTraceRecord), PartitionDataType.StaticData, AppTable, "ProcessDeployedApplicationReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(DeployedApplicationHealthReportExpiredTraceRecord), PartitionDataType.StaticData, AppTable, "ExpiredDeployedApplicationEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(ServiceHealthReportCreatedTraceRecord), PartitionDataType.StaticData, ServiceTable, "ProcessServiceReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(ServiceHealthReportExpiredTraceRecord), PartitionDataType.StaticData, ServiceTable, "ExpiredServiceEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(DeployedServiceHealthReportCreatedTraceRecord), PartitionDataType.StaticData, AppTable, "ProcessDeployedServicePackageReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(DeployedServiceHealthReportExpiredTraceRecord), PartitionDataType.StaticData, AppTable, "ExpiredDeployedServicePackageEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(StatefulReplicaHealthReportCreatedTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ProcessStatefulReplicaReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(StatefulReplicaHealthReportExpiredTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ExpiredStatefulReplicaEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(StatelessReplicaHealthReportCreatedTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ProcessStatelessInstanceReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(StatelessReplicaHealthReportExpiredTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ExpiredStatelessInstanceEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(ClusterHealthReportCreatedTraceRecord), PartitionDataType.StaticData, ClusterTable, "ProcessClusterReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(ClusterHealthReportExpiredTraceRecord), PartitionDataType.StaticData, ClusterTable, "ExpiredClusterEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(NodeHealthReportCreatedTraceRecord), PartitionDataType.StaticData, NodeTable, "ProcessNodeReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(NodeHealthReportExpiredTraceRecord), PartitionDataType.StaticData, NodeTable, "ExpiredNodeEventOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(PartitionHealthReportCreatedTraceRecord), PartitionDataType.StaticData, PartitionTable, "ProcessPartitionReportOperational", TaskName.HM),
            new TraceRecordAzureTableMetadata(typeof(PartitionHealthReportExpiredTraceRecord), PartitionDataType.StaticData, PartitionTable, "ExpiredPartitionEventOperational", TaskName.HM),

            // Partition
            new TraceRecordAzureTableMetadata(typeof(ReconfigurationCompletedTraceRecord), PartitionDataType.StaticData, PartitionTable, "ReconfigurationCompleted", TaskName.RA),
            new TraceRecordAzureTableMetadata(typeof(PrimaryMoveAnalysisTraceRecord), PartitionDataType.StaticData, PartitionTable, "PrimaryMoveAnalysisEvent", TaskName.Testability),

            // Correlation Event
            new TraceRecordAzureTableMetadata(typeof(CorrelationTraceRecord), PartitionDataType.StaticData, CorrelationTable, "CorrelationOperational", TaskName.Testability),

            // Chaos Event
            new TraceRecordAzureTableMetadata(typeof(ChaosStartedTraceRecord), PartitionDataType.StaticData, ClusterTable, "ChaosStartedEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosStoppedTraceRecord), PartitionDataType.StaticData, ClusterTable, "ChaosStoppedEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosMovePrimaryFaultScheduledTraceRecord), PartitionDataType.StaticData, PartitionTable, "ChaosMovePrimaryFaultScheduledEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosMoveSecondaryFaultScheduledTraceRecord), PartitionDataType.StaticData, PartitionTable, "ChaosMoveSecondaryFaultScheduledEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosRemoveReplicaFaultScheduledTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ChaosRemoveReplicaFaultScheduledEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosRestartCodePackageFaultScheduledTraceRecord), PartitionDataType.StaticData, AppTable, "ChaosRestartCodePackageFaultScheduledEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosRestartNodeFaultScheduledTraceRecord), PartitionDataType.StaticData, NodeTable, "ChaosRestartNodeFaultScheduledEvent", TaskName.Testability),
            new TraceRecordAzureTableMetadata(typeof(ChaosRestartReplicaFaultScheduledTraceRecord), PartitionDataType.StaticData, ReplicaTable, "ChaosRestartReplicaFaultScheduledEvent", TaskName.Testability),
        };
    }
}