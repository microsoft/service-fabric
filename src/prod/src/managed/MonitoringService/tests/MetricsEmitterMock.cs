// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using FabricMonSvc;
    using Internal;
    using MdsHealthDataConsumer;
    using Moq;

    internal class MetricsEmitterMock : HealthTreeParser
    {
        internal MetricsEmitterMock(Mock<TraceWriterWrapper> traceWriter, IInternalServiceConfiguration config)
        {
            this.Mock = new Mock<MetricsEmitter>(config, traceWriter.Object);
        }

        internal Mock<MetricsEmitter> Mock { get; private set; }

        internal void Setup(string clusterName, TreeNode clusterNode)
        {
            this.Parse(clusterName, clusterNode);
        }

        internal void Verify_CallCount()
        {
            this.Mock
                .Verify(
                mock => mock.EmitClusterHealthState(
                    It.Is<ClusterEntity>(entity => entity.ClusterName == this.ClusterName)),
                Times.Once);

            this.Mock
               .Verify(
               mock => mock.EmitClusterHealthEvent(
                   It.Is<ClusterEntity>(entity => entity.ClusterName == this.ClusterName),
                   It.IsAny<Health.EntityHealthEvent>()),
               Times.Never);

            foreach (var app in this.ApplicationList)
            {
                this.Mock
                    .Verify(
                    mock => mock.EmitApplicationHealthState(
                       It.Is<ApplicationEntity>(entity => entity.ApplicationName == app.OriginalString)),
                    Times.Once);

                this.Mock
                    .Verify(
                    mock => mock.EmitAppHealthEvent(
                       It.Is<ApplicationEntity>(entity => entity.ApplicationName == app.OriginalString),
                       It.IsAny<Health.EntityHealthEvent>()),
                    Times.Never);
            }

            foreach (var node in this.NodeList)
            {
                this.Mock
                    .Verify(
                    mock => mock.EmitNodeHealthState(
                       It.Is<NodeEntity>(entity => entity.NodeName == node)),
                    Times.Once);

                this.Mock
                    .Verify(
                    mock => mock.EmitNodeHealthEvent(
                       It.Is<NodeEntity>(entity => entity.NodeName == node),
                       It.IsAny<Health.EntityHealthEvent>()),
                    Times.Never);
            }

            foreach (var service in this.ServiceList)
            {
                this.Mock
                    .Verify(
                    mock => mock.EmitServiceHealthState(
                       It.Is<ServiceEntity>(entity => entity.ServiceName == service.OriginalString)),
                    Times.Once);

                this.Mock
                    .Verify(
                    mock => mock.EmitServiceHealthEvent(
                       It.Is<ServiceEntity>(entity => entity.ServiceName == service.OriginalString),
                       It.IsAny<Health.EntityHealthEvent>()),
                    Times.Never);
            }

            foreach (var partition in this.PartitionList)
            {
                this.Mock
                    .Verify(
                    mock => mock.EmitPartitionHealthState(
                       It.Is<PartitionEntity>(entity => entity.PartitionId == partition)),
                    Times.Once);

                this.Mock
                    .Verify(
                    mock => mock.EmitPartitionHealthEvent(
                       It.Is<PartitionEntity>(entity => entity.PartitionId == partition),
                       It.IsAny<Health.EntityHealthEvent>()),
                    Times.Never);
            }

            foreach (var replica in this.ReplicaList)
            {
                this.Mock
                    .Verify(
                    mock => mock.EmitReplicaHealthState(
                        It.Is<ReplicaEntity>(entity => entity.PartitionId == replica.Item1 && entity.ReplicaId == replica.Item2)),
                    Times.Once);

                this.Mock
                    .Verify(
                    mock => mock.EmitReplicaHealthEvent(
                        It.Is<ReplicaEntity>(entity => entity.PartitionId == replica.Item1 && entity.ReplicaId == replica.Item2),
                        It.IsAny<Health.EntityHealthEvent>()),
                    Times.Never);
            }
        }

        protected override void ParseAppNode(string clusterName, TreeNode appNode)
        {
        }

        protected override void ParseCluserNode(string clusterName, TreeNode clusterNode)
        {
        }

        protected override void ParseNodeHealthNode(string clusterName, TreeNode node)
        {
        }

        protected override void ParsePartitionNode(string clusterName, Uri appUri, Uri serviceUri, TreeNode partitionNode)
        {
        }

        protected override void ParseReplicaNode(string clusterName, Uri appUri, Uri serviceUri, Guid partitionId, TreeNode replicaNode)
        {
        }

        protected override void ParseServiceNode(string clusterName, Uri appUri, TreeNode serviceNode)
        {
        }
    }
}