// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsUploaderTest 
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Threading;

    using FabricDCA;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    [TestClass]
    public class MdsUploaderTest
    {
        // Members that are not test specific
        private TestContext testContext;
        public TestContext TestContext
        {
            get { return testContext; }
            set { testContext = value; }
        }

        internal const string TraceType = "MdsTableNameConflictTest";
        private const string TestFabricNodeId = "NodeId";
        private const string TestFabricNodeName = "NodeName";
        private const string LogFolderName = "log";
        private const string WorkFolderName = "work";
        private const string TestConfigSectionName1 = "TestConfigSection1";
        private const string TestConfigSectionName2 = "TestConfigSection2";
        private const string TestConfigSectionName3 = "TestConfigSection3";
        private const string TestConfigSectionName4 = "TestConfigSection4";
        private const string configFileName = "MdsTableNameConflictTest.cfg";
        private const string configFileContents =
        @"
[Trace/File]
Level = 5
Path = MdsUploader.TestConflict
Option = m

[{0}]
IsEnabled = true
DirectoryName = {4}
TableName = {6}

[{1}]
IsEnabled = true
DirectoryName = {4}
TableName = {6}

[{2}]
IsEnabled = true
DirectoryName = {4}
TableName = {7}

[{3}]
IsEnabled = true
DirectoryName = {5}
TableName = {6}
    ";
        private const int DtrReadTimerScaleDown = 30;
        private const int UploadIntervalMinutes = 2;
        private const string DirName1 = "Dir1";
        private const string DirName2 = "Dir2";
        private const string TableName1 = "TestTable1";
        private const string TableName2 = "TestTable2";
        private const string TaskNamePrefix = "Task";
        private const string EventTypePrefix = "Event";
        private const string IdPrefix = "Id";
        private const string TextPrefix = "foo";

        private static string testDataDirectory;
        private static string etwCsvFolder;
        private static string EtwCsvFolder
        {
            get
            {
                return Interlocked.CompareExchange(ref etwCsvFolder, null, null);
            }

            set
            {
                Interlocked.Exchange(ref etwCsvFolder, value);
            }
        }

        internal static AutoResetEvent StartDtrRead = new AutoResetEvent(false);
        internal static AutoResetEvent DtrReadCompleted = new AutoResetEvent(false);
        internal static ManualResetEvent EndOfTest = new ManualResetEvent(false);

        [ClassInitialize]
        public static void MdsTableNameConflictTestSetup(TestContext testContext)
        {
            // Create the test data directory
            const string testDataDirectoryName = "MdsUploader.TestConflict.Data";
            testDataDirectory = Path.Combine(Directory.GetCurrentDirectory(), testDataDirectoryName);
            Directory.CreateDirectory(testDataDirectory);

            // Create directories for tables
            string dir1 = Path.Combine(testDataDirectory, DirName1);
            string dir2 = Path.Combine(testDataDirectory, DirName2);
            Directory.CreateDirectory(dir1);
            Directory.CreateDirectory(dir2);

            // Create the config file for the test
            string configFileFullPath = Path.Combine(
                                     testDataDirectory,
                                     configFileName);
            string configFileContentsFormatted = String.Format(
                                                     CultureInfo.InvariantCulture,
                                                     configFileContents,
                                                     TestConfigSectionName1,
                                                     TestConfigSectionName2,
                                                     TestConfigSectionName3,
                                                     TestConfigSectionName4,
                                                     dir1,
                                                     dir2,
                                                     TableName1,
                                                     TableName2);
            byte[] configFileBytes = Encoding.ASCII.GetBytes(configFileContentsFormatted); 
            File.WriteAllBytes(configFileFullPath, configFileBytes);
            Environment.SetEnvironmentVariable("FabricConfigFileName", configFileFullPath);

            // Initialize config store
            Utility.InitializeConfigStore((IConfigStoreUpdateHandler)null);

            Utility.InitializeTracing();
            Utility.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

            // Create the log and work directories
            Utility.LogDirectory = Path.Combine(testDataDirectory, LogFolderName);
            Utility.InitializeWorkDirectory();
        }

        private bool CreateMdsUploader(string sectionName, out MdsEtwEventUploader uploader)
        {
            // Once Uploader is constructed DiskSpaceManager is no longer needed.
            using (var dsm = new DiskSpaceManager())
            {
                ConfigReader.AddAppConfig(Utility.WindowsFabricApplicationInstanceId, null);
                var initParam = new ConsumerInitializationParameters(
                    Utility.WindowsFabricApplicationInstanceId,
                    sectionName,
                    TestFabricNodeId,
                    TestFabricNodeName,
                    Utility.LogDirectory,
                    Utility.DcaWorkFolder,
                    dsm);
                uploader = new MdsEtwEventUploader(initParam);
                EtwCsvFolder = uploader.EtwCsvFolder;
                return true;
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DifferentDirectoryAndTable()
        {
            bool result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event at the start of the test");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader1;
            result = CreateMdsUploader(TestConfigSectionName3, out uploader1);
            Verify.IsTrue(EtwCsvFolder != null, "First uploader settings were acceptable");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader2;
            result = CreateMdsUploader(TestConfigSectionName4, out uploader2);
            Verify.IsTrue(EtwCsvFolder != null, "Second uploader settings were acceptable");

            result = EndOfTest.Set();
            Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

            uploader1.Dispose();
            uploader2.Dispose();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DifferentDirectorySameTable()
        {
            bool result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event at the start of the test");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader1;
            result = CreateMdsUploader(TestConfigSectionName2, out uploader1);
            Verify.IsTrue(EtwCsvFolder != null, "First uploader settings were acceptable");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader2;
            result = CreateMdsUploader(TestConfigSectionName4, out uploader2);
            Verify.IsTrue(EtwCsvFolder != null, "Second uploader settings were acceptable");

            result = EndOfTest.Set();
            Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

            uploader1.Dispose();
            uploader2.Dispose();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SameDirectoryDifferentTable()
        {
            bool result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event at the start of the test");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader1;
            result = CreateMdsUploader(TestConfigSectionName2, out uploader1);
            Verify.IsTrue(EtwCsvFolder != null, "First uploader settings were acceptable");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader2;
            result = CreateMdsUploader(TestConfigSectionName3, out uploader2);
            Verify.IsTrue(EtwCsvFolder != null, "Second uploader settings were acceptable");

            result = EndOfTest.Set();
            Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

            uploader1.Dispose();
            uploader2.Dispose();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SameDirectorySameTableOverlap()
        {
            bool result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event at the start of the test");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader1;
            result = CreateMdsUploader(TestConfigSectionName1, out uploader1);
            Verify.IsTrue(EtwCsvFolder != null, "First uploader settings were acceptable");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader2;
            result = CreateMdsUploader(TestConfigSectionName2, out uploader2);
            Verify.IsFalse(EtwCsvFolder != null, "Second uploader settings were acceptable");

            result = EndOfTest.Set();
            Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

            uploader1.Dispose();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SameDirectorySameTableNoOverlap()
        {
            bool result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event for first uploader");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader1;
            result = CreateMdsUploader(TestConfigSectionName1, out uploader1);
            Verify.IsTrue(EtwCsvFolder != null, "First uploader settings were acceptable");

            uploader1.Dispose();

            result = EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset event for second uploader");

            EtwCsvFolder = null;
            MdsEtwEventUploader uploader2;
            result = CreateMdsUploader(TestConfigSectionName2, out uploader2);
            Verify.IsTrue(EtwCsvFolder != null, "Second uploader settings were acceptable");

            result = EndOfTest.Set();
            Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

            uploader2.Dispose();
        }
    }
}