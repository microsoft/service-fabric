// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Scenarios.Test.Hosting;
    using System.Fabric.Test;
    using System.Fabric.Test.Common.FabricUtility;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Text;
    using System.Threading;

    [TestClass]
    public class ApplicationManagementTests
    {
        private static FabricDeployment deployment;
        private static string testOutputPath;
        private static Runspace runspace;
        private static string baseDropPath;

        private static ApplicationDeployer statefulServiceApplicationDeployer;
        private static ApplicationDeployer statelessServiceApplicationDeployer;
        private static ApplicationDeployer statelessServiceUpgradeApplicationDeployer;
        private static ApplicationDeployer applicationWithPLBParametersDeployer;
        private static ServiceDeployer statelessSingletonPartitionServiceDeployer;
        private static ServiceDeployer statefulIn64PartitionServiceDeployer;
        private static string applicationWithPLBParametersPackagePath;
        private static string statefulServiceApplicationPackagePath;
        private static string statelessServiceApplicationPackagePath;
        private static string statelessServiceUpgradeApplicationPackagePath;
        private static StatefulServiceDescription statefulServiceDescription;
        private static ServiceGroupDescription statefulServiceGroupDescription;
        private static StatelessServiceDescription statelessServiceDescription;
        private static ServiceGroupDescription statelessServiceGroupDescription;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            LogHelper.Log("Make drop");
            TestCommon.MakeDrop(Constants.BaseDrop);
            baseDropPath = Path.Combine(Directory.GetCurrentDirectory(), Constants.BaseDrop);

            LogHelper.Log("Create and open runspace");
            runspace = TestCommon.CreateRunSpace(baseDropPath);
            runspace.Open();

            LogHelper.Log("Initializing deployment");
            InitDeployment();
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            LogHelper.Log("Disposing deployment");
            DisposeDeployment();

            LogHelper.Log("Close and dispose runspace");
            runspace.Close();
            runspace.Dispose();

            LogHelper.Log("Delete drop folder");
            FileUtility.RecursiveDelete(baseDropPath);
            Directory.Delete(baseDropPath);
        }

        [TestMethod]
        [Owner("heeldin")]
        public void FunctionalTests()
        {
            ConnectCluster();
            TestClusterConnection();
            GetClusterConnectionTest();

            GetNodeTest();
            GetSystemServicesTest();
            GetSystemServicesPartitionAndReplicaTest();
            GetPartitionByIdTest();

            ReportFaultTransientTest();
            ReportFaultPermanentTest();

            TestApplicationLoadBalancingParameters();

            BuildStatefulServiceTestApplication(true);
            TestApplicationPackage(statefulServiceApplicationPackagePath);
            CopyApplicationPackage(statefulServiceApplicationPackagePath);
            CopyApplicationPackageToReservedFolder(statefulServiceApplicationPackagePath);
            RemoveApplicationPackageFromReservedFolder(statefulServiceApplicationPackagePath);
            RegisterApplicationType(statefulServiceApplicationPackagePath);
            VerifyApplicationTypeRegistered(statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);
            NewApplication(statefulServiceDescription.ApplicationName, statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);
            VerifyApplicationCreated(statefulServiceDescription.ApplicationName, statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);
            VerifyApplicationLoadBalancingProperties(statefulServiceDescription.ApplicationName, 0, 0, null);
            GetApplicationHealthTest(statefulServiceDescription.ApplicationName);


            GetNodeHealthTest();

            CreateService(statefulServiceDescription);
            VerifyServiceCreated(statefulServiceDescription);
            GetServiceHealthTest(statefulServiceDescription.ServiceName);
            // Service Group
            CreateServiceGroup(statefulServiceGroupDescription);
            VerifyServiceGroupCreated(statefulServiceGroupDescription, statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);
            RemoveServiceGroup(statefulServiceGroupDescription.ServiceDescription.ServiceName);
            VerifyServiceGroupDeleted(statefulServiceGroupDescription);

            var statefulServiceUpdateDescription = GetStatefulServiceUpdateDescription();
            UpdateService(statefulServiceDescription.ServiceName, statefulServiceUpdateDescription);
            VerifyServiceUpdated(statefulServiceDescription.ServiceName, statefulServiceUpdateDescription);
            statefulServiceUpdateDescription = ResetStatefulServiceUpdateDescription();
            UpdateService(statefulServiceDescription.ServiceName, statefulServiceUpdateDescription);
            VerifyServiceUpdated(statefulServiceDescription.ServiceName, statefulServiceUpdateDescription);
            RemoveService(statefulServiceDescription.ServiceName);

            statefulServiceDescription.ScalingPolicies.Clear();
            NewServiceFromTemplate(statefulServiceDescription.ApplicationName, statefulServiceDescription.ServiceName, statefulServiceDescription.ServiceTypeName);
            VerifyServiceCreated(statefulServiceDescription);
            RemoveService(statefulServiceDescription.ServiceName);

            VerifyServiceDeleted(statefulServiceDescription);
            RemoveApplication(statefulServiceDescription.ApplicationName);
            VerifyApplicationDeleted(statefulServiceDescription.ApplicationName);
            UnregisterApplicationType(statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);
            VerifyApplicationTypeUnRegistered(statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);

            BuildStatelessServiceTestApplication();
            TestApplicationPackage(statelessServiceApplicationPackagePath);
            CopyApplicationPackage(statelessServiceApplicationPackagePath);
            RegisterApplicationType(statelessServiceApplicationPackagePath);
            VerifyApplicationTypeRegistered(statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            NewApplication(statelessServiceDescription.ApplicationName, statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            VerifyApplicationCreated(statelessServiceDescription.ApplicationName, statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            VerifyApplicationLoadBalancingProperties(statelessServiceDescription.ApplicationName, 0, 0, null);

            CreateService(statelessServiceDescription);
            VerifyServiceCreated(statelessServiceDescription);
            var statelessServiceUpdateDescription = GetStatelessServiceUpdateDescription();
            UpdateService(statelessServiceDescription.ServiceName, statelessServiceUpdateDescription);
            VerifyServiceUpdated(statelessServiceDescription.ServiceName, statelessServiceUpdateDescription);
            statelessServiceUpdateDescription = ResetStatelessServiceUpdateDescription();
            UpdateService(statelessServiceDescription.ServiceName, statelessServiceUpdateDescription);
            VerifyServiceUpdated(statelessServiceDescription.ServiceName, statelessServiceUpdateDescription);
            RemoveService(statelessServiceDescription.ServiceName);
            // Service Group
            CreateServiceGroup(statelessServiceGroupDescription);
            VerifyServiceGroupCreated(statelessServiceGroupDescription, statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            RemoveServiceGroup(statelessServiceGroupDescription.ServiceDescription.ServiceName);
            VerifyServiceGroupDeleted(statelessServiceGroupDescription);

            BuildStatelessServiceUpgradeTestApplication();
            TestApplicationPackage(statelessServiceUpgradeApplicationPackagePath);
            CopyApplicationPackage(statelessServiceUpgradeApplicationPackagePath);
            RegisterApplicationType(statelessServiceUpgradeApplicationPackagePath);
            VerifyApplicationTypeRegistered(statelessServiceUpgradeApplicationDeployer.Name, statelessServiceUpgradeApplicationDeployer.Version);
            UpgradeApplicationManual(statelessServiceDescription.ApplicationName, statelessServiceUpgradeApplicationDeployer);
            VerifyApplicationCreated(statelessServiceDescription.ApplicationName, statelessServiceUpgradeApplicationDeployer.Name, statelessServiceUpgradeApplicationDeployer.Version);

            RemoveApplication(statelessServiceDescription.ApplicationName);
            VerifyApplicationDeleted(statelessServiceDescription.ApplicationName);
            UnregisterApplicationType(statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            VerifyApplicationTypeUnRegistered(statelessServiceApplicationDeployer.Name, statelessServiceApplicationDeployer.Version);
            UnregisterApplicationType(statelessServiceUpgradeApplicationDeployer.Name, statelessServiceUpgradeApplicationDeployer.Version);
            VerifyApplicationTypeUnRegistered(statelessServiceUpgradeApplicationDeployer.Name, statelessServiceUpgradeApplicationDeployer.Version);

            RemoveNodeState();
        }

        private static void InitDeployment()
        {
            LogHelper.Log("Creating deployment");
            var parameters = FabricDeploymentParameters.CreateDefault(Constants.TestDllName, Constants.TestCaseName, Constants.FabricNodeCount);
            deployment = new FabricDeployment(parameters);

            LogHelper.Log("Starting deployment");
            deployment.Start();

            LogHelper.Log("Create test directory");
            testOutputPath = Path.Combine(deployment.TestOutputPath, "service");
            FileUtility.CreateDirectoryIfNotExists(testOutputPath);
            LogHelper.Log("Test output path where services will write to: {0}", testOutputPath);
        }

        private static void DisposeDeployment()
        {
            LogHelper.Log("Dispose deployment");
            deployment.Dispose();

            LogHelper.Log("Delete test directory");
            FileUtility.RecursiveDelete(Path.Combine(Directory.GetCurrentDirectory(), Constants.TestDllName, Constants.TestCaseName));
        }

        private static void ConnectCluster()
        {
            LogHelper.Log("ConnectCluster");
            var command = new Command(Constants.ConnectClusterCommand);
            var endpointsBuilder = new StringBuilder();
            foreach (var address in deployment.ClientConnectionAddresses)
            {
                endpointsBuilder.Append(address.Address);
                endpointsBuilder.Append(':');
                endpointsBuilder.Append(address.Port);
                endpointsBuilder.Append(',');
            }

            command.Parameters.Add("ConnectionEndpoint", endpointsBuilder.ToString().TrimEnd(','));
            command.Parameters.Add("HealthReportSendIntervalInSec", 0);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void TestClusterConnection()
        {
            LogHelper.Log("TestClusterConnection");
            var command = new Command(Constants.TestClusterConnectionCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            Assert.IsTrue((bool)(result[0].ImmediateBaseObject));
        }

        private static void GetClusterConnectionTest()
        {
            LogHelper.Log("GetClusterConnectionTest");
            var command = new Command(Constants.GetClusterConnectionCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
        }

        private static void GetNodeTest()
        {
            LogHelper.Log("GetNodeTest");
            var command = new Command(Constants.GetNodeCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(5, result.Count);
            foreach (var item in result)
            {
                TestUtility.AssertAreEqual(item.ImmediateBaseObject is Node, true);
            }
        }

        private static void GetNodeHealthTest()
        {
            LogHelper.Log("GetNodeHealthTest");
            var command = new Command(Constants.GetNodeHealthCommand);
            var nodeName = deployment.Nodes.Last().NodeName;
            command.Parameters.Add("NodeName", nodeName);
            command.Parameters.Add("MaxPercentUnhealthyNodes", 11);
            TestCommon.Log(command);
            var result = InvokeHealthCommandWithRetry(command);
            TestCommon.Log(result);

            TestUtility.AssertAreEqual(1, result.Count);
            var item = result[0].ImmediateBaseObject as NodeHealth;
            Assert.IsNotNull(item);
            Assert.AreEqual(nodeName, item.NodeName);
        }

        private static void GetSystemServicesTest()
        {
            LogHelper.Log("GetSystemServicesTest");
            var command = new Command(Constants.GetServiceCommand);
            command.Parameters.Add("ApplicationName", Constants.SystemApplicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(3, result.Count);
            foreach (var obj in result)
            {
                var item = obj.ImmediateBaseObject as StatefulService;
                Assert.IsNotNull(item);
                if (item.ServiceName.Equals(Constants.NamingServiceName))
                {
                    TestUtility.AssertAreEqual(Constants.NamingServiceType, item.ServiceTypeName);
                    TestUtility.AssertAreEqual(true, item.HasPersistedState);
                }
                else if (item.ServiceName.Equals(Constants.FmServiceName))
                {
                    TestUtility.AssertAreEqual(Constants.FmServiceType, item.ServiceTypeName);
                    TestUtility.AssertAreEqual(true, item.HasPersistedState);
                }
                else if (item.ServiceName.Equals(Constants.CmServiceName))
                {
                    TestUtility.AssertAreEqual(Constants.CmServiceType, item.ServiceTypeName);
                    TestUtility.AssertAreEqual(true, item.HasPersistedState);
                }
                else
                {
                    Assert.Fail("Unexpected system service name");
                }
            }
        }

        private static void GetPartitionByIdTest()
        {
            LogHelper.Log("GetPartitionByIdTest");
            var command = new Command(Constants.GetPartitionCommand);
            var fmPartitionGuid = new Guid("00000000-0000-0000-0000-000000000001");
            command.Parameters.Add("PartitionId", fmPartitionGuid);

            LogHelper.Log("Getting partition from FM's partition with id: " + fmPartitionGuid);
            TestCommon.Log(command);


            var partitionResult = InvokeWithRetry(command);
            TestCommon.Log(partitionResult);
            TestUtility.AssertAreEqual(1, partitionResult.Count);
            var item = partitionResult[0].ImmediateBaseObject as StatefulServicePartition;

            Assert.IsNotNull(item);
            TestUtility.AssertAreEqual(item.PartitionInformation.Id,Constants.FmPartitionId);
        }


        private static void GetSystemServicesPartitionAndReplicaTest()
        {
            LogHelper.Log("GetSystemServicesPartitionAndReplicaTest");
            Uri[] systemServices = { 
                Constants.FmServiceName,
                Constants.CmServiceName,
                Constants.NamingServiceName,
            };

            Guid fmPartition = Guid.Empty;
            long fmReplicaId = 0;

            foreach (var systemService in systemServices)
            {
                var command = new Command(Constants.GetPartitionCommand);
                command.Parameters.Add("ServiceName", systemService);

                LogHelper.Log("Getting partitions for " + systemService);
                var partitionResult = InvokeWithRetry(command);
                TestCommon.Log(partitionResult);
                TestUtility.AssertAreEqual(true, (partitionResult.Count > 0));

                for (int i = 0; i < partitionResult.Count; i++)
                {
                    var item = partitionResult[0].ImmediateBaseObject as StatefulServicePartition;
                    Assert.IsNotNull(item);
                    LogHelper.Log("GetReplicaTest");
                    command = new Command(Constants.GetReplicaCommand);
                    command.Parameters.Add("PartitionId", item.PartitionInformation.Id);
                    TestCommon.Log(command);
                    var replicaResult = InvokeWithRetry(command);
                    TestCommon.Log(replicaResult);

                    TestUtility.AssertAreEqual(true, (replicaResult.Count > 0));
                    var replicaItem = replicaResult[0].ImmediateBaseObject as StatefulServiceReplica;

                    if (systemService == Constants.FmServiceName && i == 0)
                    {
                        fmPartition = item.PartitionInformation.Id;
                        fmReplicaId = replicaItem.Id;
                    }
                }
            }

            ReportReplicaHealthReportAndCheckEntityStates(fmPartition, fmReplicaId);
        }

        private static IEnumerable<StatefulServiceReplica> GetAllReplicasForService(Guid partitionId)
        {
            var command = new Command(Constants.GetReplicaCommand);
            command.Parameters.Add("PartitionId", partitionId);

            LogHelper.Log("Getting partitions for " + partitionId.ToString());
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            return result.Select(e => e.ImmediateBaseObject as StatefulServiceReplica).ToArray();
        }

        private static void BuildAndInvokeReplicaOperationCommand(string commandName, Guid partitionId, StatefulServiceReplica replica)
        {
            var command = new Command(commandName);
            command.Parameters.Add("NodeName", replica.NodeName);
            command.Parameters.Add("PartitionId", partitionId);
            command.Parameters.Add("ReplicaOrInstanceId", replica.Id);

            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static IEnumerable<StatefulServiceReplica> WaitUntilAtLeastReadyUpReplicaCountIsHit(int expectedCount, Guid partitionId)
        {
            IEnumerable<StatefulServiceReplica> output = null;
            MiscUtility.WaitUntil(() =>
                {
                    LogHelper.Log("WaitUntilReadyUpReplicaCount Retry");

                    var currentReplicas = GetAllReplicasForService(partitionId);
                    if (currentReplicas.Count(e => e.ReplicaStatus == ServiceReplicaStatus.Ready) >= expectedCount)
                    {
                        output = currentReplicas;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                },
                TimeSpan.FromMinutes(5));

            return output;
        }

        private static void ReportFaultTransientTest()
        {
            var initialReplicas = GetAllReplicasForService(Constants.FmPartitionId);

            // Report Fault Transient on the primary
            var primaryReplica = initialReplicas.First(e => e.ReplicaRole == ReplicaRole.Primary);

            // Invoke command
            BuildAndInvokeReplicaOperationCommand(Constants.RestartReplicaCommand, Constants.FmPartitionId, primaryReplica);

            // Validation phase
            // The primary should change
            // Hope that there is no PLB moving things around
            MiscUtility.WaitUntil(() =>
                {
                    var currentReplicaSet = WaitUntilAtLeastReadyUpReplicaCountIsHit(initialReplicas.Count(), Constants.FmPartitionId);
                    var currentReplicaOnOldPrimaryNode = currentReplicaSet.First(e => e.NodeName == primaryReplica.NodeName);

                    return currentReplicaOnOldPrimaryNode.ReplicaRole == ReplicaRole.ActiveSecondary;
                },
                TimeSpan.FromSeconds(300));
        }

        private static void ReportFaultPermanentTest()
        {
            var initialReplicas = GetAllReplicasForService(Constants.FmPartitionId);

            // Report Fault Transient on the primary
            var primaryReplica = initialReplicas.First(e => e.ReplicaRole == ReplicaRole.Primary);

            // Invoke command
            BuildAndInvokeReplicaOperationCommand(Constants.RemoveReplicaCommand, Constants.FmPartitionId, primaryReplica);

            // Validation phase
            // The primary should change
            // Hope that there is no PLB moving things around
            MiscUtility.WaitUntil(() =>
                {
                    var currentReplicaSet = WaitUntilAtLeastReadyUpReplicaCountIsHit(initialReplicas.Count(), Constants.FmPartitionId);
                    var currentReplicaWithEarlierReplicaId = currentReplicaSet.FirstOrDefault(e => e.Id == primaryReplica.Id);

                    return currentReplicaWithEarlierReplicaId == null || currentReplicaWithEarlierReplicaId.ReplicaStatus == ServiceReplicaStatus.Dropped;
                },
                TimeSpan.FromSeconds(300));
        }

        private static void ReportReplicaHealthReportAndCheckEntityStates(Guid partition, long replicaId)
        {
            // First wait for the system report
            GetReplicaHealthTest(partition, replicaId, false, HealthState.Ok);

            // Report health event on a replica and then check health
            SendReplicaHealthReport(partition, replicaId);

            // Check health up the hierarchy 
            GetReplicaHealthTest(partition, replicaId, true, HealthState.Error);
            GetPartitionHealthTest(partition, true, HealthState.Error, HealthEvaluationKind.Replicas);
            GetServiceHealthTest(Constants.FmServiceName, true, HealthState.Error, HealthEvaluationKind.Partitions);
            GetApplicationHealthTest(Constants.SystemApplicationName, true, HealthState.Error, HealthEvaluationKind.Services);

            // Application evaluation is done using the default app policy, so it's evaluated at warning
            GetClusterHealthTest(true, HealthState.Warning);
        }

        private static void SendReplicaHealthReport(Guid partition, long replicaId)
        {
            LogHelper.Log("ReportReplicaHealthTest for {0} {1}", partition, replicaId);
            var command = new Command(Constants.SendReplicaHealthReportCommand);
            command.Parameters.Add("PartitionId", partition.ToString());
            command.Parameters.Add("ReplicaId", replicaId);
            command.Parameters.Add("HealthState", HealthState.Warning);
            command.Parameters.Add("SourceId", "ApplicationManagementTests");
            command.Parameters.Add("HealthProperty", "Test");
            TestCommon.Log(command);
            var sendReportResult = InvokeWithRetry(command);
            TestCommon.Log(sendReportResult);
        }

        private static void GetReplicaHealthTest(Guid partitionId, long replicaId, bool considerWarningAsError, HealthState expectedHealthState)
        {
            LogHelper.Log("GetReplicaHealthTest");
            var command = new Command(Constants.GetReplicaHealthCommand);
            command.Parameters.Add("PartitionId", partitionId.ToString());
            command.Parameters.Add("ReplicaOrInstanceId", replicaId);
            command.Parameters.Add("ConsiderWarningAsError", considerWarningAsError);
            TestCommon.Log(command);

            int retryCount = Constants.MaxRetryCount;
            do
            {
                var result = InvokeHealthCommandWithRetry(command);
                TestCommon.Log(result);

                TestUtility.AssertAreEqual(1, result.Count);
                var item = result[0].ImmediateBaseObject as ReplicaHealth;
                Assert.IsNotNull(item);
                Assert.AreEqual(partitionId, item.PartitionId);
                Assert.AreEqual(replicaId, item.Id);

                if (expectedHealthState != item.AggregatedHealthState)
                {
                    // Retry, the reports may not have been received yet
                    LogHelper.Log("Received health state not the desired one: expected {0}, actual {1}. Retrying...", expectedHealthState, item.AggregatedHealthState);
                    Thread.Sleep(TimeSpan.FromSeconds(Constants.RetryIntervalInSeconds));
                    continue;
                }

                Assert.AreEqual(expectedHealthState, item.AggregatedHealthState);
                if (expectedHealthState != HealthState.Ok)
                {
                    Assert.IsNotNull(item.UnhealthyEvaluations);
                    Assert.AreEqual(1, item.UnhealthyEvaluations.Count);
                    Assert.AreEqual(HealthEvaluationKind.Event, item.UnhealthyEvaluations[0].Kind);
                    var eventReason = item.UnhealthyEvaluations[0] as EventHealthEvaluation;
                    Assert.IsNotNull(eventReason);
                }

                return;
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in GetReplicaHealthTest");
        }

        private static void GetPartitionHealthTest(Guid partitionId, bool considerWarningAsError, HealthState expectedHealthState, HealthEvaluationKind expectedReason = HealthEvaluationKind.Invalid)
        {
            LogHelper.Log("GetPartitionHealthTest");
            var command = new Command(Constants.GetPartitionHealthCommand);
            command.Parameters.Add("PartitionId", partitionId.ToString());
            command.Parameters.Add("ConsiderWarningAsError", considerWarningAsError);
            TestCommon.Log(command);

            int retryCount = Constants.MaxRetryCount;

            do
            {
                var result = InvokeHealthCommandWithRetry(command);
                TestCommon.Log(result);

                TestUtility.AssertAreEqual(1, result.Count);
                var item = result[0].ImmediateBaseObject as PartitionHealth;
                Assert.IsNotNull(item);
                Assert.AreEqual(partitionId, item.PartitionId);

                if (expectedHealthState != item.AggregatedHealthState)
                {
                    // Retry, the reports may not have been received yet
                    LogHelper.Log("Received health state not the desired one: expected {0}, actual {1}. Retrying...", expectedHealthState, item.AggregatedHealthState);
                    Thread.Sleep(TimeSpan.FromSeconds(Constants.RetryIntervalInSeconds));
                    continue;
                }

                if (expectedHealthState != HealthState.Ok)
                {
                    Assert.IsNotNull(item.UnhealthyEvaluations);
                    Assert.AreEqual(1, item.UnhealthyEvaluations.Count);

                    if (expectedReason != HealthEvaluationKind.Invalid &&
                        expectedReason != item.UnhealthyEvaluations[0].Kind)
                    {
                        Assert.Fail("Reason kind does not match expected: expected {0}, actual {1}", expectedReason, item.UnhealthyEvaluations[0].Kind);
                    }

                    if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Event)
                    {
                        var eventReason = item.UnhealthyEvaluations[0] as EventHealthEvaluation;
                        Assert.IsNotNull(eventReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Replicas)
                    {
                        var replicasReason = item.UnhealthyEvaluations[0] as ReplicasHealthEvaluation;
                        Assert.IsNotNull(replicasReason);
                    }
                    else
                    {
                        Assert.Fail("Unexpected health reason");
                    }
                }

                return;
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in GetPartitionHealthTest");
        }

        private static void GetServiceHealthTest(
            Uri serviceName,
            bool considerWarningAsError = false,
            HealthState expectedHealthState = HealthState.Ok,
            HealthEvaluationKind expectedReason = HealthEvaluationKind.Invalid)
        {
            LogHelper.Log("GetServiceHealth");
            var command = new Command(Constants.GetServiceHealthCommand);
            command.Parameters.Add("ServiceName", serviceName);
            command.Parameters.Add("ConsiderWarningAsError", considerWarningAsError);
            TestCommon.Log(command);

            int retryCount = Constants.MaxRetryCount;

            do
            {
                var result = InvokeHealthCommandWithRetry(command);

                TestCommon.Log(result);
                TestUtility.AssertAreEqual(1, result.Count);
                var item = result[0].ImmediateBaseObject as ServiceHealth;
                Assert.IsNotNull(item);
                Assert.AreEqual(serviceName, item.ServiceName);
                if (expectedHealthState != item.AggregatedHealthState)
                {
                    // Retry, the reports may not have been received yet
                    LogHelper.Log("Received health state not the desired one: expected {0}, actual {1}. Retrying...", expectedHealthState, item.AggregatedHealthState);
                    Thread.Sleep(TimeSpan.FromSeconds(Constants.RetryIntervalInSeconds));
                    continue;
                }

                if (expectedHealthState != HealthState.Ok)
                {
                    Assert.IsNotNull(item.UnhealthyEvaluations);
                    Assert.AreEqual(1, item.UnhealthyEvaluations.Count);

                    if (expectedReason != HealthEvaluationKind.Invalid &&
                        expectedReason != item.UnhealthyEvaluations[0].Kind)
                    {
                        Assert.Fail("Reason kind does not match expected: expected {0}, actual {1}", expectedReason, item.UnhealthyEvaluations[0].Kind);
                    }

                    if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Event)
                    {
                        var eventReason = item.UnhealthyEvaluations[0] as EventHealthEvaluation;
                        Assert.IsNotNull(eventReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Partitions)
                    {
                        var partitionsReason = item.UnhealthyEvaluations[0] as PartitionsHealthEvaluation;
                        Assert.IsNotNull(partitionsReason);
                    }
                    else
                    {
                        Assert.Fail("Unexpected health reason");
                    }
                }

                return;
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in GetServiceHealthTest");
        }

        private static void GetApplicationHealthTest(
            Uri applicationName,
            bool considerWarningAsError = false,
            HealthState expectedHealthState = HealthState.Ok,
            HealthEvaluationKind expectedReason = HealthEvaluationKind.Invalid)
        {
            LogHelper.Log("GetApplicationHealth");
            var command = new Command(Constants.GetApplicationHealthCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            command.Parameters.Add("ConsiderWarningAsError", considerWarningAsError);
            TestCommon.Log(command);

            int retryCount = Constants.MaxRetryCount;

            do
            {
                var result = InvokeHealthCommandWithRetry(command);

                TestCommon.Log(result);
                TestUtility.AssertAreEqual(1, result.Count);
                var item = result[0].ImmediateBaseObject as ApplicationHealth;
                Assert.IsNotNull(item);
                Assert.AreEqual(applicationName, item.ApplicationName);

                if (expectedHealthState != item.AggregatedHealthState)
                {
                    // Retry, the reports may not have been received yet
                    LogHelper.Log("Received health state not the desired one: expected {0}, actual {1}. Retrying...", expectedHealthState, item.AggregatedHealthState);
                    Thread.Sleep(TimeSpan.FromSeconds(Constants.RetryIntervalInSeconds));
                    continue;
                }

                if (expectedHealthState != HealthState.Ok)
                {
                    Assert.IsNotNull(item.UnhealthyEvaluations);
                    Assert.AreEqual(1, item.UnhealthyEvaluations.Count);

                    if (expectedReason != HealthEvaluationKind.Invalid &&
                        expectedReason != item.UnhealthyEvaluations[0].Kind)
                    {
                        Assert.Fail("Reason kind does not match expected: expected {0}, actual {1}", expectedReason, item.UnhealthyEvaluations[0].Kind);
                    }

                    if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Event)
                    {
                        var eventReason = item.UnhealthyEvaluations[0] as EventHealthEvaluation;
                        Assert.IsNotNull(eventReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Services)
                    {
                        var servicesReason = item.UnhealthyEvaluations[0] as ServicesHealthEvaluation;
                        Assert.IsNotNull(servicesReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.DeployedApplications)
                    {
                        var deployedApplicationsReason = item.UnhealthyEvaluations[0] as DeployedApplicationsHealthEvaluation;
                        Assert.IsNotNull(deployedApplicationsReason);
                    }
                    else
                    {
                        Assert.Fail("Unexpected health reason");
                    }
                }

                return;
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in GetApplicationHealthTest");
        }

        private static void GetClusterHealthTest(
           bool considerWarningAsError = false,
           HealthState expectedHealthState = HealthState.Ok,
           HealthEvaluationKind expectedReason = HealthEvaluationKind.Invalid)
        {
            LogHelper.Log("GetClusterHealth");
            var command = new Command(Constants.GetClusterHealthCommand);
            command.Parameters.Add("ConsiderWarningAsError", considerWarningAsError);
            TestCommon.Log(command);

            int retryCount = Constants.MaxRetryCount;

            do
            {
                var result = InvokeHealthCommandWithRetry(command);

                TestCommon.Log(result);
                TestUtility.AssertAreEqual(1, result.Count);
                var item = result[0].ImmediateBaseObject as ClusterHealth;
                Assert.IsNotNull(item);

                if (expectedHealthState != item.AggregatedHealthState)
                {
                    // Retry, the reports may not have been received yet
                    LogHelper.Log("Received health state not the desired one: expected {0}, actual {1}. Retrying...", expectedHealthState, item.AggregatedHealthState);
                    Thread.Sleep(TimeSpan.FromSeconds(Constants.RetryIntervalInSeconds));
                    continue;
                }

                if (expectedHealthState != HealthState.Ok)
                {
                    Assert.IsNotNull(item.UnhealthyEvaluations);
                    Assert.AreEqual(1, item.UnhealthyEvaluations.Count);

                    if (expectedReason != HealthEvaluationKind.Invalid &&
                        expectedReason != item.UnhealthyEvaluations[0].Kind)
                    {
                        Assert.Fail("Reason kind does not match expected: expected {0}, actual {1}", expectedReason, item.UnhealthyEvaluations[0].Kind);
                    }

                    if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Event)
                    {
                        var eventReason = item.UnhealthyEvaluations[0] as EventHealthEvaluation;
                        Assert.IsNotNull(eventReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Applications)
                    {
                        var applicationsReason = item.UnhealthyEvaluations[0] as ApplicationsHealthEvaluation;
                        Assert.IsNotNull(applicationsReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.ApplicationTypeApplications)
                    {
                        var applicationTypeApplicationsReason = item.UnhealthyEvaluations[0] as ApplicationTypeApplicationsHealthEvaluation;
                        Assert.IsNotNull(applicationTypeApplicationsReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.Nodes)
                    {
                        var nodesReason = item.UnhealthyEvaluations[0] as NodesHealthEvaluation;
                        Assert.IsNotNull(nodesReason);
                    }
                    else if (item.UnhealthyEvaluations[0].Kind == HealthEvaluationKind.SystemApplication)
                    {
                        var systemApplicationReason = item.UnhealthyEvaluations[0] as SystemApplicationHealthEvaluation;
                        Assert.IsNotNull(systemApplicationReason);
                    }
                    else
                    {
                        Assert.Fail("Unexpected health reason");
                    }
                }

                return;
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in GetClusterHealthTest");
        }

        private static void BuildTestApplicationWithPLBParamters()
        {
            LogHelper.Log("BuildTestApplicationWithPLBParamters");
            applicationWithPLBParametersDeployer = new ApplicationDeployer
            {
                Name = "StatefulServiceApplication",
                Version = "1.0"
            };



            applicationWithPLBParametersPackagePath = Path.Combine(deployment.ApplicationStagingPath, applicationWithPLBParametersDeployer.Name);
            applicationWithPLBParametersDeployer.Deploy(applicationWithPLBParametersPackagePath);
        }

        private static void BuildStatefulServiceTestApplication(bool includeScaling)
        {
            LogHelper.Log("BuildStatefulServiceTestApplication");
            statefulServiceApplicationDeployer = new ApplicationDeployer
            {
                Name = "StatefulServiceApplication",
                Version = "1.0"
            };

            statefulIn64PartitionServiceDeployer = new ServiceDeployer("Stateful")
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                ServiceTypeImplementation = typeof(StatefulSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatefulService",
                ServiceGroupTypeName = "SanityTestStatefulServiceGroup",
                MinReplicaSetSize = 1,
                TargetReplicaSetSize = 5,
                Partition = new UniformInt64RangePartitionSchemeDescription(1, 1, 1000),
                HasPersistedState = false,
                Version = "1.0",
                ConfigPackageName = SanityTest.ConfigurationPackageName,
                DefaultMoveCost = MoveCost.Medium
            };

            statefulIn64PartitionServiceDeployer.ConfigurationSettings.Add(Tuple.Create(SanityTest.ConfigurationSectionName, SanityTest.ConfigurationParameterName, testOutputPath));
            statefulIn64PartitionServiceDeployer.ConfigurationSettings.Add(Tuple.Create(SanityTest.ConfigurationSectionName, SanityTest.ConfigurationAddressParameterName, SanityTest.ConfigurationAddressParameterValue));

            statefulServiceDescription = new StatefulServiceDescription
            {
                ApplicationName = new Uri(string.Format("fabric:/{0}", statefulServiceApplicationDeployer.Name)),
                ServiceName = new Uri(string.Format("fabric:/{0}/{1}_new", statefulServiceApplicationDeployer.Name, statefulIn64PartitionServiceDeployer.ServiceTypeName)),
                ServiceTypeName = "SanityTestStatefulService"
            };
            var namedPartitionDescription = new NamedPartitionSchemeDescription();
            if (includeScaling)
            {
                namedPartitionDescription.PartitionNames.Add("0");
            }
            else
            {
                namedPartitionDescription.PartitionNames.Add("SanityTestStatefulServicePartition");
            }
            statefulServiceDescription.PartitionSchemeDescription = namedPartitionDescription;
            statefulServiceDescription.HasPersistedState = false;
            statefulServiceDescription.TargetReplicaSetSize = 5;
            statefulServiceDescription.MinReplicaSetSize = 1;
            statefulServiceDescription.DefaultMoveCost = MoveCost.Medium;

            if (includeScaling)
            {
                AverageServiceLoadScalingTrigger scalingTrigger = new AverageServiceLoadScalingTrigger();
                AddRemoveIncrementalNamedPartitionScalingMechanism scaleMechanism = new AddRemoveIncrementalNamedPartitionScalingMechanism();
                scalingTrigger.MetricName = "servicefabric:/_CpuCores";
                scaleMechanism.MinPartitionCount = 1;
                scaleMechanism.MaxPartitionCount = 4;
                scalingTrigger.LowerLoadThreshold = 1.0;
                scalingTrigger.UpperLoadThreshold = 5.0;
                scalingTrigger.ScaleInterval = TimeSpan.FromMinutes(12);
                scalingTrigger.UseOnlyPrimaryLoad = true;
                scaleMechanism.ScaleIncrement = 1;
                statefulServiceDescription.ScalingPolicies.Add(new ScalingPolicyDescription(scaleMechanism, scalingTrigger));
            }


            // Service Group
            statefulServiceGroupDescription = new ServiceGroupDescription();
            statefulServiceGroupDescription.ServiceDescription = new StatefulServiceDescription
            {
                ApplicationName = new Uri(string.Format("fabric:/{0}", statefulServiceApplicationDeployer.Name)),
                ServiceName = new Uri(string.Format("fabric:/{0}/{1}_newSG", statefulServiceApplicationDeployer.Name, statefulIn64PartitionServiceDeployer.ServiceTypeName)),
                ServiceTypeName = "SanityTestStatefulService"
            };
            ((StatefulServiceDescription)statefulServiceGroupDescription.ServiceDescription).PartitionSchemeDescription = namedPartitionDescription;
            ((StatefulServiceDescription)statefulServiceGroupDescription.ServiceDescription).HasPersistedState = false;
            ((StatefulServiceDescription)statefulServiceGroupDescription.ServiceDescription).TargetReplicaSetSize = 5;
            ((StatefulServiceDescription)statefulServiceGroupDescription.ServiceDescription).MinReplicaSetSize = 1;
            statefulServiceGroupDescription.MemberDescriptions = new List<ServiceGroupMemberDescription>();
            ServiceGroupMemberDescription serviceGroupMemberDescription = new ServiceGroupMemberDescription();
            serviceGroupMemberDescription.ServiceName = new Uri(string.Format("fabric:/{0}/{1}_newSG#{2}",
                statefulServiceApplicationDeployer.Name, statefulIn64PartitionServiceDeployer.ServiceTypeName, "member1"));
            serviceGroupMemberDescription.ServiceTypeName = "SanityTestStatefulService";
            statefulServiceGroupDescription.MemberDescriptions.Add(serviceGroupMemberDescription);

            statefulServiceApplicationDeployer.Services.Add(statefulIn64PartitionServiceDeployer);
            statefulServiceApplicationPackagePath = Path.Combine(deployment.ApplicationStagingPath, statefulServiceApplicationDeployer.Name);
            statefulServiceApplicationDeployer.Deploy(statefulServiceApplicationPackagePath);
        }

        private static void BuildStatelessServiceTestApplication()
        {
            LogHelper.Log("BuildStatelessServiceTestApplication");
            statelessServiceApplicationDeployer = new ApplicationDeployer
            {
                Name = "StatelessServiceApplication",
                Version = "1.0"
            };

            statelessSingletonPartitionServiceDeployer = new ServiceDeployer("Stateless")
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                InstanceCount = 3,
                ServiceTypeImplementation = typeof(StatelessSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatelessService",
                ServiceGroupTypeName = "SanityTestStatefulServiceGroup",
                Partition = new SingletonPartitionSchemeDescription(),
                Version = "1.0",
                ConfigPackageName = SanityTest.ConfigurationPackageName,
                DefaultMoveCost = MoveCost.Medium
            };

            statelessSingletonPartitionServiceDeployer.ConfigurationSettings.Add(Tuple.Create(SanityTest.ConfigurationSectionName, SanityTest.ConfigurationParameterName, testOutputPath));
            statelessSingletonPartitionServiceDeployer.ConfigurationSettings.Add(Tuple.Create(SanityTest.ConfigurationSectionName, SanityTest.ConfigurationAddressParameterName, SanityTest.ConfigurationAddressParameterValue));

            statelessServiceDescription = new StatelessServiceDescription
            {
                ApplicationName = new Uri(string.Format("fabric:/{0}", statelessServiceApplicationDeployer.Name)),
                ServiceName = new Uri(string.Format("fabric:/{0}/{1}_new", statelessServiceApplicationDeployer.Name, statelessSingletonPartitionServiceDeployer.ServiceTypeName)),
                ServiceTypeName = "SanityTestStatelessService",
                PartitionSchemeDescription = new SingletonPartitionSchemeDescription(),
                InstanceCount = 3,
                DefaultMoveCost = MoveCost.Medium
            };

            AveragePartitionLoadScalingTrigger scalingTrigger = new AveragePartitionLoadScalingTrigger();
            PartitionInstanceCountScaleMechanism scaleMechanism = new PartitionInstanceCountScaleMechanism();
            scalingTrigger.MetricName = "servicefabric:/_CpuCores";
            scaleMechanism.MinInstanceCount = 1;
            scaleMechanism.MaxInstanceCount = 3;
            scalingTrigger.LowerLoadThreshold = 1.0;
            scalingTrigger.UpperLoadThreshold = 2.0;
            scalingTrigger.ScaleInterval = TimeSpan.FromMinutes(10);
            scaleMechanism.ScaleIncrement = 1;
            statelessServiceDescription.ScalingPolicies.Add(new ScalingPolicyDescription(scaleMechanism, scalingTrigger));

            // Service Group
            statelessServiceGroupDescription = new ServiceGroupDescription();
            statelessServiceGroupDescription.ServiceDescription = new StatelessServiceDescription
            {
                ApplicationName = new Uri(string.Format("fabric:/{0}", statelessServiceApplicationDeployer.Name)),
                ServiceName = new Uri(string.Format("fabric:/{0}/{1}_newSG", statelessServiceApplicationDeployer.Name, statelessSingletonPartitionServiceDeployer.ServiceTypeName)),
                ServiceTypeName = "SanityTestStatelessService"
            };
            ((StatelessServiceDescription)statelessServiceGroupDescription.ServiceDescription).InstanceCount = 3;
            ((StatelessServiceDescription)statelessServiceGroupDescription.ServiceDescription).PartitionSchemeDescription = new SingletonPartitionSchemeDescription();
            statelessServiceGroupDescription.MemberDescriptions = new List<ServiceGroupMemberDescription>();
            ServiceGroupMemberDescription serviceGroupMemberDescription = new ServiceGroupMemberDescription();
            serviceGroupMemberDescription.ServiceName = new Uri(string.Format("fabric:/{0}/{1}_newSG#{2}",
                statelessServiceApplicationDeployer.Name, statelessSingletonPartitionServiceDeployer.ServiceTypeName, "member1"));
            serviceGroupMemberDescription.ServiceTypeName = "SanityTestStatelessService";
            statelessServiceGroupDescription.MemberDescriptions.Add(serviceGroupMemberDescription);

            statelessServiceApplicationDeployer.Services.Add(statelessSingletonPartitionServiceDeployer);
            statelessServiceApplicationPackagePath = Path.Combine(deployment.ApplicationStagingPath, statelessServiceApplicationDeployer.Name);
            statelessServiceApplicationDeployer.Deploy(statelessServiceApplicationPackagePath);
        }

        private static void BuildStatelessServiceUpgradeTestApplication()
        {
            LogHelper.Log("BuildStatelessServiceUpgradeTestApplication");
            statelessServiceUpgradeApplicationDeployer = new ApplicationDeployer
            {
                Name = "StatelessServiceApplication",
                Version = "2.0"
            };

            statelessSingletonPartitionServiceDeployer.Version = "2.0";
            statelessServiceUpgradeApplicationDeployer.Services.Add(statelessSingletonPartitionServiceDeployer);
            statelessServiceUpgradeApplicationPackagePath = Path.Combine(deployment.ApplicationStagingPath, statelessServiceApplicationDeployer.Name + "V2");
            statelessServiceUpgradeApplicationDeployer.Deploy(statelessServiceUpgradeApplicationPackagePath);
        }

        private static StatefulServiceUpdateDescription GetStatefulServiceUpdateDescription()
        {
            var description = new StatefulServiceUpdateDescription()
            {
                TargetReplicaSetSize = statefulServiceDescription.TargetReplicaSetSize + 1,
                MinReplicaSetSize = statefulServiceDescription.MinReplicaSetSize + 1,
                DefaultMoveCost = MoveCost.Zero,
                PlacementConstraints = "FaultDomain != fd:/8"
            };
            description.Correlations = new List<ServiceCorrelationDescription>();
            description.Correlations.Add(new ServiceCorrelationDescription()
            {
                Scheme = ServiceCorrelationScheme.Affinity,
                ServiceName = new Uri(string.Format("fabric:/{0}", "foo")),
            });

            description.PlacementPolicies = new List<ServicePlacementPolicyDescription>();
            description.PlacementPolicies.Add(new ServicePlacementInvalidDomainPolicyDescription
            {
                Type = ServicePlacementPolicyType.InvalidDomain,
                DomainName = "fd:/dc1"
            });

            description.ScalingPolicies = new List<ScalingPolicyDescription>();
            AverageServiceLoadScalingTrigger scalingTrigger = new AverageServiceLoadScalingTrigger();
            AddRemoveIncrementalNamedPartitionScalingMechanism scaleMechanism = new AddRemoveIncrementalNamedPartitionScalingMechanism();
            scalingTrigger.MetricName = "servicefabric:/_CpuCores";
            scaleMechanism.MinPartitionCount = 1;
            scaleMechanism.MaxPartitionCount = 5;
            scalingTrigger.LowerLoadThreshold = 1.8;
            scalingTrigger.UpperLoadThreshold = 2.0;
            scalingTrigger.ScaleInterval = TimeSpan.FromMinutes(10);
            scalingTrigger.UseOnlyPrimaryLoad = false;
            scaleMechanism.ScaleIncrement = 1;
            description.ScalingPolicies.Add(new ScalingPolicyDescription(scaleMechanism, scalingTrigger));

            return description;
        }

        private static StatefulServiceUpdateDescription ResetStatefulServiceUpdateDescription()
        {
            var description = new StatefulServiceUpdateDescription()
            {
                TargetReplicaSetSize = statefulServiceDescription.TargetReplicaSetSize + 1,
                MinReplicaSetSize = statefulServiceDescription.MinReplicaSetSize + 1,
                PlacementConstraints = ""
            };

            description.Correlations = new List<ServiceCorrelationDescription>();
            description.PlacementPolicies = new List<ServicePlacementPolicyDescription>();
            description.ScalingPolicies = new List<ScalingPolicyDescription>();
            return description;
        }

        private static StatelessServiceUpdateDescription GetStatelessServiceUpdateDescription()
        {
            var description = new StatelessServiceUpdateDescription()
            {
                InstanceCount = statelessServiceDescription.InstanceCount + 1,
                DefaultMoveCost = MoveCost.Zero,
                PlacementConstraints = "FaultDomain != fd:/9"
            };

            description.Correlations = new List<ServiceCorrelationDescription>();
            description.Correlations.Add(new ServiceCorrelationDescription()
            {
                Scheme = ServiceCorrelationScheme.Affinity,
                ServiceName = new Uri(string.Format("fabric:/{0}", "bar")),
            });

            description.PlacementPolicies = new List<ServicePlacementPolicyDescription>();
            description.PlacementPolicies.Add(new ServicePlacementInvalidDomainPolicyDescription
            {
                Type = ServicePlacementPolicyType.InvalidDomain,
                DomainName = "fd:/dc2"
            });

            description.ScalingPolicies = new List<ScalingPolicyDescription>();
            AveragePartitionLoadScalingTrigger scalingTrigger = new AveragePartitionLoadScalingTrigger();
            PartitionInstanceCountScaleMechanism scaleMechanism = new PartitionInstanceCountScaleMechanism();
            scalingTrigger.MetricName = "servicefabric:/_CpuCores";
            scaleMechanism.MinInstanceCount = 1;
            scaleMechanism.MaxInstanceCount = 5;
            scalingTrigger.LowerLoadThreshold = 1.5;
            scalingTrigger.UpperLoadThreshold = 2.0;
            scalingTrigger.ScaleInterval = TimeSpan.FromMinutes(10);
            scaleMechanism.ScaleIncrement = 1;
            description.ScalingPolicies.Add(new ScalingPolicyDescription(scaleMechanism, scalingTrigger));

            return description;
        }

        private static StatelessServiceUpdateDescription ResetStatelessServiceUpdateDescription()
        {
            var description = new StatelessServiceUpdateDescription()
            {
                InstanceCount = statelessServiceDescription.InstanceCount + 1,
                PlacementConstraints = "FaultDomain != fd:/9"
            };

            description.Correlations = new List<ServiceCorrelationDescription>();
            description.PlacementPolicies = new List<ServicePlacementPolicyDescription>();
            description.ScalingPolicies = new List<ScalingPolicyDescription>();

            return description;
        }

        private static void TestApplicationPackage(string applicationPackagePath)
        {
            LogHelper.Log("TestApplicationPackage");
            var command = new Command(Constants.TestApplicationPackageCommand);
            command.Parameters.Add("ApplicationPackagePath", new Random().Next() % 2 == 0 ? applicationPackagePath : applicationPackagePath + @"\");
            command.Parameters.Add("ImageStoreConnectionString", "file:" + deployment.ImageStoreRootPath);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            Assert.IsTrue((bool)(result[0].ImmediateBaseObject));
        }

        private static void CopyApplicationPackage(string applicationPackagePath)
        {
            LogHelper.Log("CopyApplicationPackage");
            var command = new Command(Constants.CopyApplicationPackageCommand);
            command.Parameters.Add("ApplicationPackagePath", applicationPackagePath);
            command.Parameters.Add("ImageStoreConnectionString", "file:" + deployment.ImageStoreRootPath);
            command.Parameters.Add("ApplicationPackagePathInImageStore", new DirectoryInfo(applicationPackagePath).Name);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void RemoveApplicationPackageFromReservedFolder(string applicationPackagePath)
        {
            LogHelper.Log("RemoveApplicationPackage");
            var command = new Command(Constants.RemoveApplicationPackageCommand);
            command.Parameters.Add("ImageStoreConnectionString", "file:" + deployment.ImageStoreRootPath);
            command.Parameters.Add("ApplicationPackagePathInImageStore", "Store");
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();

            try
            {
                pipeline.Commands.Add(command);
                pipeline.Invoke();
                Assert.IsTrue(pipeline.PipelineStateInfo.State == PipelineState.Stopping);
            }
            catch (CmdletInvocationException exception)
            {
                if (exception.InnerException is FabricException)
                {
                    var fabricException = exception.InnerException as FabricException;
                    Assert.IsTrue(fabricException.ErrorCode == FabricErrorCode.ImageBuilderReservedDirectoryError);
                }
                else
                {
                    Assert.IsTrue(exception.ErrorRecord.Exception is System.Management.Automation.Host.HostException);
                }
            }
        }

        private static void CopyApplicationPackageToReservedFolder(string applicationPackagePath)
        {
            LogHelper.Log("CopyApplicationPackageToReservedFolder");
            var command = new Command(Constants.CopyApplicationPackageCommand);
            command.Parameters.Add("ApplicationPackagePath", applicationPackagePath);
            command.Parameters.Add("ImageStoreConnectionString", "file:" + deployment.ImageStoreRootPath);
            command.Parameters.Add("ApplicationPackagePathInImageStore", "store");
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();

            try
            {
                pipeline.Commands.Add(command);
                pipeline.Invoke();
                Assert.IsTrue(pipeline.PipelineStateInfo.State == PipelineState.Stopping);
            }
            catch (CmdletInvocationException exception)
            {
                Assert.IsTrue(exception.ErrorRecord.Exception is System.Management.Automation.Host.HostException);
            }
        }

        private static void RegisterApplicationType(string applicationPackagePath)
        {
            LogHelper.Log("RegisterApplicationType");
            var command = new Command(Constants.RegisterApplicationTypeCommand);
            command.Parameters.Add("ApplicationPathInImageStore", GetApplicationBuildPath(applicationPackagePath));
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static string GetApplicationBuildPath(string applicationPackagePath)
        {
            return new DirectoryInfo(applicationPackagePath).Name;
        }

        private static void NewApplication(Uri applicationName, string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("NewApplication");
            var command = new Command(Constants.NewApplicationCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            command.Parameters.Add("ApplicationTypeName", applicationTypeName);
            command.Parameters.Add("ApplicationTypeVersion", applicationTypeVersion);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void NewApplication(ApplicationDescription applicationDescription)
        {
            LogHelper.Log("NewApplication");
            var command = new Command(Constants.NewApplicationCommand);
            command.Parameters.Add("ApplicationName", applicationDescription.ApplicationName);
            command.Parameters.Add("ApplicationTypeName", applicationDescription.ApplicationTypeName);
            command.Parameters.Add("ApplicationTypeVersion", applicationDescription.ApplicationTypeVersion);
            command.Parameters.Add("MinimumNodes", applicationDescription.MinimumNodes);
            command.Parameters.Add("MaximumNodes", applicationDescription.MaximumNodes);
            if (applicationDescription.Metrics != null)
            {
                string[] metrics = new string[applicationDescription.Metrics.Count];
                for (int index = 0; index < applicationDescription.Metrics.Count; index++)
                {
                    metrics[index] = String.Format("{0},{1},{2},{3}",
                        applicationDescription.Metrics[index].Name,
                        applicationDescription.Metrics[index].NodeReservationCapacity,
                        applicationDescription.Metrics[index].MaximumNodeCapacity,
                        applicationDescription.Metrics[index].TotalApplicationCapacity);
                }
                command.Parameters.Add("Metrics", metrics);
            }
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void UpdateApplication(Uri applicationName,
            bool removeApplicationCapacity,
            long? minimumNodes,
            long? maximumNodes,
            IList<ApplicationMetricDescription> applicationMetrics)
        {
            LogHelper.Log("UpdateApplication");
            var command = new Command(Constants.UpdateApplicationCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            if (removeApplicationCapacity)
            {
                command.Parameters.Add("RemoveApplicationCapacity", true);
            }
            if (minimumNodes.HasValue)
            {
                command.Parameters.Add("MinimumNodes", minimumNodes.Value);
            }
            if (maximumNodes.HasValue)
            {
                command.Parameters.Add("MaximumNodes", maximumNodes.Value);
            }
            if (applicationMetrics != null)
            {
                string[] metrics = new string[applicationMetrics.Count];
                for (int index = 0; index < applicationMetrics.Count; index++)
                {
                    metrics[index] = String.Format("{0},{1},{2},{3}",
                        applicationMetrics[index].Name,
                        applicationMetrics[index].NodeReservationCapacity,
                        applicationMetrics[index].MaximumNodeCapacity,
                        applicationMetrics[index].TotalApplicationCapacity);
                }
                command.Parameters.Add("Metrics", metrics);
            }
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void UpdateApplication(Uri applicationName,
            IList<ApplicationMetricDescription> applicationMetrics)
        {
            LogHelper.Log("UpdateApplication");
            var command = new Command(Constants.UpdateApplicationCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            if (applicationMetrics != null)
            {
                string[] metrics = new string[applicationMetrics.Count];
                for (int index = 0; index < applicationMetrics.Count; index++)
                {
                    metrics[index] = String.Format("{0},NodeReservationCapacity:{1},TotalApplicationCapacity:{2},MaximumNodeCapacity:{3}",
                        applicationMetrics[index].Name,
                        applicationMetrics[index].NodeReservationCapacity,
                        applicationMetrics[index].TotalApplicationCapacity,
                        applicationMetrics[index].MaximumNodeCapacity);
                }
                command.Parameters.Add("Metrics", metrics);
            }
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void VerifyApplicationLoadBalancingProperties(
            Uri applicationName,
            long minimumNodes,
            long maximumNodes,
            IList<ApplicationMetricDescription> applicationCapacities)
        {
            LogHelper.Log("VerifyApplicationLoadBalancingProperties");
            var command = new Command(Constants.GetApplicationLoadInformationCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            Assert.IsNotNull(result);
            TestUtility.AssertAreEqual(1, result.Count);
            TestCommon.Log(result);

            var queryResult = result[0].ImmediateBaseObject as ApplicationLoadInformation;
            Assert.AreEqual(queryResult.Name, applicationName.ToString());
            Assert.AreEqual(queryResult.MinimumNodes, minimumNodes);
            Assert.AreEqual(queryResult.MaximumNodes, maximumNodes);
            if (applicationCapacities == null)
            {
                if (queryResult.ApplicationLoadMetricInformation != null)
                {
                    Assert.AreEqual(0, queryResult.ApplicationLoadMetricInformation.Count);
                }
            }
            else
            {
                Assert.IsNotNull(queryResult.ApplicationLoadMetricInformation);
                Assert.AreEqual(applicationCapacities.Count, queryResult.ApplicationLoadMetricInformation.Count);
                foreach (var appCapacity in applicationCapacities)
                {
                    bool found = false;
                    foreach (var loadInfomation in queryResult.ApplicationLoadMetricInformation)
                    {
                        if (appCapacity.Name == loadInfomation.Name)
                        {
                            // Do not check if values reported for load are correct.
                            // There are other TAEF, e2e and FabricTest tests that are checking that.
                            // Doing that here would make the test too complicated.
                            found = true;
                            break;
                        }
                    }
                    Assert.IsTrue(found);
                }
            }
        }

        private static void TestApplicationLoadBalancingParameters()
        {
            // Test only for application with stateful services
            //BuildTestApplicationWithPLBParamters();
            BuildStatefulServiceTestApplication(false);
            TestApplicationPackage(statefulServiceApplicationPackagePath);
            CopyApplicationPackage(statefulServiceApplicationPackagePath);
            RegisterApplicationType(statefulServiceApplicationPackagePath);
            VerifyApplicationTypeRegistered(statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version);


            Uri applicationName = new Uri(string.Format("fabric:/{0}", statefulServiceApplicationDeployer.Name));
            long minimumNodes = 2;
            long maximumNodes = 4;

            List<ApplicationMetricDescription> applicationCapacities = new List<ApplicationMetricDescription>();
            applicationCapacities.Add(new ApplicationMetricDescription() 
                { 
                    Name = "CPU",
                    NodeReservationCapacity = 8,
                    MaximumNodeCapacity = 16,
                    TotalApplicationCapacity = 64
                });
            applicationCapacities.Add(new ApplicationMetricDescription()
            {
                Name = "Memory",
                NodeReservationCapacity = 1024,
                MaximumNodeCapacity = 4096,
                TotalApplicationCapacity = 16384
            });
            ApplicationDescription newApplicationDescription = new ApplicationDescription(applicationName, statefulServiceApplicationDeployer.Name, statefulServiceApplicationDeployer.Version)
            {
                MinimumNodes = minimumNodes,
                MaximumNodes = maximumNodes,
                Metrics = applicationCapacities
            };

            // Create application and verify that it is created
            NewApplication(newApplicationDescription);
            VerifyApplicationCreated(newApplicationDescription.ApplicationName, newApplicationDescription.ApplicationTypeName, newApplicationDescription.ApplicationTypeVersion);

            // Verify that PLB parameters are passed correctly
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, applicationCapacities);

            // Update minimum nodes and check
            minimumNodes = 1;
            UpdateApplication(applicationName, false, minimumNodes, null, null);
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, applicationCapacities);

            // Update maximum nodes and check
            maximumNodes = 3;
            UpdateApplication(applicationName, false, null, maximumNodes, null);
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, applicationCapacities);

            // Update application capacities and check
            applicationCapacities.RemoveAt(0);
            UpdateApplication(applicationName, applicationCapacities);
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, applicationCapacities);

            // Clear application capacities, leaving just minimim and maximum nodes
            applicationCapacities.Clear();
            UpdateApplication(applicationName, applicationCapacities);
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, null);

            // Combo: update all together
            applicationCapacities.Add(new ApplicationMetricDescription()
            {
                Name = "Memory",
                NodeReservationCapacity = 1024,
                MaximumNodeCapacity = 4096,
                TotalApplicationCapacity = 16384
            });
            minimumNodes = 2;
            maximumNodes = 4;
            UpdateApplication(applicationName, false, minimumNodes, maximumNodes, applicationCapacities);
            VerifyApplicationLoadBalancingProperties(applicationName, minimumNodes, maximumNodes, applicationCapacities);

            // Remove PLB parameters
            UpdateApplication(applicationName, true, null, null, null);
            VerifyApplicationLoadBalancingProperties(applicationName, 0, 0, null);

            // Remove the application
            RemoveApplication(applicationName);
            VerifyApplicationDeleted(applicationName);
            UnregisterApplicationType(newApplicationDescription.ApplicationTypeName, newApplicationDescription.ApplicationTypeVersion);
            VerifyApplicationTypeUnRegistered(newApplicationDescription.ApplicationTypeName, newApplicationDescription.ApplicationTypeVersion);
        }

        private static void RemoveApplication(Uri applicationName)
        {
            LogHelper.Log("RemoveApplication");
            var command = new Command(Constants.RemoveApplicationCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            command.Parameters.Add("Force");
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void UnregisterApplicationType(string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("UnregisterApplicationType");
            var command = new Command(Constants.UnregisterApplicationTypeCommand);
            command.Parameters.Add("ApplicationTypeName", applicationTypeName);
            command.Parameters.Add("ApplicationTypeVersion", applicationTypeVersion);
            command.Parameters.Add("Force");
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private void UpgradeApplicationManual(Uri applicationName, ApplicationDeployer applicationDeployer)
        {
            LogHelper.Log("StartApplicationUpgrade");
            var command = new Command(Constants.StartApplicationUpgradeCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            command.Parameters.Add("ApplicationTypeVersion", applicationDeployer.Version);
            command.Parameters.Add("UnmonitoredManual");
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            while (true)
            {
                Thread.Sleep(TimeSpan.FromSeconds(10));

                command = new Command(Constants.GetApplicationUpgradeCommand);
                command.Parameters.Add("ApplicationName", applicationName);
                TestCommon.Log(command);
                result = InvokeWithRetry(command);
                TestCommon.Log(result);
                TestUtility.AssertAreEqual(1, result.Count);
                var progress = result[0].ImmediateBaseObject as ApplicationUpgradeProgress;
                Assert.IsNotNull(progress);
                TestUtility.AssertAreEqual(progress.ApplicationName, applicationName);
                TestUtility.AssertAreEqual(applicationDeployer.Name, progress.ApplicationTypeName);
                TestUtility.AssertAreEqual(applicationDeployer.Version, progress.TargetApplicationTypeVersion);

                if (progress.UpgradeState == ApplicationUpgradeState.RollingForwardCompleted)
                {
                    break;
                }

                if (progress.UpgradeState != ApplicationUpgradeState.RollingForwardPending)
                {
                    continue;
                }

                if (progress.NextUpgradeDomain == null)
                {
                    continue;
                }

                command = new Command(Constants.ResumeApplicationUpgradeCommand);
                command.Parameters.Add("ApplicationName", applicationName);
                command.Parameters.Add("UpgradeDomainName", progress.NextUpgradeDomain);
                TestCommon.Log(command);
                result = InvokeWithRetry(command);
                TestCommon.Log(result);
            }
        }

        private static void VerifyApplicationTypeRegistered(string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("VerifyApplicationTypeRegistered");
            var command = new Command(Constants.GetApplicationTypeCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as ApplicationType;
                Assert.IsNotNull(item);
                if (item.ApplicationTypeName == applicationTypeName && item.ApplicationTypeVersion == applicationTypeVersion)
                {
                    found = true;
                }
            }
            Assert.IsTrue(found);
        }

        private static void VerifyApplicationTypeUnRegistered(string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("VerifyApplicationTypeUnRegistered");
            var command = new Command(Constants.GetApplicationTypeCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as ApplicationType;
                Assert.IsNotNull(item);
                if (item.ApplicationTypeName == applicationTypeName && item.ApplicationTypeVersion == applicationTypeVersion)
                {
                    found = true;
                }
            }
            Assert.IsFalse(found);
        }

        private static void VerifyApplicationCreated(Uri applicationName, string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("VerifyApplicationCreated");
            var command = new Command(Constants.GetApplicationCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Application;
                Assert.IsNotNull(item);
                if (item.ApplicationName == applicationName)
                {
                    found = true;
                    TestUtility.AssertAreEqual(applicationTypeName, item.ApplicationTypeName);
                    TestUtility.AssertAreEqual(applicationTypeVersion, item.ApplicationTypeVersion);
                }
            }
            Assert.IsTrue(found);
        }

        private static void VerifyApplicationDeleted(Uri applicationName)
        {
            LogHelper.Log("VerifyApplicationDeleted");
            var command = new Command(Constants.GetApplicationCommand);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Application;
                Assert.IsNotNull(item);
                if (item.ApplicationName == applicationName)
                {
                    found = true;
                }
            }
            Assert.IsFalse(found);
        }

        private static void CreateService(ServiceDescription serviceDescription)
        {
            LogHelper.Log("CreateService");
            var command = new Command(Constants.NewServiceCommand);
            command.Parameters.Add("ApplicationName", serviceDescription.ApplicationName);
            command.Parameters.Add("ServiceName", serviceDescription.ServiceName);
            command.Parameters.Add("ServiceTypeName", serviceDescription.ServiceTypeName);
            if (serviceDescription is StatefulServiceDescription)
            {
                var statefulServiceDescription = serviceDescription as StatefulServiceDescription;
                command.Parameters.Add("Stateful");
                command.Parameters.Add("TargetReplicaSetSize", statefulServiceDescription.TargetReplicaSetSize);
                command.Parameters.Add("MinReplicaSetSize", statefulServiceDescription.MinReplicaSetSize);
                command.Parameters.Add("HasPersistedState", statefulServiceDescription.HasPersistedState);
            }
            else if (serviceDescription is StatelessServiceDescription)
            {
                var statelessServiceDescription = serviceDescription as StatelessServiceDescription;
                command.Parameters.Add("Stateless");
                command.Parameters.Add("InstanceCount", statelessServiceDescription.InstanceCount);
            }

            if (serviceDescription.IsDefaultMoveCostSpecified)
            {
                command.Parameters.Add("DefaultMoveCost", statefulServiceDescription.DefaultMoveCost);
            }

            if (serviceDescription.PartitionSchemeDescription is NamedPartitionSchemeDescription)
            {
                var partitionDescription = serviceDescription.PartitionSchemeDescription as NamedPartitionSchemeDescription;
                command.Parameters.Add("PartitionSchemeNamed");
                command.Parameters.Add("PartitionNames", partitionDescription.PartitionNames);
            }
            else if (serviceDescription.PartitionSchemeDescription is UniformInt64RangePartitionSchemeDescription)
            {
                var partitionDescription = serviceDescription.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                command.Parameters.Add("PartitionSchemeUniformInt64");
                command.Parameters.Add("HighKey", partitionDescription.HighKey);
                command.Parameters.Add("LowKey", partitionDescription.LowKey);
            }
            else if (serviceDescription.PartitionSchemeDescription is SingletonPartitionSchemeDescription)
            {
                command.Parameters.Add("PartitionSchemeSingleton");
            }

            if (serviceDescription.ScalingPolicies != null)
            {
                command.Parameters.Add("ScalingPolicies", serviceDescription.ScalingPolicies);
            }
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void CreateServiceGroup(ServiceGroupDescription serviceGroupDescription)
        {
            LogHelper.Log("CreateServiceGroup");
            var command = new Command(Constants.NewServiceGroupCommand);
            command.Parameters.Add("ApplicationName", serviceGroupDescription.ServiceDescription.ApplicationName);
            command.Parameters.Add("ServiceName", serviceGroupDescription.ServiceDescription.ServiceName);
            command.Parameters.Add("ServiceTypeName", serviceGroupDescription.ServiceDescription.ServiceTypeName);
            if (serviceGroupDescription.ServiceDescription is StatefulServiceDescription)
            {
                var statefulServiceDescription = serviceGroupDescription.ServiceDescription as StatefulServiceDescription;
                command.Parameters.Add("Stateful");
                command.Parameters.Add("TargetReplicaSetSize", statefulServiceDescription.TargetReplicaSetSize);
                command.Parameters.Add("MinReplicaSetSize", statefulServiceDescription.MinReplicaSetSize);
                command.Parameters.Add("HasPersistedState", statefulServiceDescription.HasPersistedState);

            }
            else if (serviceGroupDescription.ServiceDescription is StatelessServiceDescription)
            {
                var statelessServiceDescription = serviceGroupDescription.ServiceDescription as StatelessServiceDescription;
                command.Parameters.Add("Stateless");
                command.Parameters.Add("InstanceCount", statelessServiceDescription.InstanceCount);
            }

            if (serviceGroupDescription.ServiceDescription.PartitionSchemeDescription is NamedPartitionSchemeDescription)
            {
                var partitionDescription = serviceGroupDescription.ServiceDescription.PartitionSchemeDescription as NamedPartitionSchemeDescription;
                command.Parameters.Add("PartitionSchemeNamed");
                command.Parameters.Add("PartitionNames", partitionDescription.PartitionNames);
            }
            else if (serviceGroupDescription.ServiceDescription.PartitionSchemeDescription is UniformInt64RangePartitionSchemeDescription)
            {
                var partitionDescription = serviceGroupDescription.ServiceDescription.PartitionSchemeDescription as UniformInt64RangePartitionSchemeDescription;
                command.Parameters.Add("PartitionSchemeUniformInt64");
                command.Parameters.Add("HighKey", partitionDescription.HighKey);
                command.Parameters.Add("LowKey", partitionDescription.LowKey);
            }
            else if (serviceGroupDescription.ServiceDescription.PartitionSchemeDescription is SingletonPartitionSchemeDescription)
            {
                command.Parameters.Add("PartitionSchemeSingleton");
            }
            
            command.Parameters.Add("ServiceGroupMemberDescription", ParseServiceGroupMemberDescription(serviceGroupDescription.MemberDescriptions));

            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static Hashtable[] ParseServiceGroupMemberDescription(IList<ServiceGroupMemberDescription> memberDescription)
        {
            Hashtable[] memberHashArray = new Hashtable[memberDescription.Count];
            for (int i = 0; i < memberDescription.Count; i++)
            {
                memberHashArray[i] = new Hashtable();
                memberHashArray[i].Add("ServiceName", memberDescription[i].ServiceName.ToString());
                memberHashArray[i].Add("ServiceTypeName", memberDescription[i].ServiceTypeName);
            }

            return memberHashArray;
        }

        private static void VerifyServiceCreated(ServiceDescription serviceDescription)
        {
            LogHelper.Log("VerifyServiceCreated");
            var command = new Command(Constants.GetServiceCommand);
            command.Parameters.Add("ApplicationName", serviceDescription.ApplicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Service;
                if (item.ServiceName == serviceDescription.ServiceName)
                {
                    found = true;

                    // Get the description to verify that service has correct parameters
                    var getDescriptionCommand = new Command(Constants.GetServiceDescriptionCommand);
                    getDescriptionCommand.Parameters.Add("ServiceName", serviceDescription.ServiceName);
                    TestCommon.Log(getDescriptionCommand);
                    var getDescriptionResult = InvokeWithRetry(getDescriptionCommand);
                    TestCommon.Log(getDescriptionResult);
                    Assert.IsNotNull(getDescriptionResult);
                    TestUtility.AssertAreEqual(1, getDescriptionResult.Count);

                    if (serviceDescription is StatefulServiceDescription)
                    {
                        var expect = serviceDescription as StatefulServiceDescription;
                        var actual = result[i].ImmediateBaseObject as StatefulService;
                        var actualDescription = getDescriptionResult[0].ImmediateBaseObject as StatefulServiceDescription;
                        TestUtility.AssertAreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
                        TestUtility.AssertAreEqual(ServiceKind.Stateful, actual.ServiceKind);
                        TestUtility.AssertAreEqual(expect.HasPersistedState, actual.HasPersistedState);
                        TestUtility.AssertAreEqual(expect.TargetReplicaSetSize, actualDescription.TargetReplicaSetSize);
                        TestUtility.AssertAreEqual(expect.MinReplicaSetSize, actualDescription.MinReplicaSetSize);
                        if (expect.IsDefaultMoveCostSpecified)
                        {
                            TestUtility.AssertAreEqual(expect.DefaultMoveCost, actualDescription.DefaultMoveCost);
                        }
                        if (expect.ScalingPolicies != null)
                        {
                            CheckScalingPolicies(expect.ScalingPolicies, actualDescription.ScalingPolicies);
                        }
                    }
                    else if (serviceDescription is StatelessServiceDescription)
                    {
                        var expect = serviceDescription as StatelessServiceDescription;
                        var actual = result[i].ImmediateBaseObject as StatelessService;
                        var actualDescription = getDescriptionResult[0].ImmediateBaseObject as StatelessServiceDescription;
                        TestUtility.AssertAreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
                        TestUtility.AssertAreEqual(ServiceKind.Stateless, actual.ServiceKind);
                        TestUtility.AssertAreEqual(expect.InstanceCount, actualDescription.InstanceCount);
                        if (expect.IsDefaultMoveCostSpecified)
                        {
                            TestUtility.AssertAreEqual(expect.DefaultMoveCost, actualDescription.DefaultMoveCost);
                        }
                        if (expect.ScalingPolicies != null)
                        {
                            CheckScalingPolicies(expect.ScalingPolicies, actualDescription.ScalingPolicies);
                        }
                    }
                }
            }

            Assert.IsTrue(found);
        }

        private static void VerifyServiceGroupCreated(ServiceGroupDescription serviceGroupDescription, string applicationTypeName, string applicationTypeVersion)
        {
            LogHelper.Log("VerifyServiceCreated");
            var command = new Command(Constants.GetServiceCommand);
            command.Parameters.Add("ApplicationName", serviceGroupDescription.ServiceDescription.ApplicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Service;
                if (item.ServiceName == serviceGroupDescription.ServiceDescription.ServiceName)
                {
                    found = true;
                    if (serviceGroupDescription.ServiceDescription is StatefulServiceDescription)
                    {
                        var expect = serviceGroupDescription.ServiceDescription as StatefulServiceDescription;
                        var actual = result[i].ImmediateBaseObject as StatefulService;
                        TestUtility.AssertAreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
                        TestUtility.AssertAreEqual(ServiceKind.Stateful, actual.ServiceKind);
                        TestUtility.AssertAreEqual(expect.HasPersistedState, actual.HasPersistedState);
                    }
                    else if (serviceGroupDescription.ServiceDescription is StatelessServiceDescription)
                    {
                        var expect = serviceGroupDescription.ServiceDescription as StatelessServiceDescription;
                        var actual = result[i].ImmediateBaseObject as StatelessService;
                        TestUtility.AssertAreEqual(expect.ServiceTypeName, actual.ServiceTypeName);
                        TestUtility.AssertAreEqual(ServiceKind.Stateless, actual.ServiceKind);
                    }
                }
            }

            Assert.IsTrue(found);

            command = new Command(Constants.GetServiceGroupMemberCommand);
            command.Parameters.Add("ApplicationName", serviceGroupDescription.ServiceDescription.ApplicationName);
            command.Parameters.Add("ServiceName", serviceGroupDescription.ServiceDescription.ServiceName);
            TestCommon.Log(command);
            result = InvokeWithRetry(command);
            TestCommon.Log(result);

            found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as ServiceGroupMember;
                if (item.ServiceName == serviceGroupDescription.ServiceDescription.ServiceName)
                {
                    found = true;

                    Assert.AreEqual(serviceGroupDescription.MemberDescriptions.Count, item.ServiceGroupMemberMembers.Count);

                    for(int j = 0; j < serviceGroupDescription.MemberDescriptions.Count; j++)
                    {
                        Assert.AreEqual(serviceGroupDescription.MemberDescriptions[j].ServiceName, item.ServiceGroupMemberMembers[j].ServiceName);
                        Assert.AreEqual(serviceGroupDescription.MemberDescriptions[j].ServiceTypeName, item.ServiceGroupMemberMembers[j].ServiceTypeName);
                    }
                }
            }

            Assert.IsTrue(found);

            command = new Command(Constants.GetServiceGroupMemberTypeCommand);
            command.Parameters.Add("ApplicationTypeName", applicationTypeName);
            command.Parameters.Add("ApplicationTypeVersion", applicationTypeVersion);
            TestCommon.Log(command);
            result = InvokeWithRetry(command);
            TestCommon.Log(result);

            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as ServiceGroupMemberType;

                Assert.AreNotEqual(0, item.ServiceGroupMemberTypeDescription.Count);
                for (int j = 0; j < item.ServiceGroupMemberTypeDescription.Count; j++)
                {
                    var serviceGroupMemberTypeDescriptionList = item.ServiceGroupMemberTypeDescription.ToList();
                    Assert.AreEqual(serviceGroupDescription.ServiceDescription.ServiceTypeName, serviceGroupMemberTypeDescriptionList[j].ServiceTypeName);
                }
            }
        }

        private static void NewServiceFromTemplate(Uri applicationName, Uri serviceName, string serviceTypeName)
        {
            LogHelper.Log("NewServiceFromTemplate");

            var command = new Command(Constants.NewServiceFromTemplateCommand);
            command.Parameters.Add("ApplicationName", applicationName);
            command.Parameters.Add("ServiceName", serviceName);
            command.Parameters.Add("ServiceTypeName", serviceTypeName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void UpdateService(Uri serviceName, ServiceUpdateDescription serviceUpdateDescription)
        {
            LogHelper.Log("UpdateService");

            var command = new Command(Constants.UpdateServiceCommand);
            command.Parameters.Add("ServiceName", serviceName);

            if (serviceUpdateDescription.Kind == ServiceDescriptionKind.Stateful)
            {
                var statefulServiceUpdateDescription = serviceUpdateDescription as StatefulServiceUpdateDescription;
                command.Parameters.Add("Stateful");

                if (statefulServiceUpdateDescription.TargetReplicaSetSize.HasValue)
                {
                    command.Parameters.Add("TargetReplicaSetSize", statefulServiceUpdateDescription.TargetReplicaSetSize.Value);
                }

                if (statefulServiceUpdateDescription.MinReplicaSetSize.HasValue)
                {
                    command.Parameters.Add("MinReplicaSetSize", statefulServiceUpdateDescription.MinReplicaSetSize.Value);
                }

                if (statefulServiceUpdateDescription.ReplicaRestartWaitDuration.HasValue)
                {
                    command.Parameters.Add("ReplicaRestartWaitDuration", statefulServiceUpdateDescription.ReplicaRestartWaitDuration.Value);
                }

                if (statefulServiceUpdateDescription.QuorumLossWaitDuration.HasValue)
                {
                    command.Parameters.Add("QuorumLossWaitDuration", statefulServiceUpdateDescription.QuorumLossWaitDuration.Value);
                }

                if (statefulServiceUpdateDescription.DefaultMoveCost.HasValue)
                {
                    command.Parameters.Add("DefaultMoveCost", statefulServiceUpdateDescription.DefaultMoveCost.Value);
                }

                if (statefulServiceUpdateDescription.Correlations != null)
                {
                    command.Parameters.Add("Correlation", statefulServiceUpdateDescription.Correlations);
                }

                if (statefulServiceUpdateDescription.PlacementConstraints != null)
                {
                    command.Parameters.Add("PlacementConstraints", statefulServiceUpdateDescription.PlacementConstraints);
                }

                if (statefulServiceUpdateDescription.PlacementPolicies != null)
                {
                    command.Parameters.Add("PlacementPolicy", statefulServiceUpdateDescription.PlacementPolicies);
                }
                if (statefulServiceUpdateDescription.ScalingPolicies != null)
                {
                    command.Parameters.Add("ScalingPolicies", statefulServiceUpdateDescription.ScalingPolicies);
                }
            }
            else if (serviceUpdateDescription.Kind == ServiceDescriptionKind.Stateless)
            {
                var statelessServiceUpdateDescription = serviceUpdateDescription as StatelessServiceUpdateDescription;
                command.Parameters.Add("Stateless");

                if (statelessServiceUpdateDescription.InstanceCount.HasValue)
                {
                    command.Parameters.Add("InstanceCount", statelessServiceUpdateDescription.InstanceCount.Value);
                }

                if (statelessServiceUpdateDescription.DefaultMoveCost.HasValue)
                {
                    command.Parameters.Add("DefaultMoveCost", statelessServiceUpdateDescription.DefaultMoveCost.Value);
                }

                if (statelessServiceUpdateDescription.Correlations != null)
                {
                    command.Parameters.Add("Correlation", statelessServiceUpdateDescription.Correlations);
                }

                if (statelessServiceUpdateDescription.PlacementConstraints != null)
                {
                    command.Parameters.Add("PlacementConstraints", statelessServiceUpdateDescription.PlacementConstraints);
                }

                if (statelessServiceUpdateDescription.PlacementPolicies != null)
                {
                    command.Parameters.Add("PlacementPolicy", statelessServiceUpdateDescription.PlacementPolicies);
                }

                if (statelessServiceUpdateDescription.ScalingPolicies != null)
                {
                    command.Parameters.Add("ScalingPolicies", statelessServiceUpdateDescription.ScalingPolicies);
                }
            }

            command.Parameters.Add("Force");

            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void VerifyServiceUpdated(Uri serviceName, ServiceUpdateDescription serviceUpdateDescription)
        {
            LogHelper.Log("VerifyServiceUpdated");
            var command = new Command(Constants.GetServiceDescriptionCommand);
            command.Parameters.Add("ServiceName", serviceName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            Assert.IsNotNull(result);
            TestUtility.AssertAreEqual(1, result.Count);
            var serviceDescription = result[0].ImmediateBaseObject as ServiceDescription;
            TestUtility.AssertAreEqual(serviceName, serviceDescription.ServiceName);

            if (serviceDescription is StatefulServiceDescription)
            {
                var expect = serviceUpdateDescription as StatefulServiceUpdateDescription;
                var actual = serviceDescription as StatefulServiceDescription;
                if (expect.TargetReplicaSetSize.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.TargetReplicaSetSize.Value, actual.TargetReplicaSetSize);
                }

                if (expect.MinReplicaSetSize.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.MinReplicaSetSize.Value, actual.MinReplicaSetSize);
                }

                if (expect.ReplicaRestartWaitDuration.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.ReplicaRestartWaitDuration.Value, actual.ReplicaRestartWaitDuration);
                }

                if (expect.QuorumLossWaitDuration.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.QuorumLossWaitDuration.Value, actual.QuorumLossWaitDuration);
                }

                if (expect.DefaultMoveCost.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.DefaultMoveCost, actual.DefaultMoveCost);
                }

                TestUtility.AssertAreEqual(expect.Correlations.Count, actual.Correlations.Count);
                TestUtility.AssertAreEqual(expect.PlacementConstraints, actual.PlacementConstraints);
                TestUtility.AssertAreEqual(expect.PlacementPolicies.Count, actual.PlacementPolicies.Count);
                if (expect.ScalingPolicies != null || actual.ScalingPolicies != null)
                {
                    CheckScalingPolicies(expect.ScalingPolicies, actual.ScalingPolicies);
                }


            }
            else if (serviceDescription is StatelessServiceDescription)
            {
                var expect = serviceUpdateDescription as StatelessServiceUpdateDescription;
                var actual = serviceDescription as StatelessServiceDescription;
                TestUtility.AssertAreEqual(expect.InstanceCount.Value, actual.InstanceCount);
                if (expect.DefaultMoveCost.HasValue)
                {
                    TestUtility.AssertAreEqual(expect.DefaultMoveCost, actual.DefaultMoveCost);
                }

                TestUtility.AssertAreEqual(expect.Correlations.Count, actual.Correlations.Count);
                TestUtility.AssertAreEqual(expect.PlacementConstraints, actual.PlacementConstraints);
                TestUtility.AssertAreEqual(expect.PlacementPolicies.Count, actual.PlacementPolicies.Count);
                if (expect.ScalingPolicies != null || actual.ScalingPolicies != null)
                {
                    CheckScalingPolicies(expect.ScalingPolicies, actual.ScalingPolicies);
                }

            }
        }

        private static void RemoveService(Uri serviceName)
        {
            LogHelper.Log("RemoveService");
            var command = new Command(Constants.RemoveServiceCommand);
            command.Parameters.Add("ServiceName", serviceName);
            command.Parameters.Add("Force");

            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void RemoveServiceGroup(Uri serviceName)
        {
            LogHelper.Log("RemoveServiceGroup");
            var command = new Command(Constants.RemoveServiceGroupCommand);
            command.Parameters.Add("ServiceName", serviceName);
            command.Parameters.Add("Force");

            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
        }

        private static void VerifyServiceDeleted(ServiceDescription serviceDescription)
        {
            LogHelper.Log("VerifyServiceDeleted");
            var command = new Command(Constants.GetServiceCommand);
            command.Parameters.Add("ApplicationName", serviceDescription.ApplicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Service;
                if (item.ServiceName == serviceDescription.ServiceName)
                {
                    if (item.ServiceStatus != ServiceStatus.Deleting)
                    {
                        found = true;
                        break;
                    }
                }
            }

            Assert.IsFalse(found);
        }

        private static void VerifyServiceGroupDeleted(ServiceGroupDescription serviceGroupDescription)
        {
            LogHelper.Log("VerifyServiceDeleted");
            var command = new Command(Constants.GetServiceCommand);
            command.Parameters.Add("ApplicationName", serviceGroupDescription.ServiceDescription.ApplicationName);
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);

            var found = false;
            for (int i = 0; i < result.Count; i++)
            {
                var item = result[i].ImmediateBaseObject as Service;
                if (item.ServiceName == serviceGroupDescription.ServiceDescription.ServiceName)
                {
                    if (item.ServiceStatus != ServiceStatus.Deleting)
                    {
                        found = true;
                        break;
                    }
                }
            }

            Assert.IsFalse(found);
        }

        private static void CheckScalingPolicies(IList<ScalingPolicyDescription> expected, IList<ScalingPolicyDescription> actual)
        {
            Assert.AreEqual(expected.Count, actual.Count);

            for (int i = 0; i < expected.Count; ++i)
            {
                CheckScalingPolicy(expected[i], actual[i]);
            }
        }

        private static void CheckScalingPolicy(ScalingPolicyDescription expected, ScalingPolicyDescription actual)
        {
            CheckScalingMechanism(expected.ScalingMechanism, actual.ScalingMechanism);
            CheckScalingTrigger(expected.ScalingTrigger, actual.ScalingTrigger);
        }

        public static void CheckScalingMechanism(ScalingMechanismDescription expect, ScalingMechanismDescription actual)
        {
            if (expect.Kind == ScalingMechanismKind.ScalePartitionInstanceCount)
            {
                Assert.AreEqual(expect.Kind, actual.Kind);
                PartitionInstanceCountScaleMechanism expectDerived = expect as PartitionInstanceCountScaleMechanism;
                PartitionInstanceCountScaleMechanism actualDerived = actual as PartitionInstanceCountScaleMechanism;

                Assert.AreEqual(expectDerived.MaxInstanceCount, actualDerived.MaxInstanceCount);
                Assert.AreEqual(expectDerived.MinInstanceCount, actualDerived.MinInstanceCount);
                Assert.AreEqual(expectDerived.ScaleIncrement, actualDerived.ScaleIncrement);
            }
            else if (expect.Kind == ScalingMechanismKind.AddRemoveIncrementalNamedPartition)
            {
                Assert.AreEqual(expect.Kind, actual.Kind);
                AddRemoveIncrementalNamedPartitionScalingMechanism expectDerived = expect as AddRemoveIncrementalNamedPartitionScalingMechanism;
                AddRemoveIncrementalNamedPartitionScalingMechanism actualDerived = actual as AddRemoveIncrementalNamedPartitionScalingMechanism;

                Assert.AreEqual(expectDerived.MaxPartitionCount, actualDerived.MaxPartitionCount);
                Assert.AreEqual(expectDerived.MinPartitionCount, actualDerived.MinPartitionCount);
                Assert.AreEqual(expectDerived.ScaleIncrement, actualDerived.ScaleIncrement);
            }
            else
            {
                Assert.Fail("Invalid mechanism " + expect.Kind);
            }
        }

        public static void CheckScalingTrigger(ScalingTriggerDescription expect, ScalingTriggerDescription actual)
        {
            if (expect.Kind == ScalingTriggerKind.AveragePartitionLoadTrigger)
            {
                Assert.AreEqual(expect.Kind, actual.Kind);
                AveragePartitionLoadScalingTrigger expectDerived = expect as AveragePartitionLoadScalingTrigger;
                AveragePartitionLoadScalingTrigger actualDerived = actual as AveragePartitionLoadScalingTrigger;

                Assert.AreEqual(expectDerived.LowerLoadThreshold, actualDerived.LowerLoadThreshold);
                Assert.AreEqual(expectDerived.UpperLoadThreshold, actualDerived.UpperLoadThreshold);
                Assert.AreEqual(expectDerived.MetricName, actualDerived.MetricName);
                Assert.AreEqual(expectDerived.ScaleInterval, actualDerived.ScaleInterval);
            }
            else if (expect.Kind == ScalingTriggerKind.AverageServiceLoadTrigger)
            {
                Assert.AreEqual(expect.Kind, actual.Kind);
                AverageServiceLoadScalingTrigger expectDerived = expect as AverageServiceLoadScalingTrigger;
                AverageServiceLoadScalingTrigger actualDerived = actual as AverageServiceLoadScalingTrigger;

                Assert.AreEqual(expectDerived.LowerLoadThreshold, actualDerived.LowerLoadThreshold);
                Assert.AreEqual(expectDerived.UpperLoadThreshold, actualDerived.UpperLoadThreshold);
                Assert.AreEqual(expectDerived.MetricName, actualDerived.MetricName);
                Assert.AreEqual(expectDerived.ScaleInterval, actualDerived.ScaleInterval);
                Assert.AreEqual(expectDerived.UseOnlyPrimaryLoad, actualDerived.UseOnlyPrimaryLoad);
            }
            else
            {
                Assert.Fail("Invalid trigger " + expect.Kind);
            }
        }

        private void RemoveNodeState()
        {
            LogHelper.Log("RemoveNodeState");
            var nodeName = deployment.Nodes.Last().NodeName;
            deployment.Nodes.Last().Dispose();
            Thread.Sleep(TimeSpan.FromSeconds(10));

            var command = new Command(Constants.RemoveNodeStateCommand);
            command.Parameters.Add("NodeName", nodeName);
            command.Parameters.Add("Force");
            TestCommon.Log(command);
            var result = InvokeWithRetry(command);
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
        }

        private static Collection<PSObject> InvokeHealthCommandWithRetry(
            Command command,
            bool retryOnEntityNotFound = true,
            int maxRetryCount = Constants.MaxRetryCount,
            double retryTimeoutInSeconds = Constants.RetryIntervalInSeconds)
        {
            if (command == null)
            {
                throw new ArgumentNullException("command");
            }

            int retryCount = maxRetryCount;
            do
            {
                try
                {
                    return InvokeWithRetry(command, maxRetryCount, retryTimeoutInSeconds);
                }
                catch (FabricException ex)
                {
                    bool retry = false;
                    if (retryOnEntityNotFound)
                    {
                        if (ex.ErrorCode == FabricErrorCode.FabricHealthEntityNotFound)
                        {
                            LogHelper.Log("HealthEntityNotFound, Retrying...");
                            Thread.Sleep(TimeSpan.FromSeconds(retryTimeoutInSeconds));
                            retry = true;
                        }
                    }

                    if (!retry)
                    {
                        throw;
                    }
                }
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in InvokeHealthCommandWithRetry");
        }

        private static Collection<PSObject> InvokeWithRetry(Command command,
             int maxRetryCount = Constants.MaxRetryCount,
             double retryTimeoutInSeconds = Constants.RetryIntervalInSeconds)
        {
            if (command == null)
            {
                throw new ArgumentNullException("command");
            }

            int retryCount = maxRetryCount;
            do
            {
                try
                {
                    using (Pipeline pipeline = runspace.CreatePipeline())
                    {
                        pipeline.Commands.Add(command);
                        return pipeline.Invoke();
                    }
                }
                catch (CmdletInvocationException aex)
                {
                    if (aex.ErrorRecord.Exception is OperationCanceledException ||
                        aex.ErrorRecord.Exception is TimeoutException)
                    {
                        LogHelper.Log("Invoke() failed with a retryable exception, Retrying...");
                        Thread.Sleep(TimeSpan.FromSeconds(retryTimeoutInSeconds));
                    }
                    else
                    {
                        throw aex.InnerException;
                    }
                }
            } while (retryCount-- > 0);

            throw new Exception("All Retry Counts exhausted in InvokeWithRetry");
        }
    }
}