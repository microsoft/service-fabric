// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.IO;
    using System.IO.Compression;
    using System.IO.Packaging;
    using System.Linq;
    using System.Numerics;
    using System.Text;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class FileLogCollectorTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DumpToFileTest()
        {
            string content = "hiahia\r\nlolo";
            string filePath = TestUtility.TestDirectory + "\\DumpToFileTestTemp.txt";
            FileLogCollector.DumpToFile(content, filePath);

            using (FileStream file = File.Open(filePath, FileMode.Open, FileAccess.Read))
            {
                using (StreamReader reader = new StreamReader(file))
                {
                    Assert.AreEqual(content, reader.ReadToEnd());
                }
            }

            File.Delete(filePath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GenerateMethodsTest()
        {
            TestUtility.TestProlog();

            string cmPath = FileLogCollector.GenerateClusterManifestPath();
            Assert.IsTrue(cmPath.Contains("ClusterManifest.xml"));

            string appTypeName = "app1";
            string appVersion = "ver2";
            string appPath = FileLogCollector.GenerateAppManifestPath(appTypeName, appVersion);
            Assert.IsTrue(
                appPath.Contains(appTypeName) &&
                appPath.Contains(appVersion) &&
                appPath.Contains(".xml"));

            string serviceManifestName = "svc3";
            string servicePath = FileLogCollector.GenerateServiceManifestPath(appTypeName, appVersion, serviceManifestName);
            Assert.IsTrue(
                servicePath.Contains(appTypeName) &&
                servicePath.Contains(appVersion) &&
                servicePath.Contains(serviceManifestName) &&
                servicePath.Contains(".xml"));

            string ciPath = FileLogCollector.GenerateClusterInfoPath();
            Assert.IsTrue(ciPath.Contains("ClusterInfo.txt"));

            string chPath = FileLogCollector.GenerateClusterHealthPath();
            Assert.IsTrue(chPath.Contains("ClusterHealth.txt"));

            string cuPath = FileLogCollector.GenerateClusterUpgradePath();
            Assert.IsTrue(cuPath.Contains("ClusterUpgrade.txt"));

            string nodePath = FileLogCollector.GenerateNodePath();
            Assert.IsTrue(nodePath.Contains("Node.txt"));

            string appRoot = FileLogCollector.GenerateAppRoot();
            Assert.IsTrue(appRoot.Contains(Program.Config.WorkingDirectoryPath));

            string uri_1 = "localhost";
            string uri_2 = "192.168.1.1/index/service/info.html";
            Assert.IsTrue(
                FileLogCollector.GenerateFileName(uri_1) == uri_1 &&
                FileLogCollector.GenerateFileName(uri_2) == "index_service_info.html");
            
            TestUtility.TestEpilog();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SerializeObjectTest()
        {
            StringBuilder sb = new StringBuilder();
            object obj = null;

            FileLogCollector.SerializeObject(obj, sb);
            Assert.AreEqual(0, sb.Length);

            FileLogCollector.SerializeObject(1111111, sb);
            Assert.AreEqual("1111111" + Environment.NewLine, sb.ToString());
            sb.Clear();

            FileLogCollector.SerializeObject("serialize", sb);
            Assert.AreEqual("serialize" + Environment.NewLine, sb.ToString());
            sb.Clear();

            string expectedObjStr = string.Empty;
            expectedObjStr += string.Format("{0, 25} : {1}{2}", "Str", "Str", Environment.NewLine);
            expectedObjStr += string.Format("{0, 25} : {{{1}", "Lst",  Environment.NewLine);
            expectedObjStr += string.Format("{0,25}   {1} {2}", string.Empty, "Str", Environment.NewLine);
            expectedObjStr += string.Format("{0,25}   {1} {2}", string.Empty, "Str", Environment.NewLine);
            expectedObjStr += string.Format("{0, 25}   }}{1}", string.Empty, Environment.NewLine);
            expectedObjStr += string.Format("{0, 25} : {1}{2}", "Tsi", "ToStr", Environment.NewLine);

            FileLogCollector.SerializeObject(new ObjectTest(), sb);
            Assert.AreEqual(expectedObjStr, sb.ToString());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ProcessNodeTest()
        {            
            NodeList nodes = new NodeList();
            BigInteger bigint = new BigInteger(1234);

            NodeDeactivationResult nodeDeactivationResult = new NodeDeactivationResult(NodeDeactivationIntent.Invalid, NodeDeactivationStatus.None, new List<NodeDeactivationTask>(), new List<SafetyCheck>());
            nodes.Add(new Node("Node1", "123.123.123.123", "NodeType01", "3.0", "1.0", NodeStatus.Up, TimeSpan.FromMinutes(30), TimeSpan.FromMinutes(0), DateTime.Now - TimeSpan.FromMinutes(30), DateTime.Now, HealthState.Ok, true, "UpgradeDomain1", new Uri("fd:/DataCenter1/Rack1"), new NodeId(bigint, bigint), bigint, nodeDeactivationResult));
            nodes.Add(new Node("Node2", "123.123.123.123", "NodeType02", "3.0", "1.0", NodeStatus.Down, TimeSpan.FromMinutes(0), TimeSpan.FromMinutes(30), DateTime.Now, DateTime.Now - TimeSpan.FromMinutes(30), HealthState.Ok, true, "UpgradeDomain2", new Uri("fd:/DataCenter1/Rack2"), new NodeId(bigint, bigint), bigint, nodeDeactivationResult));
            nodes.Add(new Node("Node3", "123.123.123.123", "NodeType03", "3.0", "1.0", NodeStatus.Up, TimeSpan.FromMinutes(90), TimeSpan.FromMinutes(0), DateTime.Now - TimeSpan.FromMinutes(90), DateTime.Now, HealthState.Ok, true, "UpgradeDomain3", new Uri("fd:/DataCenter1/Rack3"), new NodeId(bigint, bigint), bigint, nodeDeactivationResult));

            FileLogCollector.NodeSummary nodesummary = new FileLogCollector.NodeSummary();
            foreach (Node node in nodes)
            {
                FileLogCollector.ProcessNode(node, nodesummary);
            }

            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain1"].Total);
            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain1"].Up);
            Assert.AreEqual(0, nodesummary.NodeUDS["UpgradeDomain1"].Stable);

            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain2"].Total);
            Assert.AreEqual(0, nodesummary.NodeUDS["UpgradeDomain2"].Up);
            Assert.AreEqual(0, nodesummary.NodeUDS["UpgradeDomain2"].Stable);

            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain3"].Total);
            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain3"].Up);
            Assert.AreEqual(1, nodesummary.NodeUDS["UpgradeDomain3"].Stable);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CountealthStateTest()
        {
            ApplicationList appsList = new ApplicationList();
            StringBuilder sb = new StringBuilder();
            Application app1 = new Application();
            Application app2 = new Application();
            Application app3 = new Application();
            app1.HealthState = HealthState.Ok;
            app2.HealthState = HealthState.Warning;
            app3.HealthState = HealthState.Error;

            appsList.Add(app1);
            appsList.Add(app2);
            appsList.Add(app3);

            FileLogCollector.CountHealthState(appsList, sb);

            Assert.AreEqual("OK: 1  Warning: 1  Error: 1" + Environment.NewLine, sb.ToString());
        }

        private class ObjectTest
        {
            public string Str { get; set; }
            public IList<string> Lst { get; set; }
            public ToStrImp Tsi { get; set; }

            public class ToStrImp
            {
                public override string ToString()
                {
                    return "ToStr";
                }
            }

            public ObjectTest()
            {
                Str = "Str";
                Lst = new List<string>() { "Str", "Str"};
                Tsi = new ToStrImp();
            }
        }

    }
}