// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Services.Scenarios.Store.Test
{
    using System;
    using System.IO;
    using Collections.Generic;
    using Description;
    using Fabric.Test;
    using Fabric.Test.Common.FabricUtility;
    using Reflection;
    using Threading;
    using Globalization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    [TestClass]
    public class FabricStoreTest
    {
        public static int PrefixItemCount = 10;
        public static int PrefixEnumeratorDisposeDelayInSeconds = 5;

        private static ApplicationDeployer CreateDeployer(string outputFolderPath)
        {
            var sd = new ServiceDeployer
            {
                CodePackageFiles = new List<string> { Assembly.GetExecutingAssembly().Location },
                ServiceTypeImplementation = typeof(StoreTestService),
                ServiceTypeName = Constants.ServiceTypeName,
                MinReplicaSetSize = 2,
                TargetReplicaSetSize = 2,
                Partition = new SingletonPartitionSchemeDescription(),
                ConfigPackageName = Constants.ConfigurationPackageName,   
                HasPersistedState = true,
            };

            sd.Endpoints.Add(Tuple.Create(Constants.EndpointName, EndpointProtocol.Http));

            sd.ConfigurationSettings.Add(Tuple.Create(Constants.ConfigurationSectionName, Constants.ConfigurationSettingName, outputFolderPath));

            ApplicationDeployer dep = new ApplicationDeployer
            {
                Name = Constants.ApplicationName,
                Services =
                {
                    sd
                }
            };

            return dep;
        }

        [TestMethod]
        [Owner("alexwun")]
        public void FabricStoreBasicTest()
        {
            using (var deployment = StartApp())
            {
                // basic testing of the APIs.
                LogHelper.Log("TestSettings");
                string resp = deployment.SendPostRequest(StoreTestService.CommandTestSettings);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestEnsurePrimary");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestEnsurePrimary);
                Assert.AreEqual("PASSED", resp);

                // TODO: Not supported by V2 stack
                /*
                LogHelper.Log("TestLastModifiedOnPrimary");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestLastModifiedOnPrimary);
                Assert.AreEqual("PASSED", resp);
                */

                LogHelper.Log("TestCommitNotCancellable");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestCommitNotCancellable);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestZeroLengthValue");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestZeroLengthValue);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestKeyNotFound");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestKeyNotFound);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestContains");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestContains);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestOutOfBoundReplicatorSettings");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestOutofBoundReplicatorSettings);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestUpdateReplicatorSettings1");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestUpdateReplicatorSettings1);
                Assert.AreEqual("PASSED", resp);

                LogHelper.Log("TestUpdateReplicatorSettings2");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestUpdateReplicatorSettings2);
                Assert.AreEqual("PASSED", resp);

                // send a request to update the primary 
                // the primary will replicate to the secondary
                //
                int operationCount = FabricStoreTest.PrefixItemCount;
                for (var ix=0; ix<operationCount; ++ix)
                {
                    resp = deployment.SendPostRequest();
                }

                // the value in the primary should now be "<operationCount>" because <operationCount> requests have gone in
                Assert.AreEqual(operationCount.ToString(), resp);

                // send a request to the primary and check the current state
                LogHelper.Log("VerifyTestData");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestVerifyTestData);
                Assert.AreEqual(operationCount.ToString(), resp);

                // restart primary
                LogHelper.Log("Restarting primary");
                deployment.SendPostRequest("ReportFault");

                DateTime startTimeUtc = DateTime.UtcNow;

                LogHelper.Log("Wait for stabilization");
                if (!deployment.WaitForPartitionToBeStable(Constants.ApplicationName, Constants.ServiceTypeName, startTimeUtc, TimeSpan.FromSeconds(120 * 5), (p) => true))
                {
                    LogHelper.Log("Failed waiting for partitions to be assigned");
                    Assert.Fail("Failed wait for partitions to be assigned");
                }

                // Verify if the data is still available
                LogHelper.Log("VerifyTestData");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestVerifyTestData);
                Assert.AreEqual(operationCount.ToString(), resp);

                LogHelper.Log("TestAddOrUpdate");
                resp = deployment.SendPostRequest(StoreTestService.CommandTestAddOrUpdate);
                Assert.AreEqual("PASSED", resp);

                // Uncomment to manually run perf test
                //
                //LogHelper.Log("TryUpdatePerf");
                //resp = deployment.SendPostRequest(StoreTestService.CommandTestTryUpdatePerf);
                //Assert.AreEqual("PASSED", resp);

                DeleteApp(deployment);
            }
        }

        // TODO: Backup/Restore not supported by V2 stack
        //[TestMethod]
        [Owner("alexwun;anupv")]
        public void FabricStoreBackupTest1()
        {
            using (var deployment = StartApp())
            {
                #region basic full and incremental backup

                int operationCount = 0;
                operationCount = AddTestData(deployment, operationCount, PrefixItemCount);

                string resp = deployment.SendPostRequest(
                    StoreTestService.CommandSetTrueReturnFromPostBackupHandler);
                Assert.AreEqual("PASSED", resp);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncFull);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncIncremental);
                Assert.AreEqual("PASSED", resp);

                #endregion basic full and incremental backup

                #region return false from post backup handler, next incremental backup should fail, truncate logs should succeed

                resp = deployment.SendPostRequest(
                    StoreTestService.CommandSetFalseReturnFromPostBackupHandler);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncFull);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                // pass if incremental backup fails with a FabricMissingFullBackupException.
                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncIncrementalAfterFalseReturn);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);
                
                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncTruncateLogsOnly);
                Assert.AreEqual("PASSED", resp);

                #endregion

                DeleteApp(deployment);
            }
        }

        // TODO: Backup/Restore not supported by V2 stack
        //[TestMethod]
        [Owner("alexwun;anupv")]
        public void FabricStoreBackupTest2()
        {
            using (var deployment = StartApp())
            {
                int operationCount = 0;
                operationCount = AddTestData(deployment, operationCount, PrefixItemCount);

                string resp = deployment.SendPostRequest(
                    StoreTestService.CommandSetTrueReturnFromPostBackupHandler);
                Assert.AreEqual("PASSED", resp);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncFull);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncIncremental);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncTruncateLogsOnly);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                // the test is considered as passed if incremental backup fails with a FabricMissingFullBackupException.                
                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncIncrementalAfterTruncateLogsOnly);
                Verify.AreEqual("PASSED", resp, "Verifying missing full backup exception");

                resp = deployment.SendPostRequest(StoreTestService.CommandSetTrueReturnFromPostBackupHandler);
                Assert.AreEqual("PASSED", resp);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupDirectoryNotEmptyException);
                Verify.AreEqual("PASSED", resp, "Verifying backup directory not empty exception");

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncFull);
                Assert.AreEqual("PASSED", resp);

                operationCount = AddTestData(deployment, operationCount, 1);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupAsyncIncremental);
                Assert.AreEqual("PASSED", resp);

                resp = deployment.SendPostRequest(StoreTestService.CommandTestBackupInProgressException);
                Verify.AreEqual("PASSED", resp, "Verifying backup in progress exception");

                DeleteApp(deployment);
            }
        }

        private static FabricDeployment StartApp()
        {
            var nodeCount = 3;
            var parameters = FabricDeploymentParameters.CreateDefault(
                System.Fabric.Services.Scenarios.Test.Constants.TestDllName,
                "FabricStore",
                nodeCount,
                true);

            LogHelper.Log("Creating deployment");
            var deployment = new FabricDeployment(parameters);

            for (var ix=1; ix<=nodeCount; ++ix)
            {
                var logDir = Path.Combine(deployment.LogPath, "Node" + ix);

                if (!Directory.Exists(logDir))
                {
                    LogHelper.Log("Creating directory: {0}", logDir);
                    Directory.CreateDirectory(logDir);
                }
            }

            LogHelper.Log("Starting Deployment");
            deployment.Start();

            DateTime startTimeUtc = DateTime.UtcNow;

            LogHelper.Log("Creating, packing and uploading app");
            var dep = CreateDeployer(deployment.TestOutputPath);
            deployment.UploadProvisionAndStartApplication(dep);

            LogHelper.Log("Wait for secondaries");
            if (!deployment.WaitForPartitionToBeStable(Constants.ApplicationName, Constants.ServiceTypeName, startTimeUtc, TimeSpan.FromSeconds(120 * 5), (p) => true))
            {
                LogHelper.Log("Failed waiting for partitions to be assigned");
                Assert.Fail("Failed wait for partitions to be assigned");
            }

            return deployment;
        }

        private static void DeleteApp(FabricDeployment deployment)
        {
            LogHelper.Log("Waiting to dispose prefix enumerators...");
            Thread.Sleep(TimeSpan.FromSeconds(PrefixEnumeratorDisposeDelayInSeconds * 2));

            LogHelper.Log("Deleting app");
            deployment.DeleteApplication(Constants.ApplicationName);
        }

        private static int AddTestData(FabricDeployment deployment, int currentItemCount, int itemsToAdd)
        {
            string resp = deployment.SendGetRequest();
            currentItemCount = int.Parse(resp);

            int newCount = currentItemCount;

            for (int i = 0; i < itemsToAdd; i++, newCount++)
            {
                resp = deployment.SendPostRequest(i.ToString(CultureInfo.InvariantCulture));
            }

            Assert.AreEqual(
                newCount.ToString(CultureInfo.InvariantCulture),
                resp,
                string.Format(CultureInfo.CurrentCulture, "Operation count = {0}, response = {1}", newCount, resp));

            resp = deployment.SendGetRequest();
            Assert.AreEqual(newCount.ToString(CultureInfo.InvariantCulture), resp);

            return newCount;
        }
    }

    class Constants
    {
        public const string EndpointName = "StoreServiceEndpoint";
        public const string ServiceTypeName = "store";
        public const string ApplicationName = "store_app";
        public const string ConfigurationSectionName = "section";
        public const string ConfigurationSettingName = "setting";
        public const string ConfigurationPackageName = "config_pkg";

        public const int ServiceCounterInitialValue = 0;
    }    
}