// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Serialization
{
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.JsonSerializerWrapper;
    using System.Fabric.Query;
    using System.Reflection;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    internal class SerializerUnitTests
    {
        IJsonSerializer Serializer;
        Random random;

        public SerializerUnitTests()
        {
            this.Serializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();
            if (this.Serializer == null)
            {
                throw new DllNotFoundException("SerializerUnitTests..ctor(): Call to JsonSerializerImplDllLoader.TryGetJsonSerializerImpl() failed.");
            }

            this.random = new Random((int) DateTime.Now.Ticks);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void NodeIDSerializationTest()
        {
            NodeId nodeId = new NodeId(new Numerics.BigInteger(90.0), new Numerics.BigInteger(30.0));
            TestUsingSerializer(this.Serializer, nodeId);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceReplicaSerializationTest()
        {
            StatefulServiceReplica svcReplica = new StatefulServiceReplica(ServiceReplicaStatus.Ready, HealthState.Ok, ReplicaRole.ActiveSecondary, "fabric:/testsvc/testreplica", "nodeA", 890, TimeSpan.FromSeconds(5345));
            TestUsingSerializer(this.Serializer, svcReplica);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceSerializationTest()
        {
            StatefulService svc = new StatefulService(new Uri("fabric:/newservice"), "testSvcType", "testSvcVersion", false, HealthState.Ok, ServiceStatus.Active);
            TestUsingSerializer(this.Serializer, svc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceDescriptionSerializationTest()
        {
            StatelessServiceDescription svc = this.random.CreateRandom<StatelessServiceDescription>();
            TestUsingSerializer(this.Serializer, svc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ApplicationHealthPolicySerializationTest()
        {
            // Default
            ApplicationHealthPolicy healthPolicy = new ApplicationHealthPolicy();
            TestUsingSerializer(this.Serializer, healthPolicy);

            ApplicationHealthPolicy healthPolicy2 = this.random.CreateRandom<ApplicationHealthPolicy>();
            TestUsingSerializer(this.Serializer, healthPolicy2);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ClusterHealthPolicySerializationTest()
        {
            ClusterHealthPolicy healthPolicy = this.random.CreateRandom<ClusterHealthPolicy>();
            TestUsingSerializer(this.Serializer, healthPolicy);

            healthPolicy.ApplicationTypeHealthPolicyMap.Clear();
            TestUsingSerializer(this.Serializer, healthPolicy);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FabricUpgradeUpdateDescriptionSerializationTest()
        {
            var upgradeUpdate = this.random.CreateRandom<FabricUpgradeUpdateDescription>();
            upgradeUpdate.UpgradeMode = RollingUpgradeMode.Monitored;
            TestUsingSerializer(this.Serializer, upgradeUpdate);

            // Set some fields to null
            upgradeUpdate.UpgradeMode = null;
            upgradeUpdate.HealthCheckRetryTimeout = null;
            upgradeUpdate.HealthPolicy = null;
            upgradeUpdate.UpgradeReplicaSetCheckTimeout = null;
            TestUsingSerializer(this.Serializer, upgradeUpdate);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FabricApplicationUpdateDescriptionSerializationTest()
        {
            var upgradeUpdate = this.random.CreateRandom<ApplicationUpgradeUpdateDescription>();
            TestUsingSerializer(this.Serializer, upgradeUpdate);

            // Set some fields to null
            upgradeUpdate.UpgradeMode = null;
            upgradeUpdate.HealthCheckRetryTimeout = null;
            upgradeUpdate.HealthPolicy = null;
            upgradeUpdate.UpgradeReplicaSetCheckTimeout = null;
            TestUsingSerializer(this.Serializer, upgradeUpdate);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PagedNodeListSerializationTest()
        {
            NodeList nodeList = new NodeList();
            nodeList.ContinuationToken = "NodeId4238539259";
            nodeList.Add(this.random.CreateRandom<Node>());
            TestUsingSerializer(this.Serializer, nodeList);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PagedApplicationListSerializationTest()
        {
            ApplicationList applicationList = new ApplicationList();
            applicationList.ContinuationToken = "fabric:/MyApp";
            applicationList.Add(this.random.CreateRandom<Application>());
            TestUsingSerializer(this.Serializer, applicationList);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PagedServiceListSerializationTest()
        {
            ServiceList serviceList = new ServiceList();
            serviceList.ContinuationToken = "ServiceId4238539259";
            serviceList.Add(new StatefulService(
                this.random.CreateRandom<Uri>(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<bool>(),
                this.random.CreateRandom<HealthState>(),
                this.random.CreateRandom<ServiceStatus>()));
            serviceList.Add(new StatelessService(
                this.random.CreateRandom<Uri>(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<HealthState>(),
                this.random.CreateRandom<ServiceStatus>()));
            TestUsingSerializer(this.Serializer, serviceList);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PagedServicePartitionListSerializationTest()
        {
            ServicePartitionList partitionList = new ServicePartitionList();
            partitionList.ContinuationToken = Guid.NewGuid().ToString();

            ServicePartitionInformation partitionInfo = random.CreateRandom<ServicePartitionInformation>();
            partitionList.Add(new StatefulServicePartition(partitionInfo, 3, 1, HealthState.Warning, ServicePartitionStatus.Ready, TimeSpan.FromMinutes(49), new Epoch(10, 23)));
            partitionList.Add(new StatelessServicePartition(partitionInfo, 6, HealthState.Warning, ServicePartitionStatus.Ready));
            TestUsingSerializer(this.Serializer, partitionList);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PagedServiceReplicaListSerializationTest()
        {
            ServiceReplicaList replicaList = new ServiceReplicaList();
            replicaList.ContinuationToken = "432543";
            replicaList.Add(new StatefulServiceReplica(
                this.random.CreateRandom<ServiceReplicaStatus>(),
                this.random.CreateRandom<HealthState>(),
                ReplicaRole.IdleSecondary,
                this.random.CreateRandom<Uri>().ToString(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<long>(),
                TimeSpan.FromSeconds(2)));
            replicaList.Add(new StatelessServiceInstance(
                this.random.CreateRandom<ServiceReplicaStatus>(),
                this.random.CreateRandom<HealthState>(),
                this.random.CreateRandom<Uri>().ToString(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<long>(),
                TimeSpan.FromSeconds(4)));
            TestUsingSerializer(this.Serializer, replicaList);
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void HealthInformationSerializationTest()
        {
            HealthInformation healthInfo = this.random.CreateRandom<HealthInformation>();
            TestUsingSerializer(this.Serializer, healthInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void HealthEventSerializationTest()
        {
            HealthEvent healthEvent = this.random.CreateRandom<HealthEvent>();
            TestUsingSerializer(this.Serializer, healthEvent);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void  HealthEvaluationSerializationTest()
        {
            HealthEvaluation item1 = this.random.CreateRandom<EventHealthEvaluation>();
            RecreateObjectAndCompare(item1, this.Serializer, typeof(HealthEvaluation));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ClusterHealthChunkSerializationTest()
        {
            var replicaChunk = this.random.CreateRandom<ReplicaHealthStateChunk>();
            TestUsingSerializer(this.Serializer, replicaChunk);

            var partitionChunk = this.random.CreateRandom<PartitionHealthStateChunk>();
            partitionChunk.ReplicaHealthStateChunks = new ReplicaHealthStateChunkList() { replicaChunk };
            TestUsingSerializer(this.Serializer, partitionChunk);

            var serviceChunk = this.random.CreateRandom<ServiceHealthStateChunk>();
            serviceChunk.PartitionHealthStateChunks = new PartitionHealthStateChunkList() { partitionChunk };
            TestUsingSerializer(this.Serializer, serviceChunk);

            var dspChunk = this.random.CreateRandom<DeployedServicePackageHealthStateChunk>();
            TestUsingSerializer(this.Serializer, dspChunk);

            var daChunk = this.random.CreateRandom<DeployedApplicationHealthStateChunk>();
            daChunk.DeployedServicePackageHealthStateChunks = new DeployedServicePackageHealthStateChunkList() { dspChunk };
            TestUsingSerializer(this.Serializer, daChunk);

            var appChunk = this.random.CreateRandom<ApplicationHealthStateChunk>();
            appChunk.ServiceHealthStateChunks = new ServiceHealthStateChunkList() { serviceChunk };
            appChunk.DeployedApplicationHealthStateChunks = new DeployedApplicationHealthStateChunkList() { daChunk };
            TestUsingSerializer(this.Serializer, appChunk);

            var nodeChunk = this.random.CreateRandom<NodeHealthStateChunk>();
            TestUsingSerializer(this.Serializer, nodeChunk);

            var clusterChunk = this.random.CreateRandom<ClusterHealthChunk>();
            clusterChunk.ApplicationHealthStateChunks = new ApplicationHealthStateChunkList() { appChunk };
            clusterChunk.NodeHealthStateChunks = new NodeHealthStateChunkList() { nodeChunk };
            TestUsingSerializer(this.Serializer, clusterChunk);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ClusterHealthChunkQueryDescriptionSerializationTest()
        {
            var replicaFilter = this.random.CreateRandom<ReplicaHealthStateFilter>();

            var partitionFilter = this.random.CreateRandom<PartitionHealthStateFilter>();
            partitionFilter.ReplicaFilters.Add(replicaFilter);

            var serviceFilter = this.random.CreateRandom<ServiceHealthStateFilter>();
            serviceFilter.PartitionFilters.Add(partitionFilter);

            var dspFilter = this.random.CreateRandom<DeployedServicePackageHealthStateFilter>();

            var deployedAppFilter = this.random.CreateRandom<DeployedApplicationHealthStateFilter>();
            deployedAppFilter.DeployedServicePackageFilters.Add(dspFilter);
            
            var appFilter = this.random.CreateRandom<ApplicationHealthStateFilter>();
            appFilter.ServiceFilters.Add(serviceFilter);
            appFilter.DeployedApplicationFilters.Add(deployedAppFilter);
            
            var nodeFilter = this.random.CreateRandom<NodeHealthStateFilter>();

            var queryDescription = this.random.CreateRandom<ClusterHealthChunkQueryDescription>();
            queryDescription.ApplicationFilters.Add(appFilter);
            queryDescription.NodeFilters.Add(nodeFilter);

            TestUsingSerializer(this.Serializer, queryDescription);
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ReplicaHealthSerializationTest()
        {
            ReplicaHealth replicaHealth = this.random.CreateRandom<ReplicaHealth>();

            TestUsingSerializer(this.Serializer, replicaHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PartitionHealthSerializationTest()
        {
            PartitionHealth partitionHealth = this.random.CreateRandom<PartitionHealth>();

            var replicaHealthState = new StatefulServiceReplicaHealthState()
            {
                AggregatedHealthState = this.random.CreateRandom<HealthState>(),
                PartitionId = partitionHealth.PartitionId,
                Id = this.random.Next()
            };
            partitionHealth.ReplicaHealthStates = new List<ReplicaHealthState>();
            partitionHealth.ReplicaHealthStates.Add(replicaHealthState);

            TestUsingSerializer(this.Serializer, partitionHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceHealthSerializationTest()
        {
            ServiceHealth serviceHealth = this.random.CreateRandom<ServiceHealth>();

            var partitionHealthState = new PartitionHealthState() 
            {
                AggregatedHealthState = this.random.CreateRandom<HealthState>(),
                PartitionId = Guid.NewGuid()
            };

            serviceHealth.PartitionHealthStates = new List<PartitionHealthState>() { partitionHealthState };

            TestUsingSerializer(this.Serializer, serviceHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ApplicationHealthSerializationTest()
        {
            ApplicationHealth appHealth = this.random.CreateRandom<ApplicationHealth>();

            var deployedApplicationHealthState = new DeployedApplicationHealthState() 
            {
                ApplicationName = new Uri("fabric:/" + "testApp" + this.random.CreateRandom<string>()),
                NodeName = "NodeName_" + this.random.CreateRandom<string>(),
                AggregatedHealthState = this.random.CreateRandom<HealthState>()
            };
            appHealth.DeployedApplicationHealthStates = new List<DeployedApplicationHealthState>() { deployedApplicationHealthState };

            var serviceHealthState = new ServiceHealthState()
            {
                ServiceName = new Uri("fabric:/" + "testSvc" + this.random.CreateRandom<string>()),
                AggregatedHealthState = this.random.CreateRandom<HealthState>()
            };

            appHealth.ServiceHealthStates = new List<ServiceHealthState>() { serviceHealthState };

            TestUsingSerializer(this.Serializer, appHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void NodeHealthSerializationTest()
        {
            NodeHealth nodeHealth = this.random.CreateRandom<NodeHealth>();

            TestUsingSerializer(this.Serializer, nodeHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ClusterHealthSerializationTest()
        {
            ClusterHealth clusterHealth = this.random.CreateRandom<ClusterHealth>();

            var appHS = new ApplicationHealthState()
            {
                ApplicationName = new Uri("fabric:/" + "testAppManifest" + this.random.CreateRandom<string>()),
                AggregatedHealthState = this.random.CreateRandom<HealthState>()
            };

            var nodeHS = new NodeHealthState()
            {
                NodeName = "NodeName_" + this.random.CreateRandom<string>(),
                AggregatedHealthState = this.random.CreateRandom<HealthState>()                
            };

            clusterHealth.ApplicationHealthStates = new List<ApplicationHealthState>() { appHS };
            clusterHealth.NodeHealthStates = new List<NodeHealthState>() { nodeHS };
            TestUsingSerializer(this.Serializer, clusterHealth);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void QueryApplicationTypeSerializationTest()
        {
            ApplicationType appType = new ApplicationType("aasdf", "asdfV", ApplicationTypeStatus.Available, "mock details", null);

            TestUsingSerializer(this.Serializer, appType);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DeleteApplicationDescriptionSerializationTest()
        {
            Uri applicationName = new Uri("fabric:/" + "testApp" + this.random.CreateRandom<string>());
            DeleteApplicationDescription deleteApplicationDescription = new DeleteApplicationDescription(applicationName)
            {
                ForceDelete = this.random.CreateRandom<bool>()
            };

            TestUsingSerializer(this.Serializer, deleteApplicationDescription);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DeleteServiceDescriptionSerializationTest()
        {
            Uri serviceName = new Uri("fabric:/" + "testSvc" + this.random.CreateRandom<string>());
            DeleteServiceDescription deleteServiceDescription = new DeleteServiceDescription(serviceName)
            {
                ForceDelete = this.random.CreateRandom<bool>()
            };

            TestUsingSerializer(this.Serializer, deleteServiceDescription);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ChaosParametersSerializationTest()
        {
            TimeSpan maxClusterStabilizationTimeout = TimeSpan.FromSeconds(997);
            long maxConcurrentFaults = 7;
            TimeSpan waitTimeBetweenIterations = TimeSpan.FromSeconds(131);
            TimeSpan waitTimeBetweenFaults = TimeSpan.FromSeconds(19);
            TimeSpan timeToRun = TimeSpan.FromSeconds(104729);
            bool enableMoveReplicaFaults = true;

            var healthPolicy = new ClusterHealthPolicy
                                   {
                                       ConsiderWarningAsError = false,
                                       MaxPercentUnhealthyNodes = 10,
                                       MaxPercentUnhealthyApplications = 15
                                   };

            healthPolicy.ApplicationTypeHealthPolicyMap["TestApplicationTypePolicy"] = 11;

            var context = new Dictionary<string, string> { { "key1", "value1" }, { "key2", "value2" } };

            var chaosParameters = new ChaosParameters(
                maxClusterStabilizationTimeout,
                maxConcurrentFaults,
                enableMoveReplicaFaults,
                timeToRun,
                context,
                waitTimeBetweenIterations,
                waitTimeBetweenFaults,
                healthPolicy);

            var chaosTargetFilter = new ChaosTargetFilter();
            var nodeTypeInclustionList = new List<string> { "NodeType1", "NodeType2" };
            var applicationInclusionList = new List<string> { "fabric:/app1", "fabric:/app2" };
            chaosTargetFilter.ApplicationInclusionList = applicationInclusionList;
            chaosTargetFilter.NodeTypeInclusionList = nodeTypeInclustionList;

            this.TestUsingSerializer(this.Serializer, chaosParameters);
        }

        public void TestUsingSerializer(IJsonSerializer serializer, params object[] itemList)
        {
            foreach (object item in itemList)
            {
                RecreateObjectAndCompare(item, serializer);
            }
        }

        public void TestUsingSerializer(Type targetType, IJsonSerializer serializer, params string[] jsonStringList)
        {
            foreach (string jsonString in jsonStringList)
            {
                this.RecreateObjectAndCompare(targetType, serializer, jsonString);
            }
        }

        public void RecreateObjectAndCompare(Type targetType, IJsonSerializer serializer, string jsonString_1)
        {
            Assert.IsNotNull(jsonString_1, string.Format("Given json string is not null for target type:{0}", targetType.Name));
            var formattedJson = JContainer.Parse(jsonString_1).ToString();
            Assert.IsTrue(!string.IsNullOrWhiteSpace(jsonString_1), string.Format("Given json string is not empty: \n{0}", formattedJson));

            // Create item from json1
            var item_1 = serializer.Deserialize(jsonString_1, targetType);

            // Serialize and create json2
            string jsonString_1_2 = serializer.Serialize(item_1);
            Assert.IsTrue(!string.IsNullOrWhiteSpace(jsonString_1_2), string.Format("json string 2 is not empty: \n{0}", jsonString_1_2));

            // Create item2 from json2
            object item_1_2 = serializer.Deserialize(jsonString_1_2, targetType);

            string jsonString_1_2_3 = serializer.Serialize(item_1_2);

            // Assert two items are equal
            bool areEqItem = SerializationUtil.ObjectMatches(item_1, item_1_2);
            Assert.IsTrue(areEqItem, string.Format("Comparing two deserialized items of type: {0}", targetType.Name));

            // Assert two created jsonStrings are equal
            Assert.AreEqual<string>(jsonString_1_2, jsonString_1_2_3, string.Format("Comparing two jsonStrings for target type:{0}", targetType.Name));
        }

        public void RecreateObjectAndCompare(object item, IJsonSerializer serializer, Type targetType = null)
        {
            targetType = targetType ?? item.GetType();
            Assert.IsNotNull(item, string.Format("Given item not null for target type:{0}", targetType.Name));

            string jsonString_1 = serializer.Serialize(item);
            Assert.IsTrue(!string.IsNullOrWhiteSpace(jsonString_1), string.Format("Given json string is not empty: \n{0}", jsonString_1));

            // Create item from json1
            var item_1 = serializer.Deserialize(jsonString_1, targetType);

            // Serialize and create json2
            string jsonString_1_2 = serializer.Serialize(item_1);
            Assert.IsTrue(!string.IsNullOrWhiteSpace(jsonString_1_2), string.Format("json string 2 is not empty: \n{0}", jsonString_1_2));

            // Assert two items are equal
            bool areEqItem = SerializationUtil.ObjectMatches(item, item_1);
            Assert.IsTrue(areEqItem, string.Format("Comparing two deserialized items of type: {0}", targetType.Name));

            // Assert two created jsonStrings are equal
            Assert.AreEqual<string>(jsonString_1, jsonString_1_2, string.Format("Comparing two jsonStrings for target type:{0}", targetType.Name));
        }
    }
}