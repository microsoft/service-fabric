// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using KeyVault.Wrapper;
    using Microsoft.Cis.Monitoring.Query;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Moq;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using Tools.EtlReader;

    [TestClass]
    public class AzureBlobEtwUploaderTests
    {
        private const string TestEtlFilePatterns = "fabric*.etl";
        private const string TraceType = "AzureBlobEtwUploaderTests";
        private const string TestFabricNodeId = "abcd";
        private const string TestFabricNodeName = "Node";
        private const string TestConfigSectionName = "TestConfigSection";
        private const string AccountName = "shrek";
        private const string WinFabCitStoreAccountKeySecretName = "AccountKey-shrek";

        private static string executingAssemblyPath;
        private static string dcaTestArtifactPath;
        private static string expectedOutputDataFolderPath;
                
        /// <summary>
        /// Set static state shared between all tests.
        /// </summary>
        /// <param name="context">Test context.</param>
        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            Utility.TraceSource = new ErrorAndWarningFreeTraceEventSource();
            executingAssemblyPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            var sourceArtifactPath = Path.Combine(
                executingAssemblyPath,
                "DcaTestArtifacts");
            dcaTestArtifactPath = Path.Combine(Environment.CurrentDirectory, "DcaTestArtifacts_AzureBlobEtwUploaderTests");

            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, TestUtility.ManifestFolderName), Path.Combine(dcaTestArtifactPath, TestUtility.ManifestFolderName));

            // Rename .etl.dat files to .etl.  This is a workaround for runtests.cmd which excludes .etl files
            // from upload.
            var destFolder = Path.Combine(dcaTestArtifactPath, "Data");
            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, "Data"), destFolder);
            foreach (var file in Directory.GetFiles(destFolder, "*.etl.dat"))
            {
                // Move file to same name without .dat extension and without TEST prefix
                var fileInfo = new FileInfo(file);
                var newEtlFileName = fileInfo.Name.Substring(0, fileInfo.Name.Length - 4).Substring(4);
                var newEtlFullPath = Path.Combine(fileInfo.Directory.FullName, newEtlFileName);
                var etlFileToDelete = fileInfo.FullName.Substring(0, fileInfo.FullName.Length - 4);
                if (File.Exists(etlFileToDelete))
                {
                    File.Delete(etlFileToDelete);
                }

                File.Move(file, newEtlFullPath);
            }

            // Remove everything but etls
            foreach (var file in Directory.GetFiles(destFolder))
            {
                if (Path.GetExtension(file) != ".etl")
                {
                    File.Delete(file);
                }
            }

            expectedOutputDataFolderPath = Path.Combine(sourceArtifactPath, "OutputData");

            var logFolderPrefix = Path.Combine(Environment.CurrentDirectory, TraceType);

            Directory.CreateDirectory(logFolderPrefix);

            TraceTextFileSink.SetPath(Path.Combine(logFolderPrefix, TraceType));
        }

        /// <summary>
        /// Test inactive and active etl files are uploaded to Azure Blob storage
        /// from in-memory zipped stream.
        /// </summary>
        [TestMethod]
        public void TestUploadFromMemoryStream()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("AzureBlobEtwUploaderTests", "TestUploadFromMemoryStream", executingAssemblyPath, dcaTestArtifactPath, TestEtlFilePatterns);

            EtlInMemoryProducer etlInMemoryProducerWithAzureBlobConsumer = null;
            string containerName = string.Empty;
            StorageAccountFactory storageAccountFactory;
            CreateEtlInMemoryProducerWithAzureBlobConsumer(
                testEtlPath, 
                new TraceFileEventReaderFactory(), 
                out etlInMemoryProducerWithAzureBlobConsumer, 
                out containerName, 
                out storageAccountFactory);
            Thread.Sleep(TimeSpan.FromSeconds(15));
            etlInMemoryProducerWithAzureBlobConsumer.Dispose();

            // Verify blob storage content with expected output
            var downloadedBlobsPath = Path.Combine(testEtlPath, "DownloadedBlobs");
            Directory.CreateDirectory(downloadedBlobsPath);
            VerifyEvents(downloadedBlobsPath, containerName, storageAccountFactory);

            // Clean up container
            TestCleanUp(containerName, storageAccountFactory);
        }

        /// <summary>
        /// Test inactive and active etl files are uploaded to Azure Blob storage
        /// from persisted zipped files.
        /// </summary>
        [TestMethod]
        public void TestUploadFromPersistedFiles()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("AzureBlobEtwUploaderTests", "TestUploadFromPersistedFiles", executingAssemblyPath, dcaTestArtifactPath, TestEtlFilePatterns);

            EtlInMemoryProducer etlInMemoryProducerWithAzureBlobConsumer = null;
            string containerName = string.Empty;
            StorageAccountFactory storageAccountFactory;
            CreateEtlInMemoryProducerWithAzureBlobConsumer(
                testEtlPath,  
                new TraceFileEventReaderFactory(),
                out etlInMemoryProducerWithAzureBlobConsumer, 
                out containerName, 
                out storageAccountFactory, 
                AccessCondition.GenerateLeaseCondition(Guid.NewGuid().ToString()));
            Thread.Sleep(TimeSpan.FromSeconds(30));
            etlInMemoryProducerWithAzureBlobConsumer.Dispose();

            // Verify blob storage content with expected output
            var downloadedBlobsPath = Path.Combine(testEtlPath, "DownloadedBlobs");
            Directory.CreateDirectory(downloadedBlobsPath);
            VerifyEvents(downloadedBlobsPath, containerName, storageAccountFactory);

            // Clean up container
            TestCleanUp(containerName, storageAccountFactory);
        }

        /// <summary>
        /// Clean up container created by test
        /// </summary>
        private static void TestCleanUp(string containerName, StorageAccountFactory storageAccountFactory)
        {
            try
            {
                var container = storageAccountFactory.GetContainer(containerName);

                container.DeleteIfExists();
            }
            catch(Exception ex)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Failed to delete container {0}: {1}",
                    containerName,
                    ex);
            }
        }

        private static void CreateEtlInMemoryProducerWithAzureBlobConsumer(
            string logDirectory, 
            ITraceFileEventReaderFactory traceFileEventReaderFactory, 
            out EtlInMemoryProducer etlInMemoryProducer, 
            out string containerName,
            out StorageAccountFactory storageAccountFactory,
            AccessCondition uploadStreamAccessCondition = null,
            AccessCondition uploadFileAccessCondition = null)
        {
            var mockDiskSpaceManager = TestUtility.MockRepository.Create<DiskSpaceManager>();
            var etlInMemoryProducerSettings = new EtlInMemoryProducerSettings(
                true,
                TimeSpan.FromSeconds(1),
                TimeSpan.FromDays(3000),
                WinFabricEtlType.DefaultEtl,
                logDirectory,
                TestEtlFilePatterns,
                true);

            var configReader = TestUtility.MockRepository.Create<IEtlInMemoryProducerConfigReader>();
            configReader.Setup(c => c.GetSettings()).Returns(etlInMemoryProducerSettings);

            var mockTraceEventSourceFactory = TestUtility.MockRepository.Create<ITraceEventSourceFactory>();
            mockTraceEventSourceFactory
                .Setup(tsf => tsf.CreateTraceEventSource(It.IsAny<EventTask>()))
                .Returns(new ErrorAndWarningFreeTraceEventSource());

            var configStore = TestUtility.MockRepository.Create<IConfigStore>();
            configStore
               .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "MaxDiskQuotaInMB"))
               .Returns("100");
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "DiskFullSafetySpaceInMB"))
                .Returns("0");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.EnabledParamName))
                .Returns("true");

            containerName = string.Format("{0}-{1}", "fabriclogs", Guid.NewGuid());
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.ContainerParamName))
                .Returns(containerName);                                                     

            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.FileSyncIntervalParamName))
                .Returns("0.25");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.DataDeletionAgeParamName))
                .Returns("1");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.TestDataDeletionAgeParamName))
                .Returns("0");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.LogFilterParamName))
                .Returns("*.*:4");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, AzureConstants.DeploymentId))
                .Returns(AzureConstants.DefaultDeploymentId);

            var accountKey = GetTestStorageAccountKey();
            var isEncrypted = false;
            var storageConnectionString = string.Format(@"xstore:DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", AccountName, accountKey);
            configStore
                .Setup(cs => cs.ReadString(TestConfigSectionName, AzureConstants.ConnectionStringParamName, out isEncrypted))
                .Returns(storageConnectionString);

            Utility.InitializeConfigStore(configStore.Object);

            var mockConfigReaderFactory = TestUtility.MockRepository.Create<IEtlInMemoryProducerConfigReaderFactory>();
            mockConfigReaderFactory.Setup(f => f.CreateEtlInMemoryProducerConfigReader(It.IsAny<FabricEvents.ExtensionsEvents>(), It.IsAny<string>()))
                                   .Returns(configReader.Object);

            // Create the Azure Blob Uploader consumer
            ConfigReader.AddAppConfig(Utility.WindowsFabricApplicationInstanceId, null);
            var consumerInitParam = new ConsumerInitializationParameters(
                Utility.WindowsFabricApplicationInstanceId,
                TestConfigSectionName,
                TestFabricNodeId,
                TestFabricNodeName,
                Utility.LogDirectory,
                Utility.DcaWorkFolder,
                new DiskSpaceManager());

            // Save storage connection for clean up
            var azureUtility = new AzureUtility(new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA), TraceType);
            storageAccountFactory = azureUtility.GetStorageAccountFactory(
                new ConfigReader(consumerInitParam.ApplicationInstanceId), 
                TestConfigSectionName, 
                AzureConstants.ConnectionStringParamName);

            var azureBlobConsumer = new AzureBlobEtwUploader(consumerInitParam, uploadStreamAccessCondition, uploadFileAccessCondition);
            var etlToInMemoryBufferWriter = azureBlobConsumer.GetDataSink();

            var producerInitParam = new ProducerInitializationParameters
            {
                ConsumerSinks = new[] { etlToInMemoryBufferWriter }
            };

            // Create the in-memory producer
            etlInMemoryProducer = new EtlInMemoryProducer(
                mockDiskSpaceManager.Object,
                mockConfigReaderFactory.Object,
                traceFileEventReaderFactory,
                mockTraceEventSourceFactory.Object,
                producerInitParam);
        }

        private static string GetTestStorageAccountKey()
        {
            var kvw = KeyVaultWrapper.GetTestInfraKeyVaultWrapper();
            return kvw.GetSecret(KeyVaultWrapper.GetTestInfraKeyVaultUri(), WinFabCitStoreAccountKeySecretName);
        }

        private static void VerifyEvents(string downloadedBlobsPath, string containerName, StorageAccountFactory storageAccountFactory)
        {
            DownloadBlobs(downloadedBlobsPath, containerName, storageAccountFactory);

            TestUtility.AssertDtrPartsEqual(string.Empty, expectedOutputDataFolderPath, string.Empty, downloadedBlobsPath);
        }

        private static void DownloadBlobs(string downloadedBlobsPath, string containerName, StorageAccountFactory storageAccountFactory)
        {
            CloudStorageAccount storageAccount = null;

            try
            {
                storageAccount = storageAccountFactory.GetStorageAccount();

                // Create the blob client object
                CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();

                // Get the container reference
                CloudBlobContainer container = blobClient.GetContainerReference(containerName);

                CloudBlobDirectory directory = container.GetDirectoryReference(string.Join("/", TestFabricNodeName, "Fabric"));

                BlobContinuationToken continuationToken = null;
                BlobRequestOptions requestOptions = new BlobRequestOptions();
                OperationContext operationContext = new OperationContext();
                var blobResultSegment = directory.ListBlobsSegmented(true, BlobListingDetails.All, null, continuationToken, requestOptions, operationContext);
                int zipSequence = 1;
                foreach (ICloudBlob blob in blobResultSegment.Results)
                {
                    try
                    {
                        var localFilePath = Path.Combine(downloadedBlobsPath, string.Concat(zipSequence++, ".dtr.zip"));
                        blob.DownloadToFile(localFilePath, FileMode.Create);
                        var outputFilePath = localFilePath.Substring(0, localFilePath.Length - 4);
                        ZipFile.ExtractToDirectory(localFilePath, outputFilePath);

                        DirectoryInfo outputDir = new DirectoryInfo(outputFilePath);
                        foreach (var file in outputDir.GetFiles())
                        {
                            File.Copy(file.FullName, Path.Combine(downloadedBlobsPath, file.Name));
                            File.Delete(localFilePath);
                            Directory.Delete(outputDir.FullName, true);
                        }
                    }
                    catch (Exception ex)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Failed to download and unzip blob: {0}",
                            ex);
                    }
                }
            }
            finally
            {
                // Replace real key with a bogus key and then clear out the real key.
                if (storageAccount != null)
                {
                    storageAccount.Credentials.UpdateKey(
                        AzureUtility.DevelopmentStorageAccountWellKnownKey,
                        storageAccount.Credentials.KeyName);
                }
            }
        }
    }
}