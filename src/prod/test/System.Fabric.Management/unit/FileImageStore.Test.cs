// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Fabric.ImageStore;
    using System.IO;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    [TestClass]
    class FileImageStoreTest
    {
        class ImageSource
        {
            public const string source = "TestSource";
            public const string storeRoot = "TestRoot";
            public const string sourceDir = "TestDir";
            public const string sourceFile = "Test";
            public const string largeFileTag = "LargeFile";
            public const string contents = "Contents";

            public string sourceString;

            public ImageSource(bool isLargeFile, bool createDirectory, bool createFile, string sourceString)
            {
                this.sourceString = sourceString;
                Directory.CreateDirectory(sourceString);
                if (createFile)
                {
                    string fileName = Path.Combine(sourceString, sourceFile);
                    if (!isLargeFile)
                    {
                        File.AppendAllText(fileName, contents);
                    }
                    else
                    {
                        FileStream fs = File.Open(fileName, FileMode.Create);
                        int size = 1024 * 1024 * 3;
                        byte[] value = new byte[size];
                        for (int i = 0; i < size; i++)
                        {
                            value[i] = 0;
                        }
                        for (int i = 0; i < 200; i++)
                        {
                            fs.Write(value, 0, size);
                        }
                        fs.Close();
                    }
                    SetReadOnlyFlag(fileName, true);
                }
                if (createDirectory)
                {
                    Directory.CreateDirectory(Path.Combine(sourceString, sourceDir));
                    string fileName = Path.Combine(sourceString, sourceDir, sourceFile);
                    File.AppendAllText(fileName, contents);
                    SetReadOnlyFlag(fileName, true);
                }
            }

            private static void SetReadOnlyFlag(string fileName, bool value)
            {
                if (File.Exists(fileName))
                {
                    FileInfo fileInfo = new FileInfo(fileName);
                    fileInfo.IsReadOnly = value;
                }
            }

            public void Delete()
            {
                SetReadOnlyFlag(Path.Combine(sourceString, sourceFile), false);
                SetReadOnlyFlag(Path.Combine(sourceString, sourceDir, sourceFile), false);
                Directory.Delete(sourceString, true);
            }

            ~ImageSource()
            {
            }
        }

        class DelegateParameters
        {
            public AutoResetEvent operationToStartEvent;
            public AutoResetEvent operationFinishedEvent;
            public bool result;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDoesContentExist()
        {
            ImageSource source = new ImageSource(false, false, true, ImageSource.storeRoot);
            FileImageStore store = new FileImageStore("file:.\\TestRoot");
            Verify.IsTrue(store.DoesContentExist(ImageSource.sourceFile, TimeSpan.MaxValue), "Tag Test should Exist");
            Verify.IsFalse(store.DoesContentExist("Test1", TimeSpan.MaxValue), "Tag Test1 should not Exist");
            source.Delete();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDeleteContent()
        {
            ImageSource source = new ImageSource(false, false, true, ImageSource.storeRoot);
            FileImageStore store = new FileImageStore("file:.\\TestRoot");
            Verify.IsTrue(File.Exists(Path.Combine(ImageSource.storeRoot, ImageSource.sourceFile)));
            store.DeleteContent(ImageSource.sourceFile, TimeSpan.MaxValue);
            Verify.IsFalse(File.Exists(Path.Combine(ImageSource.storeRoot, ImageSource.sourceFile)));
            source.Delete();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUploadContent()
        {
            ImageSource root = new ImageSource(false, false, false, ImageSource.storeRoot);
            ImageSource source = new ImageSource(false, true, true, ImageSource.source);
            FileImageStore store = new FileImageStore("file:.\\TestRoot");

            store.UploadContent(ImageSource.sourceFile, Path.Combine(ImageSource.source, ImageSource.sourceFile), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            store.UploadContent(ImageSource.sourceFile, Path.Combine(ImageSource.source, ImageSource.sourceFile), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            Verify.IsTrue(File.Exists(Path.Combine(ImageSource.storeRoot, ImageSource.sourceFile)));
            store.UploadContent(ImageSource.sourceDir, Path.Combine(ImageSource.source, ImageSource.sourceDir), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            Verify.IsTrue(File.Exists(Path.Combine(ImageSource.storeRoot, ImageSource.sourceDir, ImageSource.sourceFile)));
            root.Delete();
            source.Delete();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDownloadContent()
        {
            ImageSource root = new ImageSource(false, true, true, ImageSource.storeRoot);
            ImageSource source = new ImageSource(false, false, false, ImageSource.source);
            FileImageStore store = new FileImageStore("file:.\\TestRoot");

            store.DownloadContent(ImageSource.sourceFile, Path.Combine(ImageSource.source, ImageSource.sourceFile), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent);
            store.DownloadContent(ImageSource.sourceFile, Path.Combine(ImageSource.source, ImageSource.sourceFile), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent);
            Verify.IsTrue(File.Exists(Path.Combine(ImageSource.source, ImageSource.sourceFile)));
            store.DownloadContent(ImageSource.sourceDir, Path.Combine(ImageSource.source, ImageSource.sourceDir), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent);
            Verify.IsTrue(File.Exists(Path.Combine(ImageSource.source, ImageSource.sourceDir, ImageSource.sourceFile)));
            root.Delete();
            source.Delete();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestParallelUploadsAndDownloads()
        {
            // Initialize parameters.
            string destinationLocation = "TestDest";
            ImageSource source = new ImageSource(true, false, true, ImageSource.source);
            ImageSource root = new ImageSource(false, false, false, ImageSource.storeRoot);
            ImageSource destiantion = new ImageSource(false, false, false, destinationLocation);
            AutoResetEvent operationFinishedEvent = new AutoResetEvent(false);
            AutoResetEvent operationToStartEvent = new AutoResetEvent(false);
            DelegateParameters param = new DelegateParameters() { operationFinishedEvent = operationFinishedEvent, operationToStartEvent = operationToStartEvent, result = false };

            // Start the delegate thread.
            Thread thread = new Thread(FileImageStoreTest.UploadDelegate);
            thread.Start(param);
            // Wait for the other thread to start and sleep a little so that the other thread has started.
            operationToStartEvent.WaitOne();
            // Verify you cant download.
            FileImageStore store = new FileImageStore("file:.\\TestRoot");
            string destinationPath = Path.Combine(destinationLocation, ImageSource.sourceFile);
            while (!File.Exists("TestRoot\\Test"))
            {
                Thread.Sleep(100);
            }

            Verify.Throws<FabricTransientException>(
                () => store.DownloadContent(ImageSource.sourceFile, destinationPath, TimeSpan.MaxValue, CopyFlag.CopyIfDifferent), 
                "Download should have failed because of parallel upload.");

            // Once the upload in the other thread is complete this one should complete too.
            operationFinishedEvent.WaitOne();
            store.DownloadContent(ImageSource.sourceFile, destinationPath, TimeSpan.MaxValue, CopyFlag.CopyIfDifferent);
            Verify.IsTrue(File.Exists(destinationPath));
            destiantion.Delete();
            source.Delete();
            root.Delete();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestParallelUploads()
        {
            // Initialize parameters.
            string sourceOneTag = "TestSource1";
            ImageSource source = new ImageSource(true, false, true, ImageSource.source);
            ImageSource root = new ImageSource(false, false, false, ImageSource.storeRoot);
            ImageSource sourceOne = new ImageSource(true, false, true, sourceOneTag);
            AutoResetEvent operationFinishedEvent = new AutoResetEvent(false);
            AutoResetEvent operationToStartEvent = new AutoResetEvent(false);
            DelegateParameters param = new DelegateParameters() { operationFinishedEvent = operationFinishedEvent, operationToStartEvent = operationToStartEvent, result = false };

            // Start the other thread.
            Thread thread = new Thread(FileImageStoreTest.UploadDelegate);
            thread.Start(param);

            // Initialize parameters of this thread.
            FileImageStore store = new FileImageStore("file:.\\TestRoot");
            string sourceOnePath = Path.Combine(sourceOneTag, ImageSource.sourceFile);

            // Wait for the other thread to start.
            operationToStartEvent.WaitOne();
            Thread.Sleep(100);
            // Start upload
            bool result = true;
            try
            {
                store.UploadContent(ImageSource.sourceFile, sourceOnePath, TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            }
            catch (Exception)
            {
                result = false;
            }
            
            // Wait for the other upload to complete.
            operationFinishedEvent.WaitOne();
            // Ensure only one of the uploads passes.
            Verify.IsTrue(result != param.result, String.Format("Result from other thread is {0}, this thread is {1}", result, param.result));
            Verify.IsTrue(File.Exists(sourceOnePath));

            // Ensure that you can upload.
            store.UploadContent(ImageSource.sourceFile, sourceOnePath, TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            Verify.IsTrue(File.Exists(sourceOnePath));
            sourceOne.Delete();
            source.Delete();
            root.Delete();
        }

        public static void UploadDelegate(object delegateParameters)
        {
            DelegateParameters param = (DelegateParameters)delegateParameters;
            FileImageStore store = new FileImageStore("file:.\\TestRoot");
            param.operationToStartEvent.Set();
            Console.WriteLine("Event to upload to start");

            bool success = true;
            try
            {
                store.UploadContent(ImageSource.sourceFile, Path.Combine(ImageSource.source, ImageSource.sourceFile), TimeSpan.MaxValue, CopyFlag.CopyIfDifferent, true);
            }
            catch(Exception)
            {
                success = false;
            }
            param.result = success;
            param.operationFinishedEvent.Set();
        }
    }
}