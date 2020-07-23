// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using Microsoft.Cis.Monitoring.Query;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
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
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using Tools.EtlReader;

    [TestClass]
    public class MdsFileUploaderTests
    {
        private const string TestEtlFilePatterns = "fabric*.etl";
        private const int DefaultTableNonce = 0;
        private const string LogFilter = "*.*:4";
        private const string LogSourceId = "MdsFileUploaderTests";
        private const string TestNodeId = "abcd";
        private const string TestFabricNodeId = "NodeId";
        private const string TestFabricNodeName = "NodeName";
        private const string TestConfigSectionName = "TestConfigSection";
        private const int UploadIntervalMinutes = 1;
        private const string TableName = "TestTable";

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
            dcaTestArtifactPath = Path.Combine(Environment.CurrentDirectory, "DcaTestArtifacts_MdsFileUploaderTests");

            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, TestUtility.ManifestFolderName), Path.Combine(dcaTestArtifactPath, TestUtility.ManifestFolderName));

            // Rename .etl.dat files to .etl.  This is a workaround for runtests.cmd which excludes .etl files
            // from upload.
            var destFolder = Path.Combine(dcaTestArtifactPath, "Data");
            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, "Data"), destFolder);
            foreach (var file in Directory.GetFiles(destFolder, "*.etl.dat"))
            {
                // Move file to same name without .dat extension
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

            var logFolderPrefix = Path.Combine(Environment.CurrentDirectory, LogSourceId);

            Directory.CreateDirectory(logFolderPrefix);

            TraceTextFileSink.SetPath(Path.Combine(logFolderPrefix, LogSourceId));
        }

        /// <summary>
        /// Test inactive and active etl files are converted to MDS TSF files.
        /// </summary>
        [TestMethod]
        public void TestNormalConsumerProcessing()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("MdsFileUploaderTests", "TestNormalConsumerProcessing", executingAssemblyPath, dcaTestArtifactPath, TestEtlFilePatterns);

            var etlInMemoryProducerWithMDSConsumer = CreateEtlInMemoryProducerWithMDSConsumer(testEtlPath, new TraceFileEventReaderFactory());
            Thread.Sleep(TimeSpan.FromSeconds(15));
            etlInMemoryProducerWithMDSConsumer.Dispose();

            VerifyEvents(testEtlPath);
        }

        private static EtlInMemoryProducer CreateEtlInMemoryProducerWithMDSConsumer(string logDirectory, ITraceFileEventReaderFactory traceFileEventReaderFactory)
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
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.EnabledParamName))
                .Returns("true");
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.DirectoryParamName))
                .Returns(logDirectory);
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.TableParamName))
                .Returns(TableName);
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.TablePriorityParamName))
                .Returns(MdsFileProducerConstants.DefaultTablePriority);
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.DiskQuotaParamName))
                .Returns(MdsFileProducerConstants.DefaultDiskQuotaInMB.ToString());
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.BookmarkBatchSizeParamName))
                .Returns(MdsFileProducerConstants.DefaultBookmarkBatchSize.ToString());
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.DataDeletionAgeParamName))
                .Returns(MdsFileProducerConstants.DefaultDataDeletionAge.TotalDays.ToString());
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.TestDataDeletionAgeParamName))
                .Returns(MdsFileProducerConstants.DefaultDataDeletionAge.TotalDays.ToString());
            configStore
                .Setup(cs => cs.ReadUnencryptedString(TestConfigSectionName, MdsFileProducerConstants.LogFilterParamName))
                .Returns(LogFilter);
                                                     
            Utility.InitializeConfigStore(configStore.Object);

            var mockConfigReaderFactory = TestUtility.MockRepository.Create<IEtlInMemoryProducerConfigReaderFactory>();
            mockConfigReaderFactory.Setup(f => f.CreateEtlInMemoryProducerConfigReader(It.IsAny<FabricEvents.ExtensionsEvents>(), It.IsAny<string>()))
                                   .Returns(configReader.Object);

            // Create the MDS consumer
            ConfigReader.AddAppConfig(Utility.WindowsFabricApplicationInstanceId, null);
            var consumerInitParam = new ConsumerInitializationParameters(
                Utility.WindowsFabricApplicationInstanceId,
                TestConfigSectionName,
                TestFabricNodeId,
                TestFabricNodeName,
                Utility.LogDirectory,
                Utility.DcaWorkFolder,
                new DiskSpaceManager());

            var mdsFileProducer = new MdsFileProducer(consumerInitParam);
            var etlToInMemoryBufferWriter = mdsFileProducer.GetDataSink();

            var producerInitParam = new ProducerInitializationParameters
            {
                ConsumerSinks = new[] { etlToInMemoryBufferWriter }
            };

            // Create the in-memory producer
            return new EtlInMemoryProducer(
                mockDiskSpaceManager.Object,
                mockConfigReaderFactory.Object,
                traceFileEventReaderFactory,
                mockTraceEventSourceFactory.Object,
                producerInitParam);
        }

        private static void VerifyEvents(string logDirectory)
        {
            // Get merged lines from expected output
            var allLines = TestUtility.GetMergedDtrLines("fabric", expectedOutputDataFolderPath);

            // Calculate number of records from output dtr file
            List<string> allEventRecords = new List<string>();
            StringBuilder strb = new StringBuilder();
            foreach (var line in allLines)
            {
                var lineParts = line.Split(',');
                if (lineParts.Length > 0)
                {
                    var dateString = lineParts[0];
                    DateTime result = DateTime.MinValue;
                    if (DateTime.TryParse(dateString, out result))
                    {
                        if (strb.Length > 0)
                        {
                            allEventRecords.Add(strb.ToString());
                            strb.Clear();
                        }
                        strb.Append(line);
                    }
                    else
                    {
                        strb.Append(line);
                    }
                }
            }

            // Write out the last record
            if (strb.Length > 0)
            {
                allEventRecords.Add(strb.ToString());
                strb.Clear();
            }

            // Get records from MDS table
            string tablePath = Path.Combine(logDirectory, "Fabric");
            string tableName = String.Concat(
                TableName,
                MdsFileProducer.TableSchemaVersion,
                DefaultTableNonce);

            QueryLocalTable tableQuery = new QueryLocalTable(tableName, tablePath);
            var allTableRecords = tableQuery.ReadRecords(DateTime.MinValue, DateTime.MaxValue).ToArray();

            Assert.AreEqual(allEventRecords.Count, allTableRecords.Count(), "Total number of records in the dtr files should be the same as in the MDS files");

            // Compare records
            for (int i=0; i < allEventRecords.Count; i++)
            {
                string[] eventParts = allEventRecords[i].Split(new char[] { ',' }, (int)MdsFileProducer.EtwEventParts.Count);
                object[] expectedTableFields = new object[(int)MdsFileProducer.MdsTableFields.Count];
                expectedTableFields[(int)MdsFileProducer.MdsTableFields.Timestamp] = DateTime.Parse(eventParts[(int)MdsFileProducer.EtwEventParts.Timestamp]);
                expectedTableFields[(int)MdsFileProducer.MdsTableFields.Level] = eventParts[(int)MdsFileProducer.EtwEventParts.Level];
                expectedTableFields[(int)MdsFileProducer.MdsTableFields.ThreadId] = Int32.Parse(eventParts[(int)MdsFileProducer.EtwEventParts.ThreadId]);
                expectedTableFields[(int)MdsFileProducer.MdsTableFields.ProcessId] = Int32.Parse(eventParts[(int)MdsFileProducer.EtwEventParts.ProcessId]);

                var winFabId = eventParts[(int)MdsFileProducer.EtwEventParts.WinFabId];
                string[] winFabIdParts = winFabId.Split(new[] { '.', '@' }, (int)MdsFileProducer.WinFabIdParts.MaxCount);
                if (winFabIdParts.Length >= (int)MdsFileProducer.WinFabIdParts.MinCount)
                {
                    expectedTableFields[(int)MdsFileProducer.MdsTableFields.TaskName] = winFabIdParts[(int)MdsFileProducer.WinFabIdParts.TaskName];

                    var eventType = winFabIdParts[(int)MdsFileProducer.WinFabIdParts.EventType];
                    string eventGroupName = EventDefinition.GetEventGroupName(eventType);
                    if (!string.IsNullOrEmpty(eventGroupName))
                    {
                        eventType = eventGroupName;
                    }

                    expectedTableFields[(int)MdsFileProducer.MdsTableFields.EventType] = eventType;

                    expectedTableFields[(int)MdsFileProducer.MdsTableFields.Id] = (winFabIdParts.Length == (int)MdsFileProducer.WinFabIdParts.MinCount) ? (string.Empty) : (winFabIdParts[(int)MdsFileProducer.WinFabIdParts.Id]);
                }

                expectedTableFields[(int)MdsFileProducer.MdsTableFields.Text] = eventParts[(int)MdsFileProducer.EtwEventParts.Text]; 

                Assert.AreEqual(1 + expectedTableFields.Length, allTableRecords[i].FieldData.Length, "Schema mismatch between dtr file and mds table record");

                bool match = true;
                for (int j = 0; j < expectedTableFields.Length; j++)
                {
                    // for text fields compare after stripping CR and tabs
                    if (j == (int)MdsFileProducer.MdsTableFields.Text)
                    {
                        var expectedText = ((string)expectedTableFields[(int)MdsFileProducer.MdsTableFields.Text]).Replace("\r", "").Replace("\t", "");
                        var mdsText = ((string)allTableRecords[i].FieldData[j + 1]).Replace("\r", "").Replace("\t", "");
                        if (expectedText.Equals(mdsText) == false)
                        {
                            match = false;
                            break;
                        }
                    }
                    else
                    {
                        if (expectedTableFields[j].Equals(allTableRecords[i].FieldData[j + 1]) == false)
                        {
                            match = false;
                            break;
                        }
                    }
                }

                Assert.IsTrue(match, "Event record values differ between the dtr file and mds table record");
            }
        }
    }
}