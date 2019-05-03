// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Fabric.Dca;
    using System.IO;
    using System.Threading;

    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;

    /// <summary>
    /// Provides unit tests for the <see cref="EtlToCsvFileWriter"/> class.
    /// </summary>
    [TestClass]
    public class EtlToCsvFileWriterTests
    {
        private const string TestLogSourceId = "EtlToCsvFileWriterTests";
        private const string TestNodeId = "abc1";
        private const string TestCsvFolder = "TestCsvFolder";

        private static readonly TimeSpan TestTimeout = TimeSpan.FromSeconds(5);

        private readonly string[] testCsvFileNames =
        {
            "a_b_c_OldDoneConvertedLog.dtr",
            "d_e_f_DoneConvertedLog.dtr",
            "g_h_i_PendingConvertedLog.dtr"
        };
        private const string TestContents = "fake trace contents";

        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            Utility.TraceSource = new ErrorAndWarningFreeTraceEventSource();
            try
            {
                Directory.Delete(TestCsvFolder, true);
            }
            catch (DirectoryNotFoundException)
            {
            }

            Utility.LogDirectory = Environment.CurrentDirectory; // Not used other than it needs to resolve
            HealthClient.Disabled = true;
            TraceTextFileSink.SetPath("FabricDCATests.trace");
        }

        [TestMethod]
        public void TestCsvNotDeletedOnDiskFull()
        {
            var mockTraceSourceFactory = new Mock<ITraceEventSourceFactory>(MockBehavior.Strict);
            mockTraceSourceFactory
                .Setup(tsf => tsf.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA))
                .Returns(new ErrorAndWarningFreeTraceEventSource());

            var mockConfigReader = new Mock<IEtlToCsvFileWriterConfigReader>(MockBehavior.Strict);
            mockConfigReader
                .Setup(cr => cr.IsDtrCompressionDisabledGlobally())
                .Returns(false);
            mockConfigReader
                .Setup(cr => cr.GetDtrDeletionAge())
                .Returns(TimeSpan.FromMinutes(1));

            var mockEtlProducer = new Mock<IEtlProducer>(MockBehavior.Strict);
            mockEtlProducer
                .Setup(ep => ep.IsProcessingWindowsFabricEtlFiles())
                .Returns(true);

            mockEtlProducer
                .Setup(ep => ep.HasEtlFileBeenFullyProcessed(EtlToCsvFileWriter.GetEtlFileNameFromTraceFileName(this.testCsvFileNames[0])))
                .Returns(true);
            mockEtlProducer
                .Setup(ep => ep.HasEtlFileBeenFullyProcessed(EtlToCsvFileWriter.GetEtlFileNameFromTraceFileName(this.testCsvFileNames[1])))
                .Returns(true);
            mockEtlProducer
                .Setup(ep => ep.HasEtlFileBeenFullyProcessed(EtlToCsvFileWriter.GetEtlFileNameFromTraceFileName(this.testCsvFileNames[2])))
                .Returns(false);

            var diskSpaceManager = new DiskSpaceManager(() => 1 << 30, () => 0, () => 80, TimeSpan.FromMilliseconds(5));
            var passCompletedEvent = new ManualResetEvent(false);
            var fileDeletedEvent = new ManualResetEvent(false);
            var testSetupEvent = new ManualResetEvent(false);
            diskSpaceManager.GetAvailableSpace = d =>
            {
                testSetupEvent.WaitOne();
                passCompletedEvent.Reset();
                fileDeletedEvent.Set();
                return 1 << 30; // 1GB
            };
            diskSpaceManager.RetentionPassCompleted += () => { passCompletedEvent.Set(); };

            var writer = new EtlToCsvFileWriter(
                mockTraceSourceFactory.Object,
                TestLogSourceId,
                TestNodeId,
                TestCsvFolder,
                false,
                diskSpaceManager,
                mockConfigReader.Object);
            writer.SetEtlProducer(mockEtlProducer.Object);

            File.WriteAllText(Path.Combine(TestCsvFolder, this.testCsvFileNames[0]), TestContents);
            File.SetLastWriteTimeUtc(Path.Combine(TestCsvFolder, this.testCsvFileNames[0]), DateTime.UtcNow - TimeSpan.FromHours(2));
            File.WriteAllText(Path.Combine(TestCsvFolder, this.testCsvFileNames[1]), TestContents);
            File.WriteAllText(Path.Combine(TestCsvFolder, this.testCsvFileNames[2]), TestContents);

            testSetupEvent.Set();

            Assert.IsTrue(fileDeletedEvent.WaitOne(TestTimeout), "File delete should happen within timeout.");
            Assert.IsTrue(passCompletedEvent.WaitOne(TestTimeout), "A retention pass should happen within timeout.");

            // Wait for a second completion to ensure both local and global policies get applied.
            passCompletedEvent.Reset();
            Assert.IsTrue(passCompletedEvent.WaitOne(TestTimeout), "A retention pass should happen within timeout.");

            Assert.IsFalse(File.Exists(Path.Combine(TestCsvFolder, this.testCsvFileNames[0])), "File should be deleted by local policy.");
            Assert.IsTrue(File.Exists(Path.Combine(TestCsvFolder, this.testCsvFileNames[1])), "File is not old enough to be deleted.");
            Assert.IsTrue(File.Exists(Path.Combine(TestCsvFolder, this.testCsvFileNames[2])), "File is not old enough to be deleted.");
        }
    }
}