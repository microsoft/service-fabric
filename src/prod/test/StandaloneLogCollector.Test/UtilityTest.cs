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

    [TestClass]
    public class UtilityTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GenerateProgramFilePathTest()
        {
            string directoryPath = TestUtility.TestDirectory;
            string ext = ".zip";
            DateTime utcNow = DateTime.UtcNow;
            string result = Utility.GenerateProgramFilePath(directoryPath, ext);

            Assert.IsTrue(result.Contains(directoryPath));
            Assert.IsTrue(result.Contains(Environment.MachineName));
            Assert.IsTrue(result.Contains(utcNow.Year.ToString()));
            Assert.IsTrue(result.Contains(utcNow.Month.ToString()));
            Assert.IsTrue(result.Contains(utcNow.Day.ToString()));
            Assert.IsTrue(result.Contains(ext));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GenerateProgramDirectoryPathTest()
        {
            string directoryPath = TestUtility.TestDirectory;
            DateTime utcNow = DateTime.UtcNow;
            string result = Utility.GenerateProgramDirectoryPath(directoryPath);

            Assert.IsTrue(result.Contains(directoryPath));
            Assert.IsTrue(result.Contains(Environment.MachineName));
            Assert.IsTrue(result.Contains(utcNow.Year.ToString()));
            Assert.IsTrue(result.Contains(utcNow.Month.ToString()));
            Assert.IsTrue(result.Contains(utcNow.Day.ToString()));
        }
    }
}