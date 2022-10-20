// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using FabricMonSvc;
    using Health;
    using Moq;
    using VisualStudio.TestTools.UnitTesting;
    using FabricHealth = System.Fabric.Health;

    /// <summary>
    /// Mocks the FabricClient.HealthClient
    /// </summary>
    internal class FabricClientMock : HealthTreeParser
    {
        private readonly CancellationToken token;
        private readonly Mock<TraceWriterWrapper> traceWriterMock;

        internal FabricClientMock(CancellationToken token, Mock<TraceWriterWrapper> traceWriter)
            : base()
        {
            this.token = token;
            this.traceWriterMock = traceWriter;
            this.Mock = new Mock<FabricHealthClientWrapper>(
                this.traceWriterMock.Object,
                "FabricMonitoringServiceType",
                Guid.Empty,
                1)
            {
                CallBase = true
            };

            this.Mock
                .Setup(mock => mock.ReportHealth(
                    It.Is<FabricHealth.HealthState>(state => state == FabricHealth.HealthState.Ok),
                    It.IsAny<string>(),
                    It.IsAny<string>()))
                .Verifiable();
            this.Mock
                .Setup(mock => mock.ReportHealth(
                    It.Is<FabricHealth.HealthState>(state => state == FabricHealth.HealthState.Error),
                    It.IsAny<string>(),
                    It.IsAny<string>()))
                .Callback(() => { Assert.Fail("Health data read pass reported error."); });
        }

        internal Mock<FabricHealthClientWrapper> Mock { get; private set; }

        internal void Setup(string clusterName, TreeNode clusterNode)
        {
            this.Parse(clusterName, clusterNode);

            this.Mock
                .Setup(mock => mock.ReportHealth(
                    It.Is<FabricHealth.HealthState>(state => state == FabricHealth.HealthState.Ok),
                    It.IsAny<string>(),
                    It.IsAny<string>()))
                .Verifiable();
            this.Mock
                .Setup(mock => mock.ReportHealth(
                    It.Is<FabricHealth.HealthState>(state => state == FabricHealth.HealthState.Error),
                    It.IsAny<string>(),
                    It.IsAny<string>()))
                .Callback(() => { Assert.Fail("Health data read pass reported error."); });
        }

        protected override void ParseAppNode(string clusterName, TreeNode appNode)
        {
            var appHealth = appNode.Health as ApplicationHealth;
            var serviceTreeNodes = appNode.AsEnumerable().Where(node => node.Health is ServiceHealth);
            var serviceNodes = serviceTreeNodes.Select(node => node.Health).Cast<ServiceHealth>();
            var serviceStates = serviceNodes.Select(node => new ServiceHealthState(node.ServiceName, node.AggregatedHealthState));
            var appHealthMock = this.Create_ApplicationHealthMock(
                appHealth.ApplicationUri,
                appHealth.AggregatedHealthState,
                serviceStates,
                Enumerable.Empty<DeployedApplicationHealthState>(),
                appHealth.HealthEvents,
                appHealth.UnhealthyEvaluations);

            this.Mock
            .Setup(mock => mock.GetApplicationHealthAsync(
                It.Is<Uri>(uri => uri.Equals(appHealthMock.ApplicationUri)),
                It.IsAny<TimeSpan>(),
                It.Is<CancellationToken>(token => token.Equals(this.token))))
            .Returns(Task.FromResult(appHealthMock));
        }

        protected override void ParseCluserNode(string clusterName, TreeNode clusterNode)
        {
            var clusterHealth = clusterNode.Health as ClusterHealth;

            var appTreeNodes = clusterNode.AsEnumerable().Where(node => node.Health is ApplicationHealth);
            var appNodes = appTreeNodes.Select(node => node.Health).Cast<ApplicationHealth>();
            var appHealthStateNodes = appNodes.Select(node => new ApplicationHealthState(node.ApplicationUri, node.AggregatedHealthState));

            var nodeTreeNodes = clusterNode.AsEnumerable().Where(node => node.Health is NodeHealth);
            var nodeNodes = nodeTreeNodes.Select(node => node.Health).Cast<NodeHealth>();
            var nodeHealthStates = nodeNodes.Select(node => new NodeHealthState(node.NodeName, node.AggregatedHealthState));

            var clusterHealthMock = this.Create_ClusterHealthMock(
                clusterHealth.AggregatedHealthState, 
                appHealthStateNodes, 
                nodeHealthStates,
                clusterHealth.HealthEvents,
                clusterHealth.UnhealthyEvaluations);

            this.Mock
               .Setup(mock => mock.GetClusterHealthAsync(
                   It.IsAny<TimeSpan>(),
                   It.Is<CancellationToken>(token => token.Equals(this.token))))
               .Returns(Task.FromResult(clusterHealthMock));
        }

        protected override void ParseNodeHealthNode(string clusterName, TreeNode node)
        {
            var nodeHealth = node.Health as NodeHealth;
            var nodeHealthState = new NodeHealthState(nodeHealth.NodeName, nodeHealth.AggregatedHealthState);
            var nodeMock = this.Create_NodeHealthMock(
                nodeHealth.NodeName, 
                nodeHealth.AggregatedHealthState,
                nodeHealth.HealthEvents,
                nodeHealth.UnhealthyEvaluations);

            this.Mock
            .Setup(mock => mock.GetNodeHealthAsync(
                It.Is<string>(nodeName => nodeName == nodeMock.NodeName),
                It.IsAny<TimeSpan>(),
                It.Is<CancellationToken>(token => token.Equals(this.token))))
            .Returns(Task.FromResult(nodeMock));
        }

        protected override void ParsePartitionNode(string clusterName, Uri appUri, Uri serviceUri, TreeNode partitionNode)
        {
            var partitionHealth = partitionNode.Health as PartitionHealth;
            var replicaTreeNodes = partitionNode.AsEnumerable();
            var replicaNodes = partitionNode.AsEnumerable().Select(node => node.Health).Cast<ReplicaHealth>();
            var replicaHealthStates = replicaNodes.Select(node => new ReplicaHealthState(node.PartitionId, node.ReplicaId, node.AggregatedHealthState));

            var partitionMock = this.Create_PartitionHealthMock(
                partitionHealth.PartitionId,
                partitionHealth.AggregatedHealthState,
                replicaHealthStates,
                partitionHealth.HealthEvents,
                partitionHealth.UnhealthyEvaluations);

            this.Mock
            .Setup(mock => mock.GetPartitionHealthAsync(
                It.Is<Guid>(partitionId => partitionId == partitionMock.PartitionId),
                It.IsAny<TimeSpan>(),
                It.Is<CancellationToken>(token => token.Equals(this.token))))
            .Returns(Task.FromResult(partitionMock));
        }

        protected override void ParseReplicaNode(string clusterName, Uri appUri, Uri serviceUri, Guid partitionId, TreeNode replicaNode)
        {
            var replicaHealth = replicaNode.Health as ReplicaHealth;
            var replicaMock = this.Create_ReplicaHealthMock(
                replicaHealth.PartitionId, 
                replicaHealth.ReplicaId, 
                replicaHealth.AggregatedHealthState,
                replicaHealth.HealthEvents,
                replicaHealth.UnhealthyEvaluations);

            this.Mock
            .Setup(mock => mock.GetReplicaHealthAsync(
                It.Is<Guid>(pid => pid == replicaMock.PartitionId),
                It.Is<long>(replicaId => replicaId == replicaMock.ReplicaId),
                It.IsAny<TimeSpan>(),
                It.Is<CancellationToken>(token => token.Equals(this.token))))
            .Returns(Task.FromResult(replicaMock));
        }

        protected override void ParseServiceNode(string clusterName, Uri appUri, TreeNode serviceNode)
        {
            var serviceHealth = serviceNode.Health as ServiceHealth;
            var partitionTreeNodes = serviceNode.AsEnumerable();
            var partitionNodes = partitionTreeNodes.Select(node => node.Health).Cast<PartitionHealth>();
            var partitionStates = partitionNodes.Select(node => new PartitionHealthState(node.PartitionId, node.AggregatedHealthState));
            var serviceMock = this.Create_ServiceHealthMock(
                serviceHealth.ServiceName,
                serviceHealth.AggregatedHealthState,
                partitionStates,
                serviceHealth.HealthEvents,
                serviceHealth.UnhealthyEvaluations);

            this.Mock
            .Setup(mock => mock.GetServiceHealthAsync(
                It.Is<Uri>(serviceUri => serviceUri == serviceMock.ServiceName),
                It.IsAny<TimeSpan>(),
                It.Is<CancellationToken>(token => token.Equals(this.token))))
            .Returns(Task.FromResult(serviceMock));
        }

        private ClusterHealth Create_ClusterHealthMock(
            FabricHealth.HealthState state,
            IEnumerable<ApplicationHealthState> applicationHealthStates,
            IEnumerable<NodeHealthState> nodeHealthStates,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var clusterHealthMock = new Mock<ClusterHealth>(state) { CallBase = true };
            clusterHealthMock.Setup(mock => mock.ApplicationHealthStates).Returns(applicationHealthStates);
            clusterHealthMock.Setup(mock => mock.NodeHealthStates).Returns(nodeHealthStates);
            clusterHealthMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            clusterHealthMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return clusterHealthMock.Object;
        }

        private ApplicationHealth Create_ApplicationHealthMock(
            Uri appUri,
            FabricHealth.HealthState state,
            IEnumerable<ServiceHealthState> serviceHealthStates,
            IEnumerable<DeployedApplicationHealthState> deployedAppHealthStates,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var appMock = new Mock<ApplicationHealth>(appUri, state) { CallBase = true };
            appMock.Setup(mock => mock.ServiceHealthStates).Returns(serviceHealthStates);
            appMock.Setup(mock => mock.DeployedApplicationHealthStates).Returns(deployedAppHealthStates);
            appMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            appMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return appMock.Object;
        }

        private NodeHealth Create_NodeHealthMock(
            string nodeName, 
            FabricHealth.HealthState state,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var nodeMock = new Mock<NodeHealth>(nodeName, state) { CallBase = true };
            nodeMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            nodeMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return nodeMock.Object;
        }

        private ServiceHealth Create_ServiceHealthMock(
            Uri serviceUri,
            FabricHealth.HealthState state,
            IEnumerable<PartitionHealthState> partitionHealthStates,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var serviceMock = new Mock<ServiceHealth>(serviceUri, state) { CallBase = true };
            serviceMock.Setup(mock => mock.PartitionHealthStates).Returns(partitionHealthStates);
            serviceMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            serviceMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return serviceMock.Object;
        }

        private PartitionHealth Create_PartitionHealthMock(
            Guid partitionId,
            FabricHealth.HealthState state,
            IEnumerable<ReplicaHealthState> replicaHealthStates,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var partitionMock = new Mock<PartitionHealth>(partitionId, state) { CallBase = true };
            partitionMock.Setup(mock => mock.ReplicaHealthStates).Returns(replicaHealthStates);
            partitionMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            partitionMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return partitionMock.Object;
        }

        private ReplicaHealth Create_ReplicaHealthMock(
            Guid partitionId,
            long replicaId,
            FabricHealth.HealthState state,
            IEnumerable<EntityHealthEvent> healthEvents,
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            var replicaMock = new Mock<ReplicaHealth>(partitionId, replicaId, state) { CallBase = true };
            replicaMock.Setup(mock => mock.HealthEvents).Returns(healthEvents);
            replicaMock.Setup(mock => mock.UnhealthyEvaluations).Returns(unhealthyEvaluations);
            return replicaMock.Object;
        }
    }
}