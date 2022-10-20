// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    internal class RegistryLogCollectorTest : RegistryLogCollector
    {
        public RegistryLogCollectorTest()
            : base(null)
        {
        }

        public RegistryLogCollectorTest(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CollectLogsTest()
        {
            TestUtility.TestProlog();

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                RegistryLogCollector collector = new RegistryLogCollectorTest(traceLogger);
                collector.CollectLogs();

                Assert.AreEqual(1, collector.CollectionResult.Count);
                Assert.AreEqual(1, collector.GeneratedLogs.Count);
                string outputFilePath = Program.Config.WorkingDirectoryPath + "\\Registry.xml";
                Assert.IsTrue(File.Exists(outputFilePath));
                File.Delete(outputFilePath);
            }

            File.Delete(traceLogPath);
            TestUtility.TestEpilog();
        }

        internal override string RegistryPath
        {
            get
            {
                return "SYSTEM\\CurrentControlSet\\Control";
            }
        }
    }
}