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
    using Filters;
    using Health;
    using Internal.EventSource;
    using MdsHealthDataConsumer;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using FabricHealth = System.Fabric.Health;

    [TestClass]
    public class HealthDataServiceTests
    {
        [TestMethod]
        public void HealthDataService_InHealthyCluster_LogsHealthToEventSource_AndEmitsMetrics()
        {
            var tokenSource = new CancellationTokenSource();
            var cancellationToken = tokenSource.Token;
            var traceWriterMock = TraceWriterMock.Create_TraceWriterMock_ThrowsOnWarningOrError();

            var clusterName = "unit-test-cluster";
            var configData = new ServiceConfigurationData();

            var clusterManagerServicePartitionId = new Guid("00000000-0000-0000-0000-000000000001");
            var infraServicePartitionId = new Guid("00000000-0000-0000-0000-000000000002");
            var infraServicePartitionId2 = new Guid("00000000-0000-0000-0000-000000000003");
            var monsvcPartitionId = new Guid("00000000-0000-0000-0000-000000000004");
            var mdsAgentPartitionId = new Guid("00000000-0000-0000-0000-000000000005");

            // setup the cluster health data 
            var clusterNode = new TreeNode(new ClusterHealth(FabricHealth.HealthState.Ok))
            {
                new TreeNode(new ApplicationHealth(new Uri("fabric:/System"), FabricHealth.HealthState.Ok))
                {
                    new TreeNode(new ServiceHealth(
                        new Uri("fabric:/System/ClusterManagerService"), 
                        FabricHealth.HealthState.Ok,
                        this.GetMockServiceHealthEvents("CMSvc"),
                        new List<FabricHealth.HealthEvaluation>()))
                    {
                        new TreeNode(new PartitionHealth(clusterManagerServicePartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(clusterManagerServicePartitionId, 1, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(clusterManagerServicePartitionId, 2, FabricHealth.HealthState.Ok))
                        }
                    },
                    new TreeNode(new ServiceHealth(
                        new Uri("fabric:/System/InfrastructureService"), 
                        FabricHealth.HealthState.Ok,
                        this.GetMockServiceHealthEvents("InfraSvc"), 
                        new List<FabricHealth.HealthEvaluation>()))
                    {
                        new TreeNode(new PartitionHealth(infraServicePartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(infraServicePartitionId, 1, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(infraServicePartitionId, 2, FabricHealth.HealthState.Ok))
                        },
                        new TreeNode(new PartitionHealth(infraServicePartitionId2, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(infraServicePartitionId2, 3, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(infraServicePartitionId2, 4, FabricHealth.HealthState.Ok))
                        }
                    },
                },
                new TreeNode(new ApplicationHealth(new Uri("fabric:/Monitoring"), FabricHealth.HealthState.Ok))
                {
                    new TreeNode(new ServiceHealth(new Uri("fabric:/Monitoring/MonitoringService"), FabricHealth.HealthState.Ok))
                    {
                        new TreeNode(new PartitionHealth(monsvcPartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(monsvcPartitionId, 5, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(monsvcPartitionId, 8, FabricHealth.HealthState.Ok)),
                        }
                    },
                    new TreeNode(new ServiceHealth(new Uri("fabric:/Monitoring/MonitoringAgentService"), FabricHealth.HealthState.Ok))
                    {
                        new TreeNode(new PartitionHealth(mdsAgentPartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(mdsAgentPartitionId, 1, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(mdsAgentPartitionId, 9, FabricHealth.HealthState.Ok)),
                        }
                    },
                },
                new TreeNode(new NodeHealth("_Node_0", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_1", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_2", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_3", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_4", FabricHealth.HealthState.Ok)),
            };

            var fabricClientMock = new FabricClientMock(cancellationToken, traceWriterMock);
            fabricClientMock.Setup(clusterName, clusterNode);

            var eventWriterMock = new EventWriterMock();
            eventWriterMock.Setup_Assertions(clusterName, clusterNode);

            var metricsEmitterMock = new MetricsEmitterMock(traceWriterMock, configData);
            metricsEmitterMock.Setup(clusterName, clusterNode);

            var filterRepo = new EntityFilterRepository(configData);
            var consumer = new IfxHealthDataConsumer(eventWriterMock.Mock.Object, metricsEmitterMock.Mock.Object);
            var producer = new HealthDataProducer(fabricClientMock.Mock.Object, consumer, traceWriterMock.Object, configData, filterRepo);

            Mock<HealthDataService> healthServiceMock = new Mock<HealthDataService>(producer, traceWriterMock.Object, configData, filterRepo) { CallBase = true };
            healthServiceMock.Cancel_AfterFirstReadPass(tokenSource, cancellationToken);

            healthServiceMock.Object.RunAsync(cancellationToken).Wait(timeout: TimeSpan.FromMilliseconds(300));

            eventWriterMock.Verify_CallCount();
            metricsEmitterMock.Verify_CallCount();
        }

        [TestMethod]
        public void HealthDataService_InUnhealthyCluster_LogsHealthToEventSource_AndEmitsMetrics()
        {
            var tokenSource = new CancellationTokenSource();
            var cancellationToken = tokenSource.Token;
            var traceWriterMock = TraceWriterMock.Create_TraceWriterMock_ThrowsOnWarningOrError();

            var clusterName = "unit-test-cluster";
            var configData = new ServiceConfigurationData();

            var clusterManagerServicePartitionId = new Guid("00000000-0000-0000-0000-000000000001");
            var infraServicePartitionId = new Guid("00000000-0000-0000-0000-000000000002");
            var infraServicePartitionId2 = new Guid("00000000-0000-0000-0000-000000000003");
            var monsvcPartitionId = new Guid("00000000-0000-0000-0000-000000000004");
            var mdsAgentPartitionId = new Guid("00000000-0000-0000-0000-000000000005");

            // setup the cluster health data 
            var clusterNode = new TreeNode(new ClusterHealth(FabricHealth.HealthState.Warning))
            {
                new TreeNode(new ApplicationHealth(new Uri("fabric:/System"), FabricHealth.HealthState.Warning))
                {
                    new TreeNode(new ServiceHealth(
                        new Uri("fabric:/System/ClusterManagerService"), 
                        FabricHealth.HealthState.Ok,
                        this.GetMockServiceHealthEvents("CMSvc"),
                        new List<FabricHealth.HealthEvaluation>()))
                    {
                        new TreeNode(new PartitionHealth(clusterManagerServicePartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(clusterManagerServicePartitionId, 1, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(clusterManagerServicePartitionId, 2, FabricHealth.HealthState.Ok))
                        }
                    },
                    new TreeNode(
                        new ServiceHealth(
                            new Uri("fabric:/System/InfrastructureService"),
                            FabricHealth.HealthState.Warning,
                            this.GetMockServiceHealthEvents("InfraSvc"),
                            new List<FabricHealth.HealthEvaluation>()))
                    {
                        new TreeNode(new PartitionHealth(infraServicePartitionId, FabricHealth.HealthState.Error))
                        {
                            new TreeNode(new ReplicaHealth(
                                infraServicePartitionId,
                                1,
                                FabricHealth.HealthState.Error,
                                new EntityHealthEvent[] { this.GetMockHealthEvent(11, FabricHealth.HealthState.Error, "InfraSvcReplica") },
                                new List<FabricHealth.HealthEvaluation>())),
                            new TreeNode(new ReplicaHealth(infraServicePartitionId, 2, FabricHealth.HealthState.Error))
                        },
                        new TreeNode(new PartitionHealth(infraServicePartitionId2, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(infraServicePartitionId2, 3, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(infraServicePartitionId2, 4, FabricHealth.HealthState.Ok))
                        }
                    },
                },
                new TreeNode(new ApplicationHealth(new Uri("fabric:/Monitoring"), FabricHealth.HealthState.Ok))
                {
                    new TreeNode(new ServiceHealth(new Uri("fabric:/Monitoring/MonitoringService"), FabricHealth.HealthState.Ok))
                    {
                        new TreeNode(new PartitionHealth(monsvcPartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(monsvcPartitionId, 5, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(monsvcPartitionId, 8, FabricHealth.HealthState.Ok)),
                        }
                    },
                    new TreeNode(new ServiceHealth(new Uri("fabric:/Monitoring/MonitoringAgentService"), FabricHealth.HealthState.Ok))
                    {
                        new TreeNode(new PartitionHealth(mdsAgentPartitionId, FabricHealth.HealthState.Ok))
                        {
                            new TreeNode(new ReplicaHealth(mdsAgentPartitionId, 1, FabricHealth.HealthState.Ok)),
                            new TreeNode(new ReplicaHealth(mdsAgentPartitionId, 9, FabricHealth.HealthState.Ok)),
                        }
                    },
                },
                new TreeNode(new NodeHealth("_Node_0", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth(
                    "_Node_1",
                    FabricHealth.HealthState.Error,
                    this.GetNodeHealthEvents("Node1"),
                    new List<FabricHealth.HealthEvaluation>())),
                new TreeNode(new NodeHealth("_Node_2", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_3", FabricHealth.HealthState.Ok)),
                new TreeNode(new NodeHealth("_Node_4", FabricHealth.HealthState.Ok)),
            };

            var fabricClientMock = new FabricClientMock(cancellationToken, traceWriterMock);
            fabricClientMock.Setup(clusterName, clusterNode);

            var eventWriterMock = new EventWriterMock();
            eventWriterMock.Setup_Assertions(clusterName, clusterNode);

            var metricsEmitterMock = new MetricsEmitterMock(traceWriterMock, configData);
            metricsEmitterMock.Setup(clusterName, clusterNode);

            var filterRepo = new EntityFilterRepository(configData);
            var consumer = new IfxHealthDataConsumer(eventWriterMock.Mock.Object, metricsEmitterMock.Mock.Object);
            var producer = new HealthDataProducer(fabricClientMock.Mock.Object, consumer, traceWriterMock.Object, configData, filterRepo);

            Mock<HealthDataService> healthServiceMock = new Mock<HealthDataService>(producer, traceWriterMock.Object, configData, filterRepo) { CallBase = true };
            healthServiceMock.Cancel_AfterFirstReadPass(tokenSource, cancellationToken);

            healthServiceMock.Object.RunAsync(cancellationToken).Wait(timeout: TimeSpan.FromMilliseconds(300));

            eventWriterMock.Verify_CallCount();
            metricsEmitterMock.Verify_CallCount();
        }

        [TestMethod]
        public void HealthDataService_HandlesExceptions_ThrownBy_GetHealthAsync()
        {
            var tokenSource = new CancellationTokenSource();
            var cancellationToken = tokenSource.Token;
            var traceWriterMock = TraceWriterMock.Create_Verifible();

            var configData = new ServiceConfigurationData();

            var fabricClientMock = new FabricClientMock(cancellationToken, traceWriterMock);
            fabricClientMock.Mock
                .Setup(mock => mock.DoGetHealthAsync<FabricHealth.ClusterHealth>(
                    It.IsAny<Func<Task<FabricHealth.ClusterHealth>>>()))
                .Throws<TimeoutException>();

            var eventWriterMock = new Mock<MonitoringEventWriter>(MockBehavior.Strict);

            var metricsEmitterMock = new Mock<MetricsEmitter>(MockBehavior.Strict, configData, traceWriterMock.Object);

            var filterRepo = new EntityFilterRepository(configData);
            var consumer = new IfxHealthDataConsumer(eventWriterMock.Object, metricsEmitterMock.Object);
            var producer = new HealthDataProducer(fabricClientMock.Mock.Object, consumer, traceWriterMock.Object, configData, filterRepo);

            Mock<HealthDataService> healthServiceMock = new Mock<HealthDataService>(producer, traceWriterMock.Object, configData, filterRepo) { CallBase = true };
            healthServiceMock.Cancel_AfterFirstReadPass(tokenSource, cancellationToken);

            healthServiceMock.Object.RunAsync(cancellationToken).Wait(timeout: TimeSpan.FromMilliseconds(300));
        }

        private IEnumerable<TreeNode> GetMockServiceHealthNodes(string applicationName, int count)
        {
            for (var index = 0; index < count; index++)
            {
                yield return this.GetMockServiceHealthNode(applicationName, index);
            }
        }

        private TreeNode GetMockServiceHealthNode(string applicationName, int index)
        {
            var partitionId1 = new Guid("00000000-0000-0000-0000-000000000004");
            var partitionId2 = new Guid("00000000-0000-0000-0000-000000000005");

            var serviceName = string.Format("{0}/Service{1}", applicationName, index);
            return new TreeNode(new ServiceHealth(new Uri(serviceName), FabricHealth.HealthState.Ok));
        }

        private EntityHealthEvent[] GetMockServiceHealthEvents(string prefix)
        {
            return new EntityHealthEvent[]
            {
                this.GetMockHealthEvent(10, FabricHealth.HealthState.Warning, prefix),
                this.GetMockHealthEvent(20, FabricHealth.HealthState.Ok, prefix)
            };
        }

        private EntityHealthEvent[] GetNodeHealthEvents(string prefix)
        {
            return new EntityHealthEvent[] 
            {
                this.GetMockHealthEvent(1, FabricHealth.HealthState.Error, prefix),
                this.GetMockHealthEvent(2, FabricHealth.HealthState.Warning, prefix),
                this.GetMockHealthEvent(3, FabricHealth.HealthState.Ok, prefix)
            };
        }

        private EntityHealthEvent GetMockHealthEvent(long sequenceNumber, FabricHealth.HealthState state, string sourceEnityPrefix)
        {
            return new EntityHealthEvent(
                state, 
                string.Format("Test.{0}.Description", sourceEnityPrefix), 
                string.Format("Test.{0}.Property", sourceEnityPrefix), 
                sequenceNumber, 
                string.Format("Test.{0}.Source", sourceEnityPrefix), 
                false);
        }
    }
}