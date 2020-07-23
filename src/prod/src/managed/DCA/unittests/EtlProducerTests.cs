// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Linq;

namespace FabricDCA.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.IO;
    using System.Reflection;
    using System.Threading;

    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using Tools.EtlReader;

    [TestClass]
    public class EtlProducerTests
    {
        private const string LogSourceId = "EtlProducerTests";
        private const string TestNodeId = "abcd";
        private const string EtwProviderGuid = "cbd93bc2-71e5-4566-b3a7-595d8eeca6e8";
        
        private static string activeEtlFileName;
        private static string expectedOutputDataFolderPath;
        private static string executingAssemblyPath;
        private static string dcaTestArtifactPath;

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
            dcaTestArtifactPath = Path.Combine(Environment.CurrentDirectory, "DcaTestArtifacts_EtlProducerTests");

            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, TestUtility.ManifestFolderName), Path.Combine(dcaTestArtifactPath, TestUtility.ManifestFolderName));

            // Rename .etl.dat files to .etl.  This is a workaround for runtests.cmd which exludes .etl files
            // from upload.
            var destFolder = Path.Combine(dcaTestArtifactPath, "Data");
            TestUtility.CopyAndOverwrite(Path.Combine(sourceArtifactPath, "Data"), destFolder);
            foreach (var file in Directory.GetFiles(destFolder, "*.etl.dat"))
            {
                // Move file to same name without .dat extension
                var etlName = file.Substring(0, file.Length - 4);
                if (File.Exists(etlName))
                {
                    File.Delete(etlName);
                }

                File.Move(file, etlName);
            }

            // Remove everything but etls
            foreach (var file in Directory.GetFiles(destFolder))
            {
                if (Path.GetExtension(file) != ".etl")
                {
                    File.Delete(file);
                }
            }

            // Set active file name to the file with largest filetimestamp in name.
            activeEtlFileName =
                Path.GetFileName(
                    Directory.GetFiles(Path.Combine(dcaTestArtifactPath, "Data")).OrderByDescending(n => n).First());
            expectedOutputDataFolderPath = Path.Combine(sourceArtifactPath, "OutputData");

            var logFolderPrefix = Path.Combine(Environment.CurrentDirectory, LogSourceId);

            Directory.CreateDirectory(logFolderPrefix);
            TraceTextFileSink.SetPath(Path.Combine(logFolderPrefix, LogSourceId));
        }

        /// <summary>
        /// Set state before each test.
        /// </summary>
        [TestInitialize]
        public void TestSetup()
        {
        }


        /// <summary>
        /// Test inactive and active etl files are converted given enough time.
        /// </summary>
        [TestMethod]
        public void TestNormalEtlProcessing()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlProducerTests", "TestNormalEtlProcessing", executingAssemblyPath, dcaTestArtifactPath);

            var etlProducer = CreateEtlProducerForTest(testEtlPath, new TraceFileEventReaderFactory());
            Thread.Sleep(TimeSpan.FromSeconds(10));
            etlProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath);
        }

        /// <summary>
        /// Test inactive and active etl files are converted using FlushOnDispose flag.
        /// </summary>
        [TestMethod]
        public void TestShutdownLoss()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlProducerTests", "TestShutdownLoss", executingAssemblyPath, dcaTestArtifactPath);

            var etlProducer = CreateEtlProducerForTest(testEtlPath, new TraceFileEventReaderFactory());
            etlProducer.FlushData();

            // Must flush a second time or last trace will be left off to protect against duplicates.
            // If better checkpointing can be used then this call might not be necessary.
            etlProducer.FlushData();

            etlProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath);
        }

        /// <summary>
        /// Test active etl file gets converted properly over lifetime of different EtlProducers.
        /// </summary>
        [TestMethod]
        public void TestStopStartLoss()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlProducerTests", "TestStopStartLoss", executingAssemblyPath, dcaTestArtifactPath);
            var activeEtlFilePath = Path.Combine(testEtlPath, activeEtlFileName);

            var mockTraceFileEventReaderFactory = TestUtility.MockRepository.Create<ITraceFileEventReaderFactory>();
            mockTraceFileEventReaderFactory
                .Setup(f => f.CreateTraceFileEventReader(It.Is((string file) => file == activeEtlFilePath)))
                .Returns(() => TestUtility.CreateForwardingEventReader(activeEtlFilePath).Object);
            mockTraceFileEventReaderFactory
                .Setup(f => f.CreateTraceFileEventReader(It.Is((string file) => file != activeEtlFilePath)))
                .Returns((string f) => new TraceFileEventReader(f));

            Func<EtlProducer> createNewProducer =
                () => CreateEtlProducerForTest(testEtlPath, mockTraceFileEventReaderFactory.Object);

            var etlProducer = createNewProducer();
            Thread.Sleep(TimeSpan.FromSeconds(6));
            etlProducer.Dispose();

            etlProducer = createNewProducer();
            Thread.Sleep(TimeSpan.FromSeconds(6));
            etlProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath);
        }


        /// <summary>
        /// Test active etl files get converted concurrently with multiple EtlProducers.
        /// Create 2 readers a slow and fast.  The slow is started first and then stopped
        /// until the fast finishes.  The fast should be allowed to complete while the slow
        /// is stalled.
        /// Set timeout to 10 seconds in case of deadlock.
        /// </summary>
        [TestMethod]
        [Timeout(10000)]
        public void TestConcurrentDecode()
        {
            var testEtlPathSlow = TestUtility.InitializeTestFolder("EtlProducerTests", "TestConcurrentDecodeSlow", executingAssemblyPath, dcaTestArtifactPath);
            var testEtlPathFast = TestUtility.InitializeTestFolder("EtlProducerTests", "TestConcurrentDecodeFast", executingAssemblyPath, dcaTestArtifactPath);

            var mockTraceFileEventReaderFactoryLocked = TestUtility.MockRepository.Create<ITraceFileEventReaderFactory>();
            var stallSlowReaderEvent = new ManualResetEvent(false);
            var startFastReaderEvent = new ManualResetEvent(false);
            mockTraceFileEventReaderFactoryLocked
                .Setup(f => f.CreateTraceFileEventReader(It.IsAny<string>()))
                .Returns((string f) =>
                {
                    startFastReaderEvent.Set();
                    stallSlowReaderEvent.WaitOne();
                    return new TraceFileEventReader(f);
                });

            // create slow producer
            var slowEtlProducer = CreateEtlProducerForTest(testEtlPathSlow, mockTraceFileEventReaderFactoryLocked.Object);

            // create fast producer and let it run
            startFastReaderEvent.WaitOne();
            var fastEtlProducer = CreateEtlProducerForTest(testEtlPathFast, new TraceFileEventReaderFactory());
            fastEtlProducer.FlushData();
            fastEtlProducer.FlushData();
            fastEtlProducer.Dispose();
            TestUtility.AssertFinished(testEtlPathFast, expectedOutputDataFolderPath);

            // run slow producer
            stallSlowReaderEvent.Set();
            slowEtlProducer.FlushData();
            slowEtlProducer.FlushData();
            slowEtlProducer.Dispose();
            TestUtility.AssertFinished(testEtlPathSlow, expectedOutputDataFolderPath);
        }

        /// <summary>
        /// Test to check that trace file event reader
        /// raises the event lost metadata event
        /// </summary>
        [TestMethod]
        public void TestReadTraceSessionMetadata()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlProducerTests", "TestReadTraceSessionMetadata", executingAssemblyPath, dcaTestArtifactPath);
            var activeEtlFilePath = Path.Combine(testEtlPath, activeEtlFileName);
            var eventsLostReader = TestUtility.CreateEventsLostReader(activeEtlFilePath).Object;

            var traceSessionMetadata = eventsLostReader.ReadTraceSessionMetadata();
            Assert.IsTrue(traceSessionMetadata.EventsLostCount == 10);
            Assert.IsTrue(traceSessionMetadata.StartTime.ToString() == "5/4/2016 12:17:21 AM");
            Assert.IsTrue(traceSessionMetadata.EndTime.ToString() == "5/4/2016 12:17:32 AM");
        }

        private static EtlProducer CreateEtlProducerForTest(
            string logDirectory, 
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            var mockDiskSpaceManager = TestUtility.MockRepository.Create<DiskSpaceManager>();
            var etlProducerSettings = new EtlProducerSettings(
                true,
                TimeSpan.FromSeconds(1),
                TimeSpan.FromDays(3000),
                WinFabricEtlType.DefaultEtl,
                logDirectory,
                TestUtility.TestEtlFilePatterns,
                new List<string>(),
                new Dictionary<string, List<ServiceEtwManifestInfo>>(),
                true,
                new[] { Guid.Parse(EtwProviderGuid) },
                null);

            var cr = TestUtility.MockRepository.Create<IEtlProducerConfigReader>();
            cr.Setup(c => c.GetSettings()).Returns(etlProducerSettings);

            var configStore = TestUtility.MockRepository.Create<IConfigStore>();
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "TestOnlyOldDataDeletionIntervalInSeconds"))
                .Returns("3600");
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "ApplicationLogsFormatVersion"))
                .Returns((string)null);
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "TestOnlyDtrDeletionAgeInMinutes"))
                .Returns("60");
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "MaxDiskQuotaInMB"))
                .Returns("100");
            configStore
                .Setup(cs => cs.ReadUnencryptedString("Diagnostics", "DiskFullSafetySpaceInMB"))
                .Returns("0");
            Utility.InitializeConfigStore(configStore.Object);

            var mockConfigReaderFactory = TestUtility.MockRepository.Create<IEtlProducerConfigReaderFactory>();
            mockConfigReaderFactory
                .Setup(f => f.CreateEtlProducerConfigReader(It.IsAny<FabricEvents.ExtensionsEvents>(), It.IsAny<string>()))
                .Returns(cr.Object);

            var mockTraceSourceFactory = TestUtility.MockRepository.Create<ITraceEventSourceFactory>();
            mockTraceSourceFactory
                .Setup(tsf => tsf.CreateTraceEventSource(It.IsAny<EventTask>()))
                .Returns(new ErrorAndWarningFreeTraceEventSource());

            var csvWriter = new EtlToCsvFileWriter(
                mockTraceSourceFactory.Object,
                LogSourceId,
                TestNodeId,
                TestUtility.GetActualOutputFolderPath(logDirectory),
                true,
                mockDiskSpaceManager.Object);
            var initParam = new ProducerInitializationParameters
            {
                ConsumerSinks = new[] { csvWriter }
            };

            return new EtlProducer(
                mockDiskSpaceManager.Object,
                mockConfigReaderFactory.Object,
                traceFileEventReaderFactory,
                mockTraceSourceFactory.Object,
                initParam);
        }
    }
}