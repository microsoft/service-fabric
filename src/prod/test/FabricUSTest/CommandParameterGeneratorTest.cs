// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FabricUS.Test
{
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.UpgradeService;
    using System.Fabric.WRP.Model;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    [TestClass]
    class CommandParameterGeneratorTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefaultHealthPolicy()
        {
            CommandParameterGenerator generator = new CommandParameterGenerator(new TestFabricClientWrapper());

            PaasClusterUpgradePolicy defaultPaasClusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicy();
            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(5, 5);

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                defaultPaasClusterUpgradePolicy, 
                defaultClusterHealth,
                CancellationToken.None).Result;

            VerifyCommandProcessorClusterUpgradeDescription(
                defaultPaasClusterUpgradePolicy,
                result, 
                maxPercentageUnhealthyApplication: 0,
                expectedtMaxPercentUnhealthyPerServicesType: null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplcationDeltaHealthPolicy()
        {
            CommandParameterGenerator generator = new CommandParameterGenerator(new TestFabricClientWrapper());

            PaasClusterUpgradePolicy defaultPaasClusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicy();

            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(
                totalApplicationCount: 6, 
                totalNodeCount: 5, 
                unhealthyApplicationsCount: 3, 
                unhealthyNodeCount: 0);

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                defaultPaasClusterUpgradePolicy,
                defaultClusterHealth,
                CancellationToken.None).Result;

            VerifyCommandProcessorClusterUpgradeDescription(
                defaultPaasClusterUpgradePolicy, 
                result, 
                maxPercentageUnhealthyApplication: 50,
                expectedtMaxPercentUnhealthyPerServicesType: null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplcationWithNullDeltaHealthPolicy()
        {
            CommandParameterGenerator generator = new CommandParameterGenerator(new TestFabricClientWrapper());

            PaasClusterUpgradePolicy clusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicy();
            clusterUpgradePolicy.DeltaHealthPolicy = null;

            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(
                totalApplicationCount: 6,
                totalNodeCount: 5,
                unhealthyApplicationsCount: 0,
                unhealthyNodeCount: 0);

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                clusterUpgradePolicy,
                defaultClusterHealth,
                CancellationToken.None).Result;

            VerifyCommandProcessorClusterUpgradeDescription(
                clusterUpgradePolicy, 
                result,
                maxPercentageUnhealthyApplication: 100,
                expectedtMaxPercentUnhealthyPerServicesType: null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplcationWithHealthySystemServiceHealthPolicy()
        {
            var testFabricClientWrapper = new TestFabricClientWrapper();
            CommandParameterGenerator generator = new CommandParameterGenerator(testFabricClientWrapper);

            PaasClusterUpgradePolicy clusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicyWithSystemServicesPolicies();

            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(
                totalApplicationCount: 6,
                totalNodeCount: 5,
                unhealthyApplicationsCount: 0,
                unhealthyNodeCount: 0);

            Service fssService = new StatefulService(new Uri("fabric:/System/ImageStoreService"), "FileStoreService", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            Service cmService = new StatefulService(new Uri("fabric:/System/ClusterManagerService"), "ClusterManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            Service hmService = new StatefulService(new Uri("fabric:/System/HealthManagerService"), "HealthManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            testFabricClientWrapper.GetServicesResult = new ServiceList(new List<Service>() { fssService, cmService, hmService });

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                clusterUpgradePolicy,
                defaultClusterHealth,
                CancellationToken.None).Result;

            var expectedtMaxPercentUnhealthyPerServicesType = new Dictionary<string, byte>();
            expectedtMaxPercentUnhealthyPerServicesType.Add("FileStoreService", 0);
            expectedtMaxPercentUnhealthyPerServicesType.Add("ClusterManager", 0);
            expectedtMaxPercentUnhealthyPerServicesType.Add("HealthManager", 0);

            VerifyCommandProcessorClusterUpgradeDescription(
                clusterUpgradePolicy, 
                result, 
                0,
                expectedtMaxPercentUnhealthyPerServicesType);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSystemServicePolicyWithNoApp()
        {
            var testFabricClientWrapper = new TestFabricClientWrapper();
            CommandParameterGenerator generator = new CommandParameterGenerator(testFabricClientWrapper);

            PaasClusterUpgradePolicy clusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicyWithSystemServicesPolicies();

            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(
                totalApplicationCount: 0,
                totalNodeCount: 5,
                unhealthyApplicationsCount: 0,
                unhealthyNodeCount: 0);

            Service fssService = new StatefulService(new Uri("fabric:/System/ImageStoreService"), "FileStoreService", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            Service cmService = new StatefulService(new Uri("fabric:/System/ClusterManagerService"), "ClusterManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            Service hmService = new StatefulService(new Uri("fabric:/System/HealthManagerService"), "HealthManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            testFabricClientWrapper.GetServicesResult = new ServiceList(new List<Service>() { fssService, cmService, hmService });

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                clusterUpgradePolicy,
                defaultClusterHealth,
                CancellationToken.None).Result;

            var expectedtMaxPercentUnhealthyPerServicesType = new Dictionary<string, byte>();
            expectedtMaxPercentUnhealthyPerServicesType.Add("FileStoreService", 0);
            expectedtMaxPercentUnhealthyPerServicesType.Add("ClusterManager", 0);
            expectedtMaxPercentUnhealthyPerServicesType.Add("HealthManager", 0);

            VerifyCommandProcessorClusterUpgradeDescription(
                clusterUpgradePolicy,
                result,
                0,
                expectedtMaxPercentUnhealthyPerServicesType);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplcationWithUnhealthySystemServiceHealthPolicy()
        {
            var testFabricClientWrapper = new TestFabricClientWrapper();
            CommandParameterGenerator generator = new CommandParameterGenerator(testFabricClientWrapper);

            PaasClusterUpgradePolicy clusterUpgradePolicy = CreateDefaultPaasClusterUpgradePolicyWithSystemServicesPolicies();

            ClusterHealth defaultClusterHealth = CreateDefaultClusterHealth(
                totalApplicationCount: 6,
                totalNodeCount: 5,
                unhealthyApplicationsCount: 0,
                unhealthyNodeCount: 0,
                isSystemServicesUnhealthy: true);

            Service fssService = new StatefulService(new Uri("fabric:/System/ImageStoreService"), "FileStoreService", "1.0", true, HealthState.Error, ServiceStatus.Active);
            Service cmService = new StatefulService(new Uri("fabric:/System/ClusterManagerService"), "ClusterManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            Service hmService = new StatefulService(new Uri("fabric:/System/HealthManagerService"), "HealthManager", "1.0", true, HealthState.Ok, ServiceStatus.Active);
            testFabricClientWrapper.GetServicesResult = new ServiceList(new List<Service>() { fssService, cmService, hmService });

            CommandProcessorClusterUpgradeDescription result = generator.GetClusterUpgradeDescriptionAsync(
                clusterUpgradePolicy,
                defaultClusterHealth,
                CancellationToken.None).Result;

            var expectedtMaxPercentUnhealthyPerServicesType = new Dictionary<string, byte>();
            expectedtMaxPercentUnhealthyPerServicesType.Add("FileStoreService", 100);
            expectedtMaxPercentUnhealthyPerServicesType.Add("ClusterManager", 0);
            expectedtMaxPercentUnhealthyPerServicesType.Add("HealthManager", 0);

            VerifyCommandProcessorClusterUpgradeDescription(
                clusterUpgradePolicy,
                result,
                0,
                expectedtMaxPercentUnhealthyPerServicesType);
        }

        private void VerifyCommandProcessorClusterUpgradeDescription(
            PaasClusterUpgradePolicy source, 
            CommandProcessorClusterUpgradeDescription result,
            byte maxPercentageUnhealthyApplication,
            Dictionary<string, byte> expectedtMaxPercentUnhealthyPerServicesType )
        {
            Assert.AreEqual(source.ForceRestart, result.ForceRestart);
            Assert.AreEqual(source.HealthCheckRetryTimeout, result.HealthCheckRetryTimeout);
            Assert.AreEqual(source.HealthCheckStableDuration, result.HealthCheckStableDuration);
            Assert.AreEqual(source.HealthCheckWaitDuration, result.HealthCheckWaitDuration);
            Assert.AreEqual(source.UpgradeDomainTimeout, result.UpgradeDomainTimeout);
            Assert.AreEqual(source.UpgradeTimeout, result.UpgradeTimeout);
            Assert.AreEqual(source.UpgradeReplicaSetCheckTimeout, result.UpgradeReplicaSetCheckTimeout);

            if (source.HealthPolicy == null)
            {
                Assert.IsNull(result.HealthPolicy);
            }
            else
            {
                Assert.IsNotNull(result.HealthPolicy);

                Assert.AreEqual(source.HealthPolicy.MaxPercentUnhealthyNodes, result.HealthPolicy.MaxPercentUnhealthyNodes);

                Assert.AreEqual(maxPercentageUnhealthyApplication, result.HealthPolicy.MaxPercentUnhealthyApplications);

                if (source.HealthPolicy.ApplicationHealthPolicies == null)
                {
                    Assert.IsNull(result.HealthPolicy.ApplicationHealthPolicies);
                }
                else
                {
                    Assert.IsNotNull(result.HealthPolicy.ApplicationHealthPolicies);

                    Assert.AreEqual(source.HealthPolicy.ApplicationHealthPolicies.Count(), result.HealthPolicy.ApplicationHealthPolicies.Count());

                    foreach (var keyVaulePair in result.HealthPolicy.ApplicationHealthPolicies)
                    {
                        Assert.AreEqual(
                            source.HealthPolicy.ApplicationHealthPolicies[keyVaulePair.Key].DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices,
                            keyVaulePair.Value.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices);

                        if (keyVaulePair.Value.SerivceTypeHealthPolicies != null)
                        {
                            foreach (var serviceTypeHealthPolicyKeyValue in keyVaulePair.Value.SerivceTypeHealthPolicies)
                            {
                                string serviceTypeName = serviceTypeHealthPolicyKeyValue.Key;
                                byte expectedtMaxPercentUnhealthy;
                                if (expectedtMaxPercentUnhealthyPerServicesType.TryGetValue(
                                        serviceTypeName, out expectedtMaxPercentUnhealthy))
                                {
                                    Assert.AreEqual(expectedtMaxPercentUnhealthy, serviceTypeHealthPolicyKeyValue.Value.MaxPercentUnhealthyServices);
                                }
                                else
                                {
                                    Assert.AreEqual(
                                        source.HealthPolicy.ApplicationHealthPolicies[keyVaulePair.Key].SerivceTypeHealthPolicies[serviceTypeName].MaxPercentUnhealthyServices,
                                        serviceTypeHealthPolicyKeyValue.Value);
                                }
                            }
                        }
                    }
                }
            }

            if (source.DeltaHealthPolicy == null)
            {
                Assert.IsNull(result.DeltaHealthPolicy);
            }
            else
            {
                Assert.IsNotNull(result.DeltaHealthPolicy);

                Assert.AreEqual(source.DeltaHealthPolicy.MaxPercentDeltaUnhealthyNodes,
                    result.DeltaHealthPolicy.MaxPercentDeltaUnhealthyNodes);

                Assert.AreEqual(source.DeltaHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes,
                    result.DeltaHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes);
            }            
        }

        private static PaasClusterUpgradePolicy CreateDefaultPaasClusterUpgradePolicy()
        {
            PaasClusterUpgradePolicy policy = new PaasClusterUpgradePolicy()
            {
                ForceRestart = false,
                UpgradeReplicaSetCheckTimeout = TimeSpan.MaxValue,
                HealthCheckWaitDuration = TimeSpan.FromMinutes(5),
                HealthCheckStableDuration = TimeSpan.FromMinutes(5),
                HealthCheckRetryTimeout = TimeSpan.FromMinutes(30),
                UpgradeTimeout = TimeSpan.FromHours(12),
                UpgradeDomainTimeout = TimeSpan.FromHours(2)
            };

            policy.HealthPolicy = new PaasClusterUpgradeHealthPolicy()
            {
                MaxPercentUnhealthyApplications = 100,
                MaxPercentUnhealthyNodes = 100,
                ApplicationHealthPolicies = null
            };

            policy.DeltaHealthPolicy = new PaasClusterUpgradeDeltaHealthPolicy()
            {
                MaxPercentDeltaUnhealthyApplications = 0,
                MaxPercentDeltaUnhealthyNodes = 0,
                MaxPercentUpgradeDomainDeltaUnhealthyNodes = 0,
                ApplicationDeltaHealthPolicies = null
            };

            return policy;
        }

        private static PaasClusterUpgradePolicy CreateDefaultPaasClusterUpgradePolicyWithSystemServicesPolicies()
        {
            PaasApplicationHealthPolicy systemServiceHealthPolicy = new PaasApplicationHealthPolicy();
            systemServiceHealthPolicy.DefaultServiceTypeHealthPolicy = new PaasServiceTypeHealthPolicy()
            {
                MaxPercentUnhealthyServices = 100
            };

            PaasApplicationDeltaHealthPolicy systemServiceDeltaHealthPolicy = new PaasApplicationDeltaHealthPolicy();
            systemServiceDeltaHealthPolicy.DefaultServiceTypeDeltaHealthPolicy = new PaasServiceTypeDeltaHealthPolicy()
            {
                MaxPercentDeltaUnhealthyServices = 0
            };

            PaasClusterUpgradePolicy policy = CreateDefaultPaasClusterUpgradePolicy();
            policy.HealthPolicy.ApplicationHealthPolicies = new Dictionary<string, PaasApplicationHealthPolicy>();
            policy.HealthPolicy.ApplicationHealthPolicies.Add("fabric:/System", systemServiceHealthPolicy);

            policy.DeltaHealthPolicy.ApplicationDeltaHealthPolicies = new Dictionary<string, PaasApplicationDeltaHealthPolicy>();
            policy.DeltaHealthPolicy.ApplicationDeltaHealthPolicies.Add("fabric:/System", systemServiceDeltaHealthPolicy);

            return policy;
        }

        private static ClusterHealth CreateDefaultClusterHealth(
            int totalApplicationCount, 
            int totalNodeCount, 
            int unhealthyApplicationsCount = 0, 
            int unhealthyNodeCount = 0,
            bool isSystemServicesUnhealthy = false)
        {
            HealthState aggregatedHealthState =
                (unhealthyApplicationsCount > 0 || unhealthyNodeCount > 0 || isSystemServicesUnhealthy) ? HealthState.Error : HealthState.Ok;

            List<ApplicationHealthState> applicationHealthStates = new List<ApplicationHealthState>();
            for (int i = 0; i < totalApplicationCount; i++)
            {
                HealthState healthState = HealthState.Ok;
                if (unhealthyApplicationsCount > 0)
                {
                    healthState = HealthState.Error;
                    unhealthyApplicationsCount--;
                }

                ApplicationHealthState appHealthState = new ApplicationHealthState()
                {
                    AggregatedHealthState = healthState,
                    ApplicationName = new Uri(string.Format("fabric:/app{0}", i))
                };

                applicationHealthStates.Add(appHealthState);
            }

            ApplicationHealthState systemServiceAppHealthState = new ApplicationHealthState()
            {
                AggregatedHealthState = isSystemServicesUnhealthy ? HealthState.Error : HealthState.Ok,
                ApplicationName = new Uri("fabric:/System")
            };

            applicationHealthStates.Add(systemServiceAppHealthState);

            List<NodeHealthState> nodeHealthStates = new List<NodeHealthState>();
            for (int i = 0; i < totalNodeCount; i++)
            {
                HealthState healthState = HealthState.Ok;
                if (unhealthyNodeCount > 0)
                {
                    healthState = HealthState.Error;
                    unhealthyNodeCount--;
                }

                NodeHealthState nodeHealthState = new NodeHealthState()
                {
                    AggregatedHealthState = healthState,
                    NodeName = "_node_" + i.ToString()
                };

                nodeHealthStates.Add(nodeHealthState);
            }

            return new ClusterHealth()
            {
                AggregatedHealthState = aggregatedHealthState,
                ApplicationHealthStates = applicationHealthStates,
                NodeHealthStates = nodeHealthStates,
                HealthEvents = new List<HealthEvent>(),
                UnhealthyEvaluations = new List<HealthEvaluation>()
            };            
        }
    }
}