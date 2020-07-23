// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureTableUploaderTest 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading;

    using AzureTableDownloader;
    using FabricDCA;
    using KeyVault.Wrapper;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Auth;
    using Microsoft.WindowsAzure.Storage.Table;
    using Tools.EtlReader;
    using WEX.TestExecution;

    [TestClass]
    class AzureTableUploaderTest
    {
        // Members that are not test specific
        private TestContext testContext;
        public TestContext TestContext
        {
            get { return testContext; }
            set { testContext = value; }
        }

        internal const string TraceType = "AzureTableUploaderTest";
        private const string TestFabricNodeId = "TestFabricNodeId";
        private const string TestConfigSectionName = "TestConfigSection";
        private const string ManifestFileName = "AzureTableUploaderTest.man";
        private const string configFileName = "AzureTableUploaderTest.cfg";
        private const string accountName = "shrek";
        private const string WinFabCitStoreAccountKeySecretName = "AccountKey-shrek";
        private const string configFileContents =
        @"
[Trace/File]
Level = 5
Path = AzureTableUploader.Test
Option = m

[TestConfigSection]
IsEnabled = true
StoreConnectionString = xstore:DefaultEndpointsProtocol\=https\;AccountName\={0}\;AccountKey\={1}
TableNamePrefix = {2}
TestOnlyDataDeletionAgeInMinutes = {3}
TestOnlyEtwManifestFilePattern = {4}
        ";

        internal const string TaskNameProperty = "TaskName";
        internal const string EventTypeProperty = "EventType";
        internal const string DcaVersionProperty = "dca_version";
        internal const string DcaInstanceProperty = "dca_instance";
        internal const string DataProperty = "data";

        private static Guid providerId = Guid.Parse("{C083AB76-A7F3-4CA7-9E64-CA7E5BA8807A}");
        internal static string TableNamePrefix;
        private const int StartTimeOffsetMinutes = 60;
        internal const int EventIntervalMinutes = 1;
        private const int EntityDeletionAgeMinutes =
            StartTimeOffsetMinutes - ((EventIntervalMinutes * IdBasedEventSource.EventsPerInstance * IdBasedEventSource.MaxInstancesPerPartition) / 2);
        private const int DeletionCutoffTimeBufferMinutes = 2;

        private static string testDataDirectory;

        private static ManifestCache manifestCache;

        private static CloudTableClient tableClient;

        [ClassInitialize]
        public static void AzureTableUploaderTestSetup(object testContext)
        {
            Verify.IsTrue(EntityDeletionAgeMinutes > 0);

            string assemblyLocation = Assembly.GetExecutingAssembly().Location;
            string manifestDir = Path.GetDirectoryName(assemblyLocation);
            // Prepare the manifest cache
            manifestCache = new ManifestCache(manifestDir);
            string manifestFullPath = Path.Combine(manifestDir, ManifestFileName);
            manifestCache.LoadManifest(manifestFullPath);

            // Create the table name prefix
            TableNamePrefix = String.Concat("CIT", Guid.NewGuid().ToString("N"));

            // Create the test data directory
            const string testDataDirectoryName = "AzureTableUploader.Test.Data";
            testDataDirectory = Path.Combine(Directory.GetCurrentDirectory(), testDataDirectoryName);
            Directory.CreateDirectory(testDataDirectory);

            var accountKey = GetTestStorageAccountKey();
            var escapedAccountKey = accountKey.Replace("=", @"\=");

            // Create the config file for the test
            string configFileFullPath = Path.Combine(
                                     testDataDirectory,
                                     configFileName);
            string configFileContentsFormatted = String.Format(
                                                     CultureInfo.InvariantCulture,
                                                     configFileContents,
                                                     accountName,
                                                     escapedAccountKey,
                                                     TableNamePrefix,
                                                     EntityDeletionAgeMinutes,
                                                     ManifestFileName);
            byte[] configFileBytes = Encoding.ASCII.GetBytes(configFileContentsFormatted); 
            File.WriteAllBytes(configFileFullPath, configFileBytes);
            Environment.SetEnvironmentVariable("FabricConfigFileName", configFileFullPath);

            // Initialize config store
            Utility.InitializeConfigStore((IConfigStoreUpdateHandler)null);

            Utility.InitializeTracing();
            Utility.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

            HealthClient.Disabled = true;

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Table name prefix: {0}",
                TableNamePrefix);

            // Create the table client
            StorageCredentials credentials = new StorageCredentials(accountName, accountKey);
            CloudStorageAccount storageAccount = new CloudStorageAccount(credentials, true);
            tableClient = storageAccount.CreateCloudTableClient();
        }

        [ClassCleanup]
        public static void AzureTableUploaderTestCleanup()
        {
            // Delete all tables created by the test
            IEnumerable<CloudTable> tables = tableClient.ListTables(TableNamePrefix);
            foreach (CloudTable table in tables)
            {
                SaveDataFromTable(table);
                try
                {
                    table.DeleteIfExists();
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Failed to delete table {0}. Exception {1}.",
                        table.Name,
                        e);
                }
            }
        }

        private static string GetTestStorageAccountKey()
        {
            var kvw = KeyVaultWrapper.GetTestInfraKeyVaultWrapper();
            return kvw.GetSecret(KeyVaultWrapper.GetTestInfraKeyVaultUri(), WinFabCitStoreAccountKeySecretName);
        }

        private static void SaveDataFromTable(CloudTable table)
        {
            try
            {
                // Initialize file name
                string fileName = Path.ChangeExtension(table.Name, ".csv");
                string fileFullPath = Path.Combine(testDataDirectory, fileName);

                // Download the table to the file
                AzureTableDownloader.DownloadToFile(table, fileFullPath);
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Failed to save data from table {0}. Exception {1}.",
                    table.Name,
                    e);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void QueryableEventUploadTest()
        {
            // Initialize last FMM event timestamp to a value that
            // will prevent the uploaders from performing old entity
            // deletion
            Utility.LastFmmEventTimestamp = DateTime.MinValue;

            // Upload events to table
            Thread idBasedUploader1 = new Thread(this.UploadEvents);
            Thread idBasedUploader2 = new Thread(this.UploadEvents);
            Thread nonIdBasedUploader = new Thread(this.UploadEvents);

            DateTime startTime = DateTime.UtcNow.AddMinutes(-1 * StartTimeOffsetMinutes);
            IdBasedEventSource idBasedSource = new IdBasedEventSource(providerId, startTime);
            NonIdBasedEventSource nonIdBasedSource = new NonIdBasedEventSource(providerId, startTime);

            idBasedUploader1.Start(idBasedSource);
            idBasedUploader2.Start(idBasedSource);
            nonIdBasedUploader.Start(nonIdBasedSource);

            idBasedUploader1.Join();
            idBasedUploader2.Join();
            nonIdBasedUploader.Join();

            // Verify that the events were uploaded successfully
            idBasedSource.VerifyUpload(tableClient);
            nonIdBasedSource.VerifyUpload(tableClient);

            // Delete old events from table

            // Initialize last FMM event timestamp to a value that
            // will cause the uploader to perform old entity deletion
            Utility.LastFmmEventTimestamp = DateTime.UtcNow;

            // Deletion routine will run approximately one minute after we create the uploader below
            DateTime deletionRoutineRunTime = DateTime.UtcNow.AddMinutes(1);

            // Figure out - approximately - the cut-off time computed by the deletion routine
            DateTime deletionCutoffTime = deletionRoutineRunTime.AddMinutes(-1 * EntityDeletionAgeMinutes);

            // Since the cut-off time prediction is only approximate, add buffers
            DateTime deletionCutoffTimeLow = deletionCutoffTime.AddMinutes(-1 * DeletionCutoffTimeBufferMinutes);
            DateTime deletionCutoffTimeHigh = deletionCutoffTime.AddMinutes(DeletionCutoffTimeBufferMinutes);

            // Create an uploader to perform the deletion
            AzureTableQueryableEventUploader uploader = CreateAndInitializeUploader();

            int deletionWaitTimeSeconds = 150;
            bool verifyDeletionResult = false;
            int verificationAttempts = 0;
            while (deletionWaitTimeSeconds > 0)
            {
                // Sleep for a while to allow the deletion routine to run
                Thread.Sleep(deletionWaitTimeSeconds * 1000);

                // Check if the deletion is complete
                verificationAttempts++;
                Utility.TraceSource.WriteInfo(
                    AzureTableUploaderTest.TraceType,
                    "Attempt {0}: Verifying old entity deletion ...",
                    verificationAttempts);
                verifyDeletionResult = idBasedSource.VerifyDeletion(tableClient, deletionCutoffTimeLow, deletionCutoffTimeHigh);
                verifyDeletionResult = verifyDeletionResult && nonIdBasedSource.VerifyDeletion(tableClient, deletionCutoffTimeLow, deletionCutoffTimeHigh);

                if (verifyDeletionResult)
                {
                    Utility.TraceSource.WriteInfo(
                        AzureTableUploaderTest.TraceType,
                        "Finished verifying old entity deletion.");
                    break;
                }
                else
                {
                    Utility.TraceSource.WriteInfo(
                        AzureTableUploaderTest.TraceType,
                        "Old entity deletion is either incomplete or unsuccessful. Will verify again after a while, if we have not reached the limit for verification attempts.");
                }

                // The next wait time will be shorter
                deletionWaitTimeSeconds = deletionWaitTimeSeconds / 2;
            }

            // Dispose the uploader
            uploader.Dispose();

            if (false == verifyDeletionResult)
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Old entites have not been deleted as expected. We have reached the limit for verification attempts.");
                Verify.Fail("Old entity deletion failed");
            }
        }

        private AzureTableQueryableEventUploader CreateAndInitializeUploader()
        {
            const string TestFabricNodeInstanceName = "test";
            const string TestLogDirectory = "Logs";
            const string TestWorkDirectory = "Work";

            // Once Uploader is constructed DiskSpaceManager is no longer needed.
            using (var testDiskSpaceManager = new DiskSpaceManager())
            {

                ConfigReader.AddAppConfig(Utility.WindowsFabricApplicationInstanceId, null);
                ConsumerInitializationParameters initParam = new ConsumerInitializationParameters(
                    Utility.WindowsFabricApplicationInstanceId,
                    TestConfigSectionName,
                    TestFabricNodeId,
                    TestFabricNodeInstanceName,
                    TestLogDirectory,
                    TestWorkDirectory,
                    testDiskSpaceManager);
                return new AzureTableQueryableEventUploader(initParam);
            }
        }

        private void UploadEvents(object context)
        {
            // Get the event source
            EventSource eventSource = context as EventSource;

            // Create and initialize the uploader
            AzureTableQueryableEventUploader uploader = CreateAndInitializeUploader();

            // Provide the uploader with the ETW manifest cache
            uploader.SetEtwManifestCache(manifestCache);

            // Inform the uploader that we're about to deliver ETW events to it
            uploader.OnEtwEventProcessingPeriodStart();

            // Deliver the ETW events to the uploader
            EventRecord eventRecord;
            bool result = eventSource.GetNextEvent(out eventRecord);
            while (result)
            {
                uploader.OnEtwEventsAvailable(new EventRecord[] { eventRecord });
                eventSource.FreeEventBuffers(eventRecord);
                result = eventSource.GetNextEvent(out eventRecord);
            }

            // Inform the uploader that we're done delivering ETW events to it
            uploader.OnEtwEventProcessingPeriodStop();

            // Dispose the uploader
            uploader.Dispose();
        }

        internal static void VerifyEntityProperty<T>(
                                DynamicTableEntity entity,
                                string propertyName,
                                string tableName,
                                T[] validValues,
                                T actualValue)
        {
            if (false == validValues.Contains(actualValue))
            {
                Utility.TraceSource.WriteError(
                    AzureTableUploaderTest.TraceType,
                    "Property {0} for entity with PK: {1}, RK: {2} in table {3} has invalid value {4}. Valid values: {5}.",
                    propertyName,
                    entity.PartitionKey,
                    entity.RowKey,
                    tableName,
                    actualValue,
                    String.Join(",", validValues));
                Verify.Fail("Invalid property value in table");
            }
        }
    }
}