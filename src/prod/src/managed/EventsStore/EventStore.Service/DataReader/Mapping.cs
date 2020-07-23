// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.DataReader
{
    using System;
    using System.Collections.Generic;
    using EventsStore.EventsModels.Application;
    using EventsStore.EventsModels.Cluster;
    using EventsStore.EventsModels.Node;
    using EventsStore.EventsModels.Partition;
    using EventsStore.EventsModels.Replica;
    using EventsStore.EventsModels.Service;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    internal class ModelAndTraceRelation
    {
        public ModelAndTraceRelation(Type model, Type raw)
        {
            this.ModelType = model;
            this.UnderlyingType = raw;
        }

        public Type ModelType { get; }

        public Type UnderlyingType { get; }
    }

    internal class Mapping
    {
        public static IDictionary<EntityType, IList<ModelAndTraceRelation>> EntityToEventsMap = new Dictionary<EntityType, IList<ModelAndTraceRelation>>
        {
            { EntityType.Node, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(NodeDownEvent), typeof(NodeDownTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeUpEvent), typeof(NodeUpTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeAddedEvent), typeof(NodeAddedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeRemovedEvent), typeof(NodeRemovedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeDeactivateStartEvent), typeof(NodeDeactivateStartTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeDeactivateCompleteEvent), typeof(NodeDeactivateCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeOpenedSuccessEvent), typeof(NodeOpenedSuccessTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeOpenFailedEvent), typeof(NodeOpenedFailedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeClosedEvent), typeof(NodeClosedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeAbortedEvent), typeof(NodeAbortedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeHealthReportCreatedEvent), typeof(NodeHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(NodeHealthReportExpiredEvent), typeof(NodeHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosRestartNodeFaultScheduledEvent), typeof(ChaosRestartNodeFaultScheduledTraceRecord))} },

             { EntityType.Application, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(ApplicationCreatedEvent), typeof(ApplicationCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationDeletedEvent), typeof(ApplicationDeletedTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationUpgradeStartEvent), typeof(ApplicationUpgradeStartTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationUpgradeCompleteEvent), typeof(ApplicationUpgradeCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationUpgradeDomainCompleteEvent), typeof(ApplicationUpgradeDomainCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationUpgradeRollbackStartEvent), typeof(ApplicationUpgradeRollbackStartTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationUpgradeRollbackCompleteEvent), typeof(ApplicationUpgradeRollbackCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationHealthReportCreatedEvent), typeof(ApplicationHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(ApplicationHealthReportExpiredEvent), typeof(ApplicationHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(DeployedApplicationHealthReportCreatedEvent), typeof(DeployedApplicationHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(DeployedApplicationHealthReportExpiredEvent), typeof(DeployedApplicationHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(ContainerDeactivatedEvent), typeof(ContainerExitedTraceRecord)),
                new ModelAndTraceRelation(typeof(ProcessDeactivatedEvent), typeof(ProcessExitedTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosRestartCodePackageFaultScheduledEvent), typeof(ChaosRestartCodePackageFaultScheduledTraceRecord)),
                new ModelAndTraceRelation(typeof(DeployedServiceHealthReportCreatedEvent), typeof(DeployedServiceHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(DeployedServiceHealthReportExpiredEvent), typeof(DeployedServiceHealthReportExpiredTraceRecord))} },

              { EntityType.Service, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(ServiceCreatedEvent), typeof(ServiceCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(ServiceDeletedEvent), typeof(ServiceDeletedTraceRecord)),
                new ModelAndTraceRelation(typeof(ServiceHealthReportCreatedEvent), typeof(ServiceHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(ServiceHealthReportExpiredEvent), typeof(ServiceHealthReportExpiredTraceRecord))} },

             { EntityType.Cluster, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(ClusterUpgradeStartEvent), typeof(ClusterUpgradeStartTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterUpgradeCompleteEvent), typeof(ClusterUpgradeCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterUpgradeDomainCompleteEvent), typeof(ClusterUpgradeDomainCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterUpgradeRollbackStartEvent), typeof(ClusterUpgradeRollbackStartTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterUpgradeRollbackCompleteEvent), typeof(ClusterUpgradeRollbackCompleteTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterHealthReportCreatedEvent), typeof(ClusterHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(ClusterHealthReportExpiredEvent), typeof(ClusterHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosStartedEvent), typeof(ChaosStartedTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosStoppedEvent), typeof(ChaosStoppedTraceRecord))} },

             { EntityType.Partition, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(PartitionHealthReportCreatedEvent), typeof(PartitionHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(PartitionHealthReportExpiredEvent), typeof(PartitionHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(PartitionReconfigurationCompletedEvent), typeof(ReconfigurationCompletedTraceRecord)),
                new ModelAndTraceRelation(typeof(PartitionPrimaryMoveAnalysisEvent), typeof(PrimaryMoveAnalysisTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosMovePrimaryFaultScheduledEvent), typeof(ChaosMovePrimaryFaultScheduledTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosMoveSecondaryFaultScheduledEvent), typeof(ChaosMoveSecondaryFaultScheduledTraceRecord))} },

             { EntityType.Replica, new List<ModelAndTraceRelation> {
                new ModelAndTraceRelation(typeof(StatefulReplicaHealthReportCreatedEvent), typeof(StatefulReplicaHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(StatefulReplicaHealthReportExpiredEvent), typeof(StatefulReplicaHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(StatelessReplicaHealthReportCreatedEvent), typeof(StatelessReplicaHealthReportCreatedTraceRecord)),
                new ModelAndTraceRelation(typeof(StatelessReplicaHealthReportExpiredEvent), typeof(StatelessReplicaHealthReportExpiredTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosRemoveReplicaFaultScheduledEvent), typeof(ChaosRemoveReplicaFaultScheduledTraceRecord)),
                new ModelAndTraceRelation(typeof(ChaosRestartReplicaFaultScheduledEvent), typeof(ChaosRestartReplicaFaultScheduledTraceRecord))} },
        };
    }
}