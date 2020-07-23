// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    using ParameterNames = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.ParameterNames;

    [TestClass]
    public class ProgramParameterDefinitionsTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void IsHelpModeTest()
        {
            Assert.IsFalse(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string,string>()));
            Assert.IsFalse(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string, string>() { { "?", "false" } }));
            Assert.IsFalse(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string, string>() { { "StartUtcTime", "01/01/2017" } }));

            Assert.IsTrue(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string, string>() { {"help", "true"} }));
            Assert.IsTrue(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string, string>() { { "?", "true" } }));
            Assert.IsTrue(ProgramParameterDefinitions.IsHelpMode(new Dictionary<string, string>() { {"help", "true"}, { "?", "true" } }));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ShowHelpTest()
        {
            // long as no exception is thrown, we're fine.
            ProgramParameterDefinitions.ShowHelp();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateDateTimeTest()
        {
            DateTime result;
            Assert.IsFalse(ProgramParameterDefinitions.ValidateDateTime(new Dictionary<string, string>() { { "EndUtcTime", "hiahia" } }, ParameterNames.EndUtcTime, DateTime.MaxValue, out result));

            Assert.IsTrue(ProgramParameterDefinitions.ValidateDateTime(new Dictionary<string, string>(), ParameterNames.EndUtcTime, DateTime.MaxValue, out result));
            Assert.AreEqual(DateTime.MaxValue, result);
            Assert.IsTrue(ProgramParameterDefinitions.ValidateDateTime(new Dictionary<string, string>() { { "StartUtcTime", "01/01/2017" } }, ParameterNames.EndUtcTime, DateTime.MaxValue, out result));
            Assert.AreEqual(DateTime.MaxValue, result);
            Assert.IsTrue(ProgramParameterDefinitions.ValidateDateTime(new Dictionary<string, string>() { { "EndUtcTime", "01/01/2017" } }, ParameterNames.EndUtcTime, DateTime.MaxValue, out result));
            Assert.AreEqual(new DateTime(2017, 1, 1), result);

            Thread.CurrentThread.CurrentCulture = new CultureInfo("en-IN");
            Assert.IsTrue(ProgramParameterDefinitions.ValidateDateTime(new Dictionary<string, string>() { { "StartUtcTime", "01/25/2017" } }, ParameterNames.StartUtcTime, DateTime.MaxValue, out result));
            Assert.AreEqual(new DateTime(2017, 1, 25), result);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateOutputPathTest()
        {
            string longOutputPath = @"c:\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { "Output", longOutputPath }, { "AcceptEula", "true" }, { "Mode", "Collect" } }));
            Assert.IsFalse(ProgramParameterDefinitions.ValidateOutputPath(new Dictionary<string, string>() { { "Output", longOutputPath }, { "Mode", "Collect" } }));

            string defaultPath = Utility.GenerateProgramDirectoryPath(new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName);
            Assert.AreEqual(defaultPath.Length <= 80, ProgramParameterDefinitions.ValidateOutputPath(new Dictionary<string, string>()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateTest()
        {
            string existingDirectoryPath = TestUtility.TestDirectory + "\\exist";
            Directory.CreateDirectory(existingDirectoryPath);
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, existingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, existingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "upload" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, existingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collectandupload" }, { ParameterNames.StorageConnectionString, "hiahia" } }));
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, existingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "upload" }, { ParameterNames.StorageConnectionString, "hiahia" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, existingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "upload" }, { ParameterNames.StorageConnectionString, "hiahia" }, { ParameterNames.StartUtcTime, "01/01/2017" } }));
            Directory.Delete(existingDirectoryPath);

            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.StartUtcTime, "hiahia" }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.EndUtcTime, "lolo" }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.StartUtcTime, "01/01/2017" }, { ParameterNames.EndUtcTime, "01/01/2016" }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Scope, "node1" }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));

            string nonExistingDirectoryPath = TestUtility.TestDirectory + "\\nonExist";
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, nonExistingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Scope, "cluster" }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" } }));
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, nonExistingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collectandupload" }, { ParameterNames.StorageConnectionString, "hiahia" } }));

            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>()
            {
                { ParameterNames.Output, nonExistingDirectoryPath },
                { ParameterNames.StartUtcTime, "01/01/2016" },
                { ParameterNames.EndUtcTime, "01/01/2017" },
                { ParameterNames.IncludeLeaseLogs, "true" },
                { ParameterNames.Scope, "node" },
                { ParameterNames.AcceptEula, "true" },
                { ParameterNames.StorageConnectionString, "hiahia" },
                { ParameterNames.IncludeCrashDumps, "true" },
            }));

            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "upload" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.Output, nonExistingDirectoryPath }, { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "upload" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>()
            {
                { ParameterNames.Output, nonExistingDirectoryPath },
                { ParameterNames.StartUtcTime, "01/01/2016" },
                { ParameterNames.EndUtcTime, "01/01/2017" },
                { ParameterNames.IncludeLeaseLogs, "true" },
                { ParameterNames.Scope, "node" },
                { ParameterNames.AcceptEula, "true" },
                { ParameterNames.Mode, "upload" },
                { ParameterNames.IncludeCrashDumps, "true" },
            }));

            string existingConfig = TestUtility.TestDirectory + "\\MyClusterConfig.json";
            string nonexistentConfig = TestUtility.TestDirectory + "\\nonexistentConfig.json";
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" }, { ParameterNames.ClusterConfigFilePath, existingConfig } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { ParameterNames.AcceptEula, "true" }, { ParameterNames.Mode, "collect" }, { ParameterNames.ClusterConfigFilePath, nonexistentConfig } }));
        }
    }
}