// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using System.Linq;
    using FabricMonSvc;
    using Health;
    using Microsoft.ServiceFabric.Monitoring.Internal.EventSource;
    using Moq;
    using VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Represents the mock of MonitoringEventWriter class.
    /// </summary>
    internal class EventWriterMock : HealthTreeParser
    {
        internal EventWriterMock()
        {
            this.Mock = new Mock<MonitoringEventWriter>() { CallBase = false };
        }

        internal Mock<MonitoringEventWriter> Mock { get; private set; }

        internal void Setup_Assertions(string clusterName, TreeNode clusterNode)
        {
            this.Parse(clusterName, clusterNode);
        }

        internal void Verify_CallCount()
        {
            this.Mock
                .Verify(
                mock => mock.LogClusterHealthState(
                    It.Is<ClusterEntity>(entity => entity.ClusterName == this.ClusterName)),
                Times.Once);

            foreach (var app in this.ApplicationList)
            {
                this.Mock
                    .Verify(
                    mock => mock.LogApplicationHealthState(
                       It.Is<ApplicationEntity>(entity => entity.ApplicationName == app.OriginalString)),
                    Times.Once);
            }

            foreach (var node in this.NodeList)
            {
                this.Mock
                    .Verify(
                    mock => mock.LogNodeHealthState(
                       It.Is<NodeEntity>(entity => entity.NodeName == node)),
                    Times.Once);
            }

            foreach (var service in this.ServiceList)
            {
                this.Mock
                    .Verify(
                    mock => mock.LogServiceHealthState(
                       It.Is<ServiceEntity>(entity => entity.ServiceName == service.OriginalString)),
                    Times.Once);

                if (service.ToString().Contains("CMSvc") || service.ToString().Contains("InfraSvc"))
                {
                    this.Mock
                        .Verify(
                        mock => mock.LogServiceHealthEvent(
                           It.Is<ServiceEntity>(entity => entity.ServiceName == service.OriginalString),
                           It.IsAny<EntityHealthEvent>()),
                        Times.Exactly(2));
                }
            }

            foreach (var partition in this.PartitionList)
            {
                this.Mock
                    .Verify(
                    mock => mock.LogPartitionHealthState(
                       It.Is<PartitionEntity>(entity => entity.PartitionId == partition)),
                    Times.Once);
            }

            foreach (var replica in this.ReplicaList)
            {
                this.Mock
                    .Verify(
                    mock => mock.LogReplicaHealthState(
                        It.Is<ReplicaEntity>(entity => entity.PartitionId == replica.Item1 && entity.ReplicaId == replica.Item2)),
                    Times.Once);
            }
        }

        protected override void ParseCluserNode(string expectedClusterName, TreeNode clusterNode)
        {
            var clusterHealth = clusterNode.Health as ClusterHealth;
            this.Mock
                .Setup(mock => mock.LogClusterHealthState(It.IsAny<ClusterEntity>()))
                .Callback((ClusterEntity entity) =>
                {
                    Assert.AreEqual(expectedClusterName, entity.ClusterName);
                    this.Assert_Health(clusterHealth, entity.Health);
                });
        }

        protected override void ParseNodeHealthNode(string clusterName, TreeNode node)
        {
            var nodeHealth = node.Health as NodeHealth;
            this.Mock
                .Setup(mock => mock.LogNodeHealthState(
                    It.Is<NodeEntity>(entity => entity.NodeName == nodeHealth.NodeName)))
                .Callback((NodeEntity entity) =>
                {
                    Assert.AreEqual(clusterName, entity.ClusterName);
                    Assert.AreEqual(nodeHealth.NodeName, entity.NodeName);
                    this.Assert_Health(nodeHealth, entity.Health);
                });
        }

        protected override void ParseAppNode(string clusterName, TreeNode appNode)
        {
            var appHealth = appNode.Health as ApplicationHealth;
            this.Mock
               .Setup(mock => mock.LogApplicationHealthState(
                   It.Is<ApplicationEntity>(entity => entity.ApplicationName == appHealth.ApplicationUri.OriginalString)))
               .Callback((ApplicationEntity entity) =>
               {
                   Assert.AreEqual(clusterName, entity.ClusterName);
                   Assert.AreEqual(appHealth.ApplicationUri, entity.ApplicationName);
                   this.Assert_Health(appHealth, entity.Health);
               });
        }

        protected override void ParseServiceNode(string clusterName, Uri appUri, TreeNode serviceNode)
        {
            var serviceHealth = serviceNode.Health as ServiceHealth;
            this.Mock
                .Setup(mock => mock.LogServiceHealthState(
                    It.Is<ServiceEntity>(entity => entity.ServiceName == serviceHealth.ServiceName.OriginalString)))
                .Callback((ServiceEntity entity) =>
                {
                    Assert.AreEqual(clusterName, entity.ClusterName);
                    Assert.AreEqual(appUri.OriginalString, entity.ApplicationName);
                    Assert.AreEqual(serviceHealth.ServiceName.OriginalString, entity.ServiceName);
                    this.Assert_Health(serviceHealth, entity.Health);
                });
        }

        protected override void ParsePartitionNode(string clusterName, Uri appUri, Uri serviceUri, TreeNode partitionNode)
        {
            var partitionHealth = partitionNode.Health as PartitionHealth;
            this.Mock
                .Setup(mock => mock.LogPartitionHealthState(
                    It.Is<PartitionEntity>(entity => entity.PartitionId == partitionHealth.PartitionId)))
                .Callback((PartitionEntity entity) =>
                {
                    Assert.AreEqual(clusterName, entity.ClusterName);
                    Assert.AreEqual(appUri.OriginalString, entity.ApplicationName);
                    Assert.AreEqual(serviceUri.OriginalString, entity.ServiceName);
                    Assert.AreEqual(partitionHealth.PartitionId, entity.PartitionId);
                    this.Assert_Health(partitionHealth, entity.Health);
                });
        }

        protected override void ParseReplicaNode(string clusterName, Uri appUri, Uri serviceUri, Guid partitionId, TreeNode replicaNode)
        {
            var replicaHealth = replicaNode.Health as ReplicaHealth;
            this.Mock
                .Setup(mock => mock.LogReplicaHealthState(
                    It.Is<ReplicaEntity>(entity => entity.PartitionId == partitionId && entity.ReplicaId == replicaHealth.ReplicaId)))
                .Callback((ReplicaEntity entity) =>
                {
                    Assert.AreEqual(clusterName, entity.ClusterName);
                    Assert.AreEqual(appUri.OriginalString, entity.ApplicationName);
                    Assert.AreEqual(serviceUri.OriginalString, entity.ServiceName);
                    Assert.AreEqual(partitionId, entity.PartitionId);
                    Assert.AreEqual(replicaHealth.ReplicaId, entity.ReplicaId);
                    this.Assert_Health(replicaHealth, entity.Health);
                });
        }

        private void Assert_Health(EntityHealth expected, EntityHealth actual)
        {
            Assert.AreEqual(expected.AggregatedHealthState, actual.AggregatedHealthState);
            Assert.AreEqual(expected.GetHealthEvaluationString(), actual.GetHealthEvaluationString());

            if (expected.HealthEvents == null)
            {
                Assert.IsNull(actual.HealthEvents);
            }
            else
            {
                var expectedEvents = expected.HealthEvents.ToList();
                var actualEvents = actual.HealthEvents.ToList();
                Assert.AreEqual(expectedEvents.Count, actualEvents.Count);
                for (var index = 0; index < expectedEvents.Count; index++)
                {
                    var expectedEvent = expectedEvents[index];
                    var actualEvent = actualEvents[index];
                    Assert.AreEqual(expectedEvent.SequenceNumber, actualEvent.SequenceNumber);
                    Assert.AreEqual(expectedEvent.Property, actualEvent.Property);
                    Assert.AreEqual(expectedEvent.Description, actualEvent.Description);
                    Assert.AreEqual(expectedEvent.SourceId, actualEvent.SourceId);
                    Assert.AreEqual(expectedEvent.HealthState, actualEvent.HealthState);
                    Assert.AreEqual(expectedEvent.IsExpired, actualEvent.IsExpired);
                }
            }
        }
    }
}