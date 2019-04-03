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
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    using ParameterNames = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.ParameterNames;
    using Scopes = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.Scopes;
    using Modes = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.Modes;

    [TestClass]
    public class EnvironmentVariablesTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            string expectedOutput = "hiahia";
            DateTime expectedStartTime = new DateTime(1, 1, 1);
            DateTime expectedEndTime = new DateTime(2, 2, 2);
            EnvironmentVariables variable = new EnvironmentVariables(expectedOutput, expectedStartTime, expectedEndTime, true, Scopes.Cluster, true, Modes.Upload, "lala", "c:\\SFPackage\\ClusterConfig.json", true);
            Assert.AreEqual(expectedOutput, variable.OutputDirectoryPath);
            Assert.AreEqual(expectedStartTime, variable.StartUtcTime);
            Assert.AreEqual(expectedEndTime, variable.EndUtcTime);
            Assert.AreEqual(true, variable.IncludeLeaseLogs);
            Assert.AreEqual(Scopes.Cluster, variable.Scope);
            Assert.AreEqual(true, variable.AcceptEula);
            Assert.AreEqual(Modes.Upload, variable.Mode);
            Assert.AreEqual("lala", variable.StorageConnectionString);
            Assert.AreEqual("c:\\SFPackage\\ClusterConfig.json", variable.ClusterConfigFilePath);
            Assert.AreEqual(true, variable.IncludeCrashDumps);
            Directory.Delete(variable.WorkingDirectoryPath);
            Directory.Delete(variable.OutputDirectoryPath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateFromProgramParametersTest()
        {
            Dictionary<string, string> parsedValues = new Dictionary<string, string>();
            EnvironmentVariables variable;

            variable = EnvironmentVariables.CreateFromProgramParameters(parsedValues);
            Assert.IsFalse(string.IsNullOrWhiteSpace(variable.OutputDirectoryPath));
            Assert.IsTrue(variable.StartUtcTime < variable.EndUtcTime);
            Assert.IsTrue(variable.StartUtcTime > variable.EndUtcTime - TimeSpan.FromHours(2));
            Assert.IsTrue(variable.EndUtcTime < DateTime.UtcNow + TimeSpan.FromHours(1));
            Assert.IsFalse(variable.IncludeLeaseLogs);
            Assert.IsFalse(variable.IncludeCrashDumps);
            Assert.AreEqual(Scopes.Cluster, variable.Scope);
            Assert.AreEqual(false, variable.AcceptEula);
            Assert.AreEqual(Modes.CollectAndUpload, variable.Mode);
            TestUtility.TestEpilog();

            string expectedOutput = "c:\\hiahia.zip";
            DateTime expectedEndTime = new DateTime(2, 2, 2);
            parsedValues = new Dictionary<string, string>()
            {
                { ParameterNames.Output, expectedOutput },
                { ParameterNames.EndUtcTime, expectedEndTime.ToString() },
                { ParameterNames.Scope, "Cluster" },
            };
            variable = EnvironmentVariables.CreateFromProgramParameters(parsedValues);
            Assert.AreEqual(expectedOutput, variable.OutputDirectoryPath);
            Assert.IsTrue(variable.StartUtcTime < variable.EndUtcTime);
            Assert.IsTrue(variable.StartUtcTime > variable.EndUtcTime - TimeSpan.FromHours(2));
            Assert.AreEqual(expectedEndTime.ToUniversalTime(), variable.EndUtcTime);
            Assert.IsFalse(variable.IncludeLeaseLogs);
            Assert.IsFalse(variable.IncludeCrashDumps);
            Assert.AreEqual(Scopes.Cluster, variable.Scope);
            Assert.AreEqual(false, variable.AcceptEula);
            Assert.AreEqual(Modes.CollectAndUpload, variable.Mode);
            Assert.IsNull(variable.FabricDataRoot);
            TestUtility.TestEpilog();

            DateTime expectedStartTime = new DateTime(1, 1, 1);
            parsedValues = new Dictionary<string, string>()
            {
                { ParameterNames.StartUtcTime, expectedStartTime.ToString() },
                { ParameterNames.IncludeLeaseLogs, "true" },
                { ParameterNames.Scope, Scopes.Node.ToString() },
                { ParameterNames.AcceptEula, "true" },
                { ParameterNames.Mode, "collectandupload" },
                { ParameterNames.ClusterConfigFilePath, "c:\\SFPackage\\ClusterConfig.json" },
                { ParameterNames.IncludeCrashDumps, "true" },
            };
            variable = EnvironmentVariables.CreateFromProgramParameters(parsedValues);
            Assert.IsFalse(string.IsNullOrWhiteSpace(variable.OutputDirectoryPath));
            Assert.IsTrue(variable.StartUtcTime < variable.EndUtcTime);
            Assert.IsTrue(variable.StartUtcTime > DateTime.UtcNow - TimeSpan.FromHours(2));
            Assert.IsTrue(variable.EndUtcTime < DateTime.UtcNow + TimeSpan.FromHours(1));
            Assert.IsTrue(variable.IncludeLeaseLogs);
            Assert.IsTrue(variable.IncludeCrashDumps);
            Assert.AreEqual(Scopes.Node, variable.Scope);
            Assert.AreEqual(true, variable.AcceptEula);
            Assert.AreEqual(Modes.CollectAndUpload, variable.Mode);
            Assert.AreEqual("c:\\SFPackage\\ClusterConfig.json", variable.ClusterConfigFilePath);
            TestUtility.TestEpilog();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryGetValuesFromConfigFileTest()
        {
            Dictionary<string, string> parsedValues = new Dictionary<string, string>();
            EnvironmentVariables variable;

            string ConfigPath = TestUtility.TestDirectory + "\\MyClusterConfig.json";
            parsedValues = new Dictionary<string, string>()
            {
                { ParameterNames.ClusterConfigFilePath, ConfigPath },
            };
            variable = EnvironmentVariables.CreateFromProgramParameters(parsedValues);
            Assert.AreEqual(ConfigPath, variable.ClusterConfigFilePath);
            Assert.IsNull(variable.FabricDataRoot);
            Assert.IsNull(variable.FabricLogRoot);
            Assert.IsNull(variable.Nodes);

            // returns false because data root path doesn't exist in the test
            Assert.IsFalse(variable.TryGetValuesFromConfigFile());
            Assert.AreEqual("D:\\ProgramData\\CustomDataRoot", variable.FabricDataRoot);
            Assert.AreEqual("D:\\ProgramData\\CustomLogRoot", variable.FabricLogRoot);
            Assert.IsNotNull(variable.Nodes);

            string nodeName = "vm";
            string nodeIp = "Machine";
            int index = 0;
            foreach (var node in variable.Nodes)
            {
                Assert.AreEqual(node.Item1, nodeName + index);
                Assert.AreEqual(node.Item2, nodeIp + index);
                index++;
            }

            TestUtility.TestEpilog();
        }
    }
}