// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Fabric.Common;
using System.Fabric.Dca.EtlConsumerHelper;
using System.IO;
using System.Linq;
using System.Threading;
using Moq;

namespace FabricDCA.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class BootstrapTraceProcessorTests
    {
        private const string TestLogSourceId = "BootstrapTraceProcessorTests";
        private const string DefaultWorkDirectory = TestLogSourceId + ".Data";
        private const string TestFileName = "FabricSetup-foo.trace";
        private static readonly TimeSpan DefaultScanInterval = TimeSpan.FromMilliseconds(5);

        [ClassInitialize]
        public static void ClassSetup(object testContext)
        {
            if (FabricDirectory.Exists(DefaultWorkDirectory))
            {
                FabricDirectory.Delete(DefaultWorkDirectory, true);
            }
        }

        [TestMethod]
        public void BootstrapTraceProcessorMoveOnWritableTest()
        {
            var mockTraceOutputWriter = TestUtility.MockRepository.Create<ITraceOutputWriter>();

            var startEvent = new ManualResetEvent(false);
            var stopEvent = new ManualResetEvent(false);
            mockTraceOutputWriter.Setup(w => w.OnBootstrapTraceFileScanStart()).Callback(() => { startEvent.Set(); });
            mockTraceOutputWriter.Setup(w => w.OnBootstrapTraceFileScanStop()).Callback(() => { stopEvent.Set(); });

            var traceDirectory = Path.Combine(DefaultWorkDirectory, "MoveOnWritableTest.TraceDirectory");
            var markerFileDirectory = Path.Combine(DefaultWorkDirectory, "MoveOnWritableTest.MarkerFileDirectory");
            var testFilename = Path.Combine(traceDirectory, TestFileName);
            var markerFilename = Path.Combine(markerFileDirectory, TestFileName);
            mockTraceOutputWriter.Setup(w => w.OnBootstrapTraceFileAvailable(testFilename)).Returns(true);

            FabricDirectory.CreateDirectory(traceDirectory);
            FabricDirectory.CreateDirectory(markerFileDirectory);

            var processor = new BootstrapTraceProcessor(
                traceDirectory,
                markerFileDirectory,
                new [] { mockTraceOutputWriter.Object },
                DefaultScanInterval,
                new ErrorAndWarningFreeTraceEventSource(), 
                TestLogSourceId);

            processor.Start();

            var stream = File.Open(testFilename, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read);
            stream.WriteByte(0);

            // Wait for at least one pass
            startEvent.Reset();
            startEvent.WaitOne();
            stopEvent.Reset();
            stopEvent.WaitOne();

            // Assert marker directory empty
            Assert.AreEqual(0, Directory.EnumerateFiles(markerFileDirectory).Count(), "File should not be copied to marker directory until writeable.");

            stream.Close();

            // Wait for at least one pass
            startEvent.Reset();
            startEvent.WaitOne();
            stopEvent.Reset();
            stopEvent.WaitOne();

            // Assert marker directory empty
            Assert.AreEqual(
                markerFilename, 
                Directory.EnumerateFiles(markerFileDirectory).First(), 
                "File should be copied to marker directory when closed.");
        }
    }
}