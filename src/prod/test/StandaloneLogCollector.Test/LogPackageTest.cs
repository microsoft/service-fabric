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

    [TestClass]
    public class LogPackageTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PackageFileTest()
        {
            TestUtility.TestProlog();

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                using (LogPackage logPackage = new LogPackage(Program.Config.OutputDirectoryPath, traceLogger))
                {
                    foreach (KeyValuePair<string, string> log in TestUtility.ExistingTestLogs)
                    {
                        Assert.IsTrue(logPackage.AddFile(string.Format("{0}/{1}", log.Key, log.Value), Path.Combine(TestUtility.TestDirectory, log.Value), false));
                    }
                }
            }

            string extractedDirectory = Program.Config.WorkingDirectoryPath + "\\extracted";
            string zipFilePath = Directory.GetFiles(Program.Config.OutputDirectoryPath, "*.zip", SearchOption.TopDirectoryOnly).First();
            ZipFile.ExtractToDirectory(zipFilePath, extractedDirectory);
            foreach (string fileName in TestUtility.ExistingTestLogs.Values)
            {
                Assert.IsTrue(Directory.GetFiles(extractedDirectory, fileName, SearchOption.AllDirectories).Any());
            }

            Directory.Delete(extractedDirectory, recursive: true);
            File.Delete(traceLogPath);

            TestUtility.TestEpilog();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CopyFileTest()
        {
            TestUtility.TestProlog(new string[] {"-output", TestUtility.TestDirectory + "\\myOutput", "-mode", "collect"});

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                using (LogPackage logPackage = new LogPackage(Program.Config.OutputDirectoryPath, traceLogger))
                {
                    foreach (KeyValuePair<string, string> log in TestUtility.ExistingTestLogs)
                    {
                        Assert.IsTrue(logPackage.AddFile(string.Format("{0}/{1}", log.Key, log.Value), Path.Combine(TestUtility.TestDirectory, log.Value), true));
                    }
                }
            }

            string zipFilePath = Directory.GetFiles(Program.Config.OutputDirectoryPath, "*.zip", SearchOption.TopDirectoryOnly).First();
            Assert.IsTrue(File.Exists(zipFilePath));
            foreach (string fileName in TestUtility.ExistingTestLogs.Values)
            {
                Assert.IsTrue(Directory.GetFiles(Program.Config.OutputDirectoryPath, fileName, SearchOption.AllDirectories).Any());
            }

            File.Delete(traceLogPath);

            TestUtility.TestEpilog();
        }
    }
}