// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.IO.Compression;
    using System.IO.Packaging;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using DcaFileShare = Microsoft.ServiceFabric.StandaloneLogCollector.DcaLogCollector.DcaFileShare;

    [TestClass]
    internal class DcaLogCollectorTest : DcaLogCollector
    {
        private static readonly string[] fileShares = new string[]
        {
            Environment.ExpandEnvironmentVariables("%systemdrive%\\hiahiaFabricLogsShare"),
            Environment.ExpandEnvironmentVariables("%systemdrive%\\hiahiaPerfCountersShare"),
        };

        public DcaLogCollectorTest(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        public DcaLogCollectorTest()
            : base (null)
        {
        }  

        [TestInitialize]
        public void TestInitialize()
        {
            foreach (string fileShare in DcaLogCollectorTest.fileShares)
            {
                if (!Directory.Exists(fileShare))
                {
                    Directory.CreateDirectory(fileShare);
                }
            }
        }

        [TestCleanup]
        public void TestCleanup()
        {
            foreach (string fileShare in DcaLogCollectorTest.fileShares)
            {
                if (Directory.Exists(fileShare))
                {
                    Directory.Delete(fileShare, recursive: true);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            DcaLogCollector collector = new DcaLogCollector(null);
            Assert.IsTrue(collector.DcaFileShares.Length > 0);
            Assert.IsNotNull(collector.LogIndex);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryGetDcaFileShareLocationTest()
        {
            TestUtility.TestProlog();

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                DcaLogCollector collector = new DcaLogCollector(traceLogger);

                string fileShareLocation;
                string manifestXml = this.TryGetClusterManifest();
                Assert.IsTrue(collector.TryGetDcaFileShareLocation(manifestXml, "ns:ClusterManifest/ns:FabricSettings/ns:Section[@Name='FileShareWinFabEtw']", out fileShareLocation));
                Assert.IsFalse(string.IsNullOrWhiteSpace(fileShareLocation));

                Assert.IsTrue(collector.TryGetDcaFileShareLocation(manifestXml, "ns:ClusterManifest/ns:FabricSettings/ns:Section[@Name='FileShareWinFabPerfCtr']", out fileShareLocation));
                Assert.IsFalse(string.IsNullOrWhiteSpace(fileShareLocation));
            }

            File.Delete(traceLogPath);

            TestUtility.TestEpilog();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructPackagePathTest()
        {
            string result = DcaLogCollector.ConstructPackagePath("11", "22");
            Assert.AreEqual("11//22", result);

            result = DcaLogCollector.ConstructPackagePath("11", "22", "33");
            Assert.AreEqual("11//22//33", result);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CollectLogsDryRunTest()
        {
            this.InternalCollectLogsDryRunTest(new string[] { "-scope", "node", "-mode", "collect" });
            this.InternalCollectLogsDryRunTest(new string[] { "-scope", "cluster", "-mode", "collect" });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DcaFileShareConstructorTest()
        {
            CollectLogsDelegate del = (x, y) => {};
            string xPath = "hiaPath";
            string propertyName = "hiaProperty";

            DcaFileShare fileShare = new DcaFileShare(xPath, del, propertyName);
            Assert.AreEqual(xPath, fileShare.RootXPath);
            Assert.AreEqual(del, fileShare.LogCollectionDelegate);
            Assert.AreEqual(propertyName, fileShare.ContentIndexPropertyName);

            fileShare = new DcaFileShare(xPath, del);
            Assert.AreEqual(xPath, fileShare.RootXPath);
            Assert.AreEqual(del, fileShare.LogCollectionDelegate);
            Assert.AreEqual(null, fileShare.ContentIndexPropertyName);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetPropertyNameTest()
        {
            string propertyName = DcaLogCollector.GetPropertyName(() => this.LogIndex.FabricLogDirectory);
            Assert.AreEqual("FabricLogDirectory", propertyName);
        }

        internal void InternalCollectLogsDryRunTest(string[] args)
        {
            TestUtility.TestProlog(args);

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                DcaLogCollectorTest collector = new DcaLogCollectorTest(traceLogger);
                collector.CollectLogs();
                Assert.IsNotNull(collector.LogIndex.FabricLogDirectory);
                Assert.IsNotNull(collector.LogIndex.PerfCounterDirectory);
                Assert.IsNull(collector.LogIndex.MiscellaneousLogsFileName);
            }

            File.Delete(traceLogPath);

            TestUtility.TestEpilog();
        }

        internal override string TryGetClusterManifest()
        {
            return File.ReadAllText(Path.Combine(TestUtility.TestDirectory, "MyTestClusterManifest3.xml"));
        }

        internal override string TryGetCurrentNodeName()
        {
            return "hiahiaNode";
        }
    }
}