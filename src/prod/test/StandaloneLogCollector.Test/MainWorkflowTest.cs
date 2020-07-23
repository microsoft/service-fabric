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
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.Tracing.Azure;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using LogCollectionResult = System.Collections.Generic.Dictionary<string, System.Collections.Generic.KeyValuePair<string, bool>>;
    using Modes = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.Modes;

    [TestClass]
    internal class MainWorkflowTest : MainWorkflow
    {
        public MainWorkflowTest()
            :base(null)
        {
        }

        public MainWorkflowTest(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ExecuteTest()
        {
            this.InternalExecuteTest(Modes.Collect);
            this.InternalExecuteTest(Modes.Upload);
            this.InternalExecuteTest(Modes.CollectAndUpload);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SerializeCollectionResultTest()
        {
            // long as there's no exception happens
            MainWorkflow.SerializeCollectionResult(new LogCollectionResult());
            MainWorkflow.SerializeCollectionResult(new LogCollectionResult()
            {
                { "/hiahia", new KeyValuePair<string, bool>("c:\\hiahia.txt", false) },
                { "/lolo", new KeyValuePair<string, bool>("c:\\hiahia2.txt", true) },
            });
        }

        internal void InternalExecuteTest(Modes mode)
        {
            string uploadOnlyDir = null;
            string sasConnectionString = "https://liphistorageaccount.blob.core.windows.net/container1?sv=2015-04-05&sr=c&sig=w0tlW6%2BaeHOR%2FIE%2FxASnaPn%2B%2BWvuWD67%2BkmNgKEFLWE%3D&se=2017-02-18T07%3A17%3A41Z&sp=rwdl";
            switch(mode)
            {
                case Modes.Collect:
                    TestUtility.TestProlog(new string[2] { "-mode", "collect" });
                    break;

                case Modes.CollectAndUpload:
                    TestUtility.TestProlog(new string[4] { "-mode", "collectandupload", "-StorageConnectionString", sasConnectionString });
                    break;

                case Modes.Upload:
                    uploadOnlyDir = Path.Combine(TestUtility.TestDirectory, "uploadonly");
                    Directory.CreateDirectory(uploadOnlyDir);
                    TestUtility.TestProlog(new string[6] { "-mode", mode.ToString(), "-StorageConnectionString", sasConnectionString, "output", uploadOnlyDir });
                    break;
            }

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            MainWorkflowTest workflow = null;
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                using (LogPackage logPackage = new LogPackage(Program.Config.OutputDirectoryPath, traceLogger))
                {
                    workflow = new MainWorkflowTest(traceLogger);
                    workflow.Execute(logPackage);
                }
            }

            if (mode != Modes.Upload)
            {
                string indexFilePath = Program.Config.OutputDirectoryPath + "\\" + ContainerIndex.IndexFileName;
                using (StreamReader reader = new StreamReader(indexFilePath))
                {
                    string content = reader.ReadToEnd();
                    ContainerIndex index;
                    Assert.IsTrue(ContainerIndex.TryDeserialize(content, out index));
                    Assert.IsNotNull(index.MiscellaneousLogsFileName);
                    Assert.IsNotNull(index.FabricLogDirectory);
                    Assert.IsNotNull(index.PerfCounterDirectory);
                }

                string extractedDirectory = Program.Config.WorkingDirectoryPath + "\\extracted";
                string zipFilePath = Directory.GetFiles(Program.Config.OutputDirectoryPath, "*.zip", SearchOption.TopDirectoryOnly).First();
                ZipFile.ExtractToDirectory(zipFilePath, extractedDirectory);
                foreach (KeyValuePair<string, string> log in TestUtility.GeneratedTestLogs)
                {
                    Assert.IsTrue(File.Exists(extractedDirectory + log.Key.Replace('/', '\\') + "\\" + log.Value));
                }

                foreach (KeyValuePair<string, string> log in TestUtility.GeneratedTestLogs)
                {
                    Assert.IsTrue(File.Exists(extractedDirectory + log.Key.Replace('/', '\\') + "\\" + log.Value));
                    Assert.IsFalse(File.Exists(Path.Combine(TestUtility.TestDirectory, log.Value)));
                }

                Directory.Delete(extractedDirectory, recursive: true);
            }
            else
            {
                Assert.IsFalse(Directory.GetFiles(uploadOnlyDir, "*", SearchOption.AllDirectories).Any());
            }

            Assert.AreEqual(mode != Modes.Upload, (workflow.collector != null && workflow.collector.IsCollected));
            Assert.AreEqual(mode != Modes.Collect, (workflow.uploader != null && workflow.uploader.IsUploaded));

            File.Delete(traceLogPath);

            TestUtility.TestEpilog();
        }

        internal override LogCollectorBase[] LoadLogCollectors()
        {
            this.collector = new MockUpLogCollector(this.TraceLogger);
            return new LogCollectorBase[]
            {
                this.collector
            };
        }

        internal override LogUploaderBase[] LoadLogUploaders()
        {
            this.uploader = new MockUpLogUploader(this.TraceLogger);
            return new LogUploaderBase[]
            {
                this.uploader
            };
        }

        private MockUpLogUploader uploader;

        private MockUpLogCollector collector;
    }
}