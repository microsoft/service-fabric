// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using NetFwTypeLib;

    [TestClass]
    internal class FirewallInfoCollectorTest : FirewallInfoCollector
    {
        public FirewallInfoCollectorTest()
            : base(null)
        {
        }

        public FirewallInfoCollectorTest(TraceLogger traceLogger)
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
                FirewallInfoCollectorTest collector = new FirewallInfoCollectorTest(traceLogger);
                collector.CollectLogs();

                Assert.AreEqual(1, collector.CollectionResult.Count);
                Assert.AreEqual(1, collector.GeneratedLogs.Count);
                string outputFilePath = Program.Config.WorkingDirectoryPath + "\\Firewall.xml";
                Assert.IsTrue(File.Exists(outputFilePath));
                File.Delete(outputFilePath);
            }

            File.Delete(traceLogPath);
            TestUtility.TestEpilog();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void LoadFirewallRulesTest()
        {
            TestUtility.TestProlog();

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                FirewallInfoCollectorTest collector = new FirewallInfoCollectorTest(traceLogger);

                // long as no exception is thrown
                IEnumerable<INetFwRule> rules = collector.LoadFirewallRules();
                Assert.IsNotNull(rules);
            }

            File.Delete(traceLogPath);
            TestUtility.TestEpilog();
        }

        internal override bool IsServiceFabricRule(NetFwTypeLib.INetFwRule rule)
        {
            return true;
        }
    }
}