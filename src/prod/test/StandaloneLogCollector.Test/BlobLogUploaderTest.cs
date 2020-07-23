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
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.Tracing.Azure;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using Microsoft.WindowsAzure.Storage.Blob;

    [TestClass]
    internal class BlobLogUploaderTest : BlobLogUploader
    {
        private static readonly Dictionary<string, string> dependenciesToMove = new Dictionary<string, string>()
        {
            { "TraceDiagnostic.Store.dll", "tempTraceDiagnostic.Store.dll" },
            { "Microsoft.Tracing.Azure.dll", "tempMicrosoft.Tracing.Azure.dll"},
        };

        [ClassInitialize]
        public static void InitializeClass()
        {
            // this is a work-around before 'RDBug 10820056:move TE to an isolated folder in the build drop' is fixed
            MoveFiles(dependenciesToMove, isKeyTheSrc: true);
        }

        [ClassCleanup]
        public static void CleanUpClass()
        {
            // this is a work-around before 'RDBug 10820056:move TE to an isolated folder in the build drop' is fixed
            MoveFiles(dependenciesToMove, isKeyTheSrc: false);
        }

        private static void MoveFiles(Dictionary<string, string> files, bool isKeyTheSrc)
        {
            string folderPath = Path.GetDirectoryName(TestUtility.TestDirectory);
            foreach (var file in files)
            {
                string src = Path.Combine(folderPath, isKeyTheSrc ? file.Key : file.Value);
                string dst = Path.Combine(folderPath, isKeyTheSrc ? file.Value : file.Key);
                if (File.Exists(src))
                {
                    if (!File.Exists(dst))
                    {
                        File.Move(src, dst);
                    }
                    else
                    {
                        File.Delete(src);
                    }
                }
            }
        }

        public BlobLogUploaderTest(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        public BlobLogUploaderTest()
            : base (null)
        {
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryGetDirectoryTest()
        {
            string result = BlobLogUploader.TryGetDirectory("c:\\my", "c:\\my\\a.txt");
            Assert.IsNull(result);

            result = BlobLogUploader.TryGetDirectory("c:\\my  my\\1", "c:\\my  my\\1\\a.txt");
            Assert.IsNull(result);

            result = BlobLogUploader.TryGetDirectory("c:\\my  my", "c:\\my  my\\1  2\\3\\a.txt");
            Assert.AreEqual("1  2/3", result);

            result = BlobLogUploader.TryGetDirectory("c:\\my  my\\1  2", "c:\\my  my\\1  2\\3\\a.txt");
            Assert.AreEqual("3", result);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetContainerTest()
        {
            BlobLogUploaderTest uploader = new BlobLogUploaderTest();
            CloudBlobContainer container = uploader.GetContainer(TestUtility.GetAzureCredential());
            Assert.AreEqual("container1", container.Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetDestinationBlobTest()
        {
            string rootSrcDirectory = "c:\\my";
            string srcPath = "c:\\my\\1\\2\\3.txt";

            BlobLogUploaderTest uploader = new BlobLogUploaderTest();
            CloudBlobContainer dstContainer = uploader.GetContainer(TestUtility.GetAzureCredential());

            CloudBlockBlob dstBlob = BlobLogUploader.GetDestinationBlob(rootSrcDirectory, srcPath, dstContainer);
            Assert.AreEqual("1/2/3.txt", dstBlob.Name);
            Assert.AreEqual(dstContainer.Name, dstBlob.Container.Name);

            srcPath = "c:\\my\\3.txt";
            dstBlob = BlobLogUploader.GetDestinationBlob(rootSrcDirectory, srcPath, dstContainer);
            Assert.AreEqual("3.txt", dstBlob.Name);
            Assert.AreEqual(dstContainer.Name, dstBlob.Container.Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UploadTest()
        {
            TestUtility.TestProlog();

            string traceLogPath = Utility.GenerateProgramFilePath(Program.Config.WorkingDirectoryPath, ".log");
            using (TraceLogger traceLogger = new TraceLogger(traceLogPath))
            {
                using (LogPackage logPackage = new LogPackage(Program.Config.OutputDirectoryPath, traceLogger))
                {
                    foreach (KeyValuePair<string, string> log in TestUtility.ExistingTestLogs)
                    {
                        Assert.IsTrue(logPackage.AddFile(string.Format("{0}/{1}", log.Key, log.Value), Path.Combine(TestUtility.TestDirectory, log.Value), true));
                    }

                    BlobLogUploaderTest uploader = new BlobLogUploaderTest(traceLogger);
                    uploader.UploadAsync(logPackage, TestUtility.GetAzureCredential()).Wait();
                    Assert.AreEqual(4, uploader.fileUploaded);
                }
            }

            File.Delete(traceLogPath);
            TestUtility.TestEpilog();
        }

        protected override Task UploadFileAsync(string srcPath, CloudBlockBlob dstBlob)
        {
            Task result = Task.Factory.StartNew(() => { });

            lock (this.lockObject)
            {
                this.fileUploaded++;
            }

            return result;
        }

        protected override bool IsContainerEmpty(CloudBlobContainer container)
        {
            return true;
        }

        private int fileUploaded;

        private object lockObject = new object();
    }
}