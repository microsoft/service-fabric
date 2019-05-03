// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using System;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
    using Tools.EtlReader;

    [TestClass]
    public class EtlInMemoryProducerTests
    {
        private const string LogSourceId = "EtlInMemoryProducerTests";
        private const string TestNodeId = "abcd";
                        
        private static string executingAssemblyPath;
        private static string dcaTestArtifactPath;
        private static string activeEtlFileName;
        private static string expectedOutputDataFolderPath;

        // <summary>
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
            dcaTestArtifactPath = Path.Combine(Environment.CurrentDirectory, "DcaTestArtifacts_EtlInMemoryProducerTests");

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
        /// Test inactive and active etl files are converted given enough time.
        /// </summary>
        [TestMethod]
        public void TestNormalEtlProcessing()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlInMemoryProducerTests", "TestNormalEtlProcessing", executingAssemblyPath, dcaTestArtifactPath);

            var etlInMemoryProducer = CreateEtlInMemoryProducerForTest(testEtlPath, new TraceFileEventReaderFactory());
            Thread.Sleep(TimeSpan.FromSeconds(10));
            etlInMemoryProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath);
        }

        /// <summary>
        /// Test inactive and active etl files are converted using FlushOnDispose flag.
        /// </summary>
        [TestMethod]
        public void TestShutdownLoss()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlInMemoryProducerTests", "TestShutdownLoss", executingAssemblyPath, dcaTestArtifactPath);

            var etlProducer = CreateEtlInMemoryProducerForTest(testEtlPath, new TraceFileEventReaderFactory());
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
            var testEtlPath = TestUtility.InitializeTestFolder("EtlInMemoryProducerTests", "TestStopStartLoss", executingAssemblyPath, dcaTestArtifactPath);
            var activeEtlFilePath = Path.Combine(testEtlPath, activeEtlFileName);

            var mockTraceFileEventReaderFactory = TestUtility.MockRepository.Create<ITraceFileEventReaderFactory>();
            mockTraceFileEventReaderFactory
                .Setup(f => f.CreateTraceFileEventReader(It.Is((string file) => file == activeEtlFilePath)))
                .Returns(() => TestUtility.CreateForwardingEventReader(activeEtlFilePath).Object);
            mockTraceFileEventReaderFactory
                .Setup(f => f.CreateTraceFileEventReader(It.Is((string file) => file != activeEtlFilePath)))
                .Returns((string f) => new TraceFileEventReader(f));

            var etlInMemoryProducer = CreateEtlInMemoryProducerForTest(testEtlPath, mockTraceFileEventReaderFactory.Object);
            Thread.Sleep(TimeSpan.FromSeconds(6));
            etlInMemoryProducer.Dispose();

            etlInMemoryProducer = CreateEtlInMemoryProducerForTest(testEtlPath, mockTraceFileEventReaderFactory.Object);
            Thread.Sleep(TimeSpan.FromSeconds(6));
            etlInMemoryProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath);
        }

        /// <summary>
        /// Tests that the in-memory producer blocks until it gets acked by both
        /// the fast and the slow consumer.
        /// </summary>
        [TestMethod]
        public void TestEtlProcessingWithFastAndSlowConsumer()
        {
            var testEtlPath = TestUtility.InitializeTestFolder("EtlInMemoryProducerTests", "TestEtlProcessingWithFastAndSlowConsumer", executingAssemblyPath, dcaTestArtifactPath);

            var etlInMemoryProducer = CreateEtlInMemoryProducerWithFastAndSlowConsumerForTest(testEtlPath, new TraceFileEventReaderFactory());
            Thread.Sleep(TimeSpan.FromSeconds(15));
            etlInMemoryProducer.Dispose();

            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath, "*_fast");
            TestUtility.AssertFinished(testEtlPath, expectedOutputDataFolderPath, "*_slow");
        }

        private static EtlInMemoryProducer CreateEtlInMemoryProducerForTest(
            string logDirectory,
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            var mockDiskSpaceManager = TestUtility.MockRepository.Create<DiskSpaceManager>();
            var etlInMemoryProducerSettings = new EtlInMemoryProducerSettings(
                true,
                TimeSpan.FromSeconds(1),
                TimeSpan.FromDays(3000),
                WinFabricEtlType.DefaultEtl,
                logDirectory,
                TestUtility.TestEtlFilePatterns,
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
            Utility.InitializeConfigStore(configStore.Object);

            var mockConfigReaderFactory = TestUtility.MockRepository.Create<IEtlInMemoryProducerConfigReaderFactory>();
            mockConfigReaderFactory.Setup(f => f.CreateEtlInMemoryProducerConfigReader(It.IsAny<FabricEvents.ExtensionsEvents>(), It.IsAny<string>()))
                                   .Returns(configReader.Object);

            var dcaInMemoryConsumer = CreateEtlToInMemoryBufferWriter(logDirectory, null);

            var etlToInMemoryBufferWriter = new EtlToInMemoryBufferWriter(
                mockTraceEventSourceFactory.Object,
                LogSourceId,
                TestNodeId,
                TestUtility.GetActualOutputFolderPath(logDirectory),
                true,
                dcaInMemoryConsumer);

            var initParam = new ProducerInitializationParameters
            {
                ConsumerSinks = new[] { etlToInMemoryBufferWriter }
            };

            return new EtlInMemoryProducer(
                mockDiskSpaceManager.Object,
                mockConfigReaderFactory.Object,
                traceFileEventReaderFactory,
                mockTraceEventSourceFactory.Object,
                initParam);
        }

        private static EtlInMemoryProducer CreateEtlInMemoryProducerWithFastAndSlowConsumerForTest(
            string logDirectory,
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            var mockDiskSpaceManager = TestUtility.MockRepository.Create<DiskSpaceManager>();
            var etlInMemoryProducerSettings = new EtlInMemoryProducerSettings(
                true,
                TimeSpan.FromSeconds(1),
                TimeSpan.FromDays(3000),
                WinFabricEtlType.DefaultEtl,
                logDirectory,
                TestUtility.TestEtlFilePatterns,
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
            Utility.InitializeConfigStore(configStore.Object);

            var mockConfigReaderFactory = TestUtility.MockRepository.Create<IEtlInMemoryProducerConfigReaderFactory>();
            mockConfigReaderFactory.Setup(f => f.CreateEtlInMemoryProducerConfigReader(It.IsAny<FabricEvents.ExtensionsEvents>(), It.IsAny<string>()))
                                   .Returns(configReader.Object);

            var fastDcaInMemoryConsumer = CreateEtlToInMemoryBufferWriter(logDirectory, false);
            var slowDcaInMemoryConsumer = CreateEtlToInMemoryBufferWriter(logDirectory, true);

            var fastEtlToInMemoryBufferWriter = new EtlToInMemoryBufferWriter(
                mockTraceEventSourceFactory.Object,
                LogSourceId,
                TestNodeId,
                TestUtility.GetActualOutputFolderPath(logDirectory),
                true,
                fastDcaInMemoryConsumer);

            var slowEtlToInMemoryBufferWriter = new EtlToInMemoryBufferWriter(
                mockTraceEventSourceFactory.Object,
                LogSourceId,
                TestNodeId,
                TestUtility.GetActualOutputFolderPath(logDirectory),
                true,
                slowDcaInMemoryConsumer);

            var initParam = new ProducerInitializationParameters
            {
                ConsumerSinks = new[] { fastEtlToInMemoryBufferWriter, slowEtlToInMemoryBufferWriter }
            };

            return new EtlInMemoryProducer(
                mockDiskSpaceManager.Object,
                mockConfigReaderFactory.Object,
                traceFileEventReaderFactory,
                mockTraceEventSourceFactory.Object,
                initParam);
        }

        private static string CreateDestinationFileName(string etlFileName, string additionalSuffix, bool isActiveEtl, EventIndex lastEventIndexProcessed)
        {
            string differentiator = string.Format(
                                        CultureInfo.InvariantCulture,
                                        "{0:D10}",
                                        isActiveEtl ? lastEventIndexProcessed.TimestampDifferentiator : int.MaxValue);

            string traceFileNamePrefix = string.Format(
                                                  CultureInfo.InvariantCulture,
                                                  "{0}_{1}_{2:D20}_{3}{4}.",
                                                  TestNodeId,
                                                  Path.GetFileNameWithoutExtension(etlFileName),
                                                  lastEventIndexProcessed.Timestamp.Ticks,
                                                  differentiator,
                                                  additionalSuffix);

            return string.Concat(
                traceFileNamePrefix,
                EtlConsumerConstants.FilteredEtwTraceFileExtension);
        }

        private static IDcaInMemoryConsumer CreateEtlToInMemoryBufferWriter(string logDirectory, bool? slow)
        {
            EventIndex globalLastEventIndexProcessed = default(EventIndex);
            globalLastEventIndexProcessed.Set(DateTime.MinValue, -1);
            InternalFileSink fileSink = new InternalFileSink(Utility.TraceSource, LogSourceId);
            EventIndex lastEventIndexProcessed = default(EventIndex);

            var onProcessingPeriodStartActionForFastWriter = new Action<string, bool, string>((traceFileName, isActiveEtl, traceFileSubFolder) =>
            {
                fileSink.Initialize();
                lastEventIndexProcessed.Set(DateTime.MinValue, -1);
            });

            var maxIndexAlreadyProcessedForFastWriter = new Func<string, EventIndex>((traceFileSubFolder) =>
            {
                return globalLastEventIndexProcessed;
            });

            var consumerProcessTraceEventActionForFastWriter = new Action<DecodedEventWrapper, string>((decodedEventWrapper, traceFileSubFolder) =>
            {
                string etwEvent = decodedEventWrapper.StringRepresentation.Replace("\r\n", "\r\t").Replace("\n", "\t");
                fileSink.WriteEvent(etwEvent);
                lastEventIndexProcessed.Set(decodedEventWrapper.Timestamp, decodedEventWrapper.TimestampDifferentiator);
            });

            var onProcessingPeriodStopActionForFastWriter = new Action<string, bool, string>((traceFileName, isActiveEtl, traceFileSubFolder) =>
            {
                if (slow.HasValue && slow.Value)
                {
                    Thread.Sleep(2000);
                }

                fileSink.Close();
                if (fileSink.WriteStatistics.EventsWritten > 0)
                {
                    var additionalSuffix = (slow.HasValue) ? (slow.Value ? "_slow" : "_fast") : (string.Empty);
                    var destFileName = CreateDestinationFileName(traceFileName, additionalSuffix, isActiveEtl, lastEventIndexProcessed);
                    File.Move(fileSink.TempFileName, Path.Combine(logDirectory, "output", destFileName));
                    globalLastEventIndexProcessed.Set(lastEventIndexProcessed.Timestamp, lastEventIndexProcessed.TimestampDifferentiator);
                }
                else
                {
                    fileSink.Delete();
                }
            });

            var dcaInMemoryConsumer = TestUtility.MockRepository.Create<IDcaInMemoryConsumer>();
            dcaInMemoryConsumer.Setup(c => c.ConsumerProcessTraceEventAction)
                               .Returns(consumerProcessTraceEventActionForFastWriter);

            dcaInMemoryConsumer.Setup(c => c.MaxIndexAlreadyProcessed)
                               .Returns(maxIndexAlreadyProcessedForFastWriter);

            dcaInMemoryConsumer.Setup(c => c.OnProcessingPeriodStart(It.IsAny<string>(), It.IsAny<bool>(), It.IsAny<string>()))
                               .Callback((string traceFileName, bool isActiveEtl, string traceFileSubFolder) => onProcessingPeriodStartActionForFastWriter(traceFileName, isActiveEtl, traceFileSubFolder));

            dcaInMemoryConsumer.Setup(c => c.OnProcessingPeriodStop(It.IsAny<string>(), It.IsAny<bool>(), It.IsAny<string>()))
                               .Callback((string traceFileName, bool isActiveEtl, string traceFileSubFolder) => onProcessingPeriodStopActionForFastWriter(traceFileName, isActiveEtl, traceFileSubFolder));

            return dcaInMemoryConsumer.Object;
        }
    }
}