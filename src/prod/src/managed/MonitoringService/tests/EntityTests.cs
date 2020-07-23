// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using FabricMonSvc;
    using Filters;
    using Internal.EventSource;
    using MdsHealthDataConsumer;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using FabricHealth = System.Fabric.Health;

    [TestClass]
    public class EntityTests
    {
        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetClusterHealthAsync()
        {
            this.ProcessAsync<FabricHealth.ClusterHealth>(
                (healthClient, consumer, trace, config, filters) 
                => new ClusterEntity(healthClient, consumer, trace, config, filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetApplicationHealthAsync()
        {
            this.ProcessAsync<FabricHealth.ApplicationHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new ApplicationEntity(
                    healthClient, 
                    consumer, 
                    "TestClusterName", 
                    new Uri(@"fabric:/MockApp"), 
                    trace, 
                    config, 
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetServiceHealthAsync()
        {
            this.ProcessAsync<FabricHealth.ServiceHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new ServiceEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    "TestApp",
                    new Uri(@"fabric:/TestApp/TestSvc"),
                    trace,
                    config,
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetPartitionHealthAsync()
        {
            this.ProcessAsync<FabricHealth.PartitionHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new PartitionEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    "TestApp",
                    "TestSvc",
                    Guid.Empty,
                    trace,
                    config,
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetReplicaHealthAsync()
        {
            this.ProcessAsync<FabricHealth.ReplicaHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new ReplicaEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    "TestApp",
                    "TestSvc",
                    Guid.Empty,
                    1,
                    trace,
                    config,
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetNodeHealthAsync()
        {
            this.ProcessAsync<FabricHealth.NodeHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new NodeEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    "TestNode",
                    trace,
                    config,
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetDeployedAppHealthAsync()
        {
            this.ProcessAsync<FabricHealth.DeployedApplicationHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new DeployedApplicationEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    new Uri(@"fabric:/MockApp"),
                    "TestNode",
                    trace,
                    config,
                    filters));
        }

        [TestMethod]
        public void ProcessAsync_Returns_EmptySequence_OnException_In_GetDeployedServiceHealthAsync()
        {
            this.ProcessAsync<FabricHealth.DeployedServicePackageHealth>(
                (healthClient, consumer, trace, config, filters) =>
                new DeployedServicePackageEntity(
                    healthClient,
                    consumer,
                    "TestClusterName",
                    new Uri(@"fabric:/MockApp"),
                    "TestServiceManifest",
                    "TestNode",
                    trace,
                    config,
                    filters));
        }

        private void ProcessAsync<TEntityHealth>(
            Func<FabricHealthClientWrapper, HealthDataConsumer, 
                TraceWriterWrapper, ServiceConfigurationData, 
                EntityFilterRepository, IEntity> getEntity) 
            where TEntityHealth : FabricHealth.EntityHealth
        {
            var tokenSource = new CancellationTokenSource();
            var cancellationToken = tokenSource.Token;
            var traceWriterMock = TraceWriterMock.Create_Verifible();

            var configData = new ServiceConfigurationData();

            var fabricClientMock = new Mock<FabricHealthClientWrapper>(
                traceWriterMock.Object,
                "FabricMonitoringServiceType",
                Guid.Empty,
                1) { CallBase = true };

            fabricClientMock
                .Setup(mock => mock.DoGetHealthAsync<TEntityHealth>(
                    It.IsAny<Func<Task<TEntityHealth>>>()))
                .Throws<TimeoutException>();

            var eventWriterMock = new Mock<MonitoringEventWriter>(MockBehavior.Strict);

            var metricsEmitterMock = new Mock<MetricsEmitter>(MockBehavior.Strict, configData, traceWriterMock.Object);

            var filterRepo = new EntityFilterRepository(configData);
            var consumer = new IfxHealthDataConsumer(eventWriterMock.Object, metricsEmitterMock.Object);
            var producer = new HealthDataProducer(fabricClientMock.Object, consumer, traceWriterMock.Object, configData, filterRepo);

            IEntity entity = getEntity(fabricClientMock.Object, consumer, traceWriterMock.Object, configData, filterRepo);
            var result = entity.ProcessAsync(cancellationToken).GetAwaiter().GetResult();

            Assert.IsNotNull(result);
            Assert.AreEqual(0, result.Count());
        }
    }
}