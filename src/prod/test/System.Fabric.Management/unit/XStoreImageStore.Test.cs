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
    using System.Collections.Generic;
    using System.Linq;

    [TestClass]
    public class XStoreImageStore
    {
        private TestContext testContextInstance;
        private ImageBuilderTestContext imageBuilderTestContext;
        private IImageStore imageStore;

        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestInitialize]
        public void TestSetup()
        {
            var testName = this.TestContext.TestName;
#if DotNetCoreClr
            testName = this.TestContext.FullyQualifiedTestClassName + "." + this.TestContext.TestName;
#endif
            this.imageBuilderTestContext = ImageBuilderTestContext.Create(testName);
            this.imageStore = imageBuilderTestContext.ImageStore;
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFolderUploadAndDownload()
        {
            string localSourcePath = GenerateRandomFolder(5, 2, 100 * 1024 /*100 MB*/);
            string localDownloadPath = Path.Combine(TestUtility.TestDirectory, Path.GetRandomFileName());
            try
            {
                string storeFolderName = "ImageStoreTest";
                this.imageStore.UploadContent(storeFolderName, localSourcePath, TimeSpan.MaxValue, CopyFlag.AtomicCopy, true);

                Assert.IsTrue(this.imageStore.DoesContentExist(storeFolderName, TimeSpan.MaxValue), "Uploaded folder exists");

                this.imageStore.DownloadContent(storeFolderName, localDownloadPath, TimeSpan.MaxValue, CopyFlag.AtomicCopy);

                ValidateFolderContent(localSourcePath, localDownloadPath);
            }
            finally
            {
                if(Directory.Exists(localSourcePath))
                {
                    Directory.Delete(localSourcePath, true);
                }

                if (Directory.Exists(localDownloadPath))
                {
                    Directory.Delete(localDownloadPath, true);
                }
            }
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFolderCopyAndDownload()
        {
            string localSourcePath = GenerateRandomFolder(4, 2, 25 * 1024 /*25 MB*/);
            string localDownloadPath = Path.Combine(TestUtility.TestDirectory, Path.GetRandomFileName());
            try
            {
                string storeUploadedFolderName = "ImageStoreTest\\UploadedFolder";
                string storeCopiedFolderName = "ImageStoreTest\\CopiedFolder";

                this.imageStore.UploadContent(storeUploadedFolderName, localSourcePath, TimeSpan.MaxValue, CopyFlag.AtomicCopy, true);

                Assert.IsTrue(this.imageStore.DoesContentExist(storeUploadedFolderName, TimeSpan.MaxValue), "Uploaded folder exists");

                this.imageStore.CopyContent(storeUploadedFolderName, storeCopiedFolderName, TimeSpan.MaxValue, new string[] { }, CopyFlag.AtomicCopy, true);

                Assert.IsTrue(this.imageStore.DoesContentExist(storeCopiedFolderName, TimeSpan.MaxValue), "Copied folder exists");

                this.imageStore.DownloadContent(storeCopiedFolderName, localDownloadPath, TimeSpan.MaxValue, CopyFlag.AtomicCopy);

                ValidateFolderContent(localSourcePath, localDownloadPath);
            }
            finally
            {
                if (Directory.Exists(localSourcePath))
                {
                    Directory.Delete(localSourcePath, true);
                }

                if (Directory.Exists(localDownloadPath))
                {
                    Directory.Delete(localDownloadPath, true);
                }
            }
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFileFilterDuringCopy()
        {
            string localSourcePath = GenerateRandomFolder(8, 2, 100 /*100 KB*/);
            string localDownloadPath = Path.Combine(TestUtility.TestDirectory, Path.GetRandomFileName());
            try
            {
                string storeUploadedFolderName = "ImageStoreTest\\UploadedFolder";
                string storeCopiedFolderName = "ImageStoreTest\\CopiedFolder";

                // Choose a random file from the folder to skip during copy
                 var randFileName = new DirectoryInfo(localSourcePath).GetFiles("*", SearchOption.AllDirectories).First().Name;

                 this.imageStore.UploadContent(storeUploadedFolderName, localSourcePath, TimeSpan.MaxValue, CopyFlag.AtomicCopy, true);

                 this.imageStore.CopyContent(storeUploadedFolderName, storeCopiedFolderName, TimeSpan.MaxValue, new string[] { randFileName }, CopyFlag.AtomicCopy, true);

                 this.imageStore.DownloadContent(storeCopiedFolderName, localDownloadPath, TimeSpan.MaxValue, CopyFlag.AtomicCopy);

                bool filteredFileExists = new DirectoryInfo(localDownloadPath).GetFiles("*", SearchOption.AllDirectories).Any(fileInfo => fileInfo.Name == randFileName);

                Assert.IsFalse(filteredFileExists, string.Format("FilteredFile {0} exists", randFileName));
            }
            finally
            {
                if (Directory.Exists(localSourcePath))
                {
                    Directory.Delete(localSourcePath, true);
                }

                if (Directory.Exists(localDownloadPath))
                {
                    Directory.Delete(localDownloadPath, true);
                }
            }
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFileFileNotFoundException()
        {
            string localSourcePath = GenerateRandomFolder(1, 1, 10 /*10 KB*/);
            try
            {
                string storeUploadedFolderName = "ImageStoreTest\\UploadedFolder";
                this.imageStore.UploadContent(storeUploadedFolderName, localSourcePath, TimeSpan.MaxValue, CopyFlag.AtomicCopy, true);

                // Other components have a dependency on FileNotFound exception
                AssertThrows<FileNotFoundException>(
                    () => this.imageStore.DownloadContent("RandomFile.txt", TestUtility.TestDirectory, TimeSpan.MaxValue, CopyFlag.AtomicCopy));
            }
            finally
            {
                if (Directory.Exists(localSourcePath))
                {
                    Directory.Delete(localSourcePath, true);
                }
            }
        }

        private static void ValidateFolderContent(string path1, string path2)
        {
            DirectoryInfo dirInfo1 = new DirectoryInfo(path1);
            DirectoryInfo dirInfo2 = new DirectoryInfo(path2);

            Assert.IsTrue(dirInfo1.Exists, string.Format("{0} exists", path1));
            Assert.IsTrue(dirInfo2.Exists, string.Format("{0} exists", path2));

            var dirFiles1 = dirInfo1.GetFiles("*", SearchOption.AllDirectories).OrderBy(key => key.Name);
            var dirFiles2 = dirInfo2.GetFiles("*", SearchOption.AllDirectories).OrderBy(key => key.Name);

            Assert.IsTrue(dirFiles1.Count() == dirFiles2.Count(), "Directory file count");

            for(int i = 0 ; i < dirFiles1.Count(); i++)
            {
                ValidateFileContent(dirFiles1.ElementAt(i).FullName, dirFiles2.ElementAt(i).FullName);
            }
        }

        private static void ValidateFileContent(string path1, string path2)
        {
            FileInfo fileInfo1 = new FileInfo(path1);
            FileInfo fileInfo2 = new FileInfo(path2);

            Assert.IsTrue(fileInfo1.Exists, string.Format("{0} exists", path1));
            Assert.IsTrue(fileInfo2.Exists, string.Format("{0} exists", path2));

            Assert.IsTrue(fileInfo1.Length == fileInfo2.Length, "FileContent lenght");
        }

        private static string GenerateRandomFolder(int numberOfFile, int level, int maxSizeInKB)
        {
            string[] levelPath = new string[level + 1];
            try
            {
                for (int i = 0; i < levelPath.Length; i++)
                {
                    if (i == 0)
                    {
                        levelPath[i] = Path.Combine(TestUtility.TestDirectory, Path.GetRandomFileName());
                    }
                    else
                    {
                        levelPath[i] = Path.Combine(levelPath[i - 1], Path.GetRandomFileName());
                    }

                    Directory.CreateDirectory(levelPath[i]);
                }

                // Generate atleast one file in maxSize in deepest level
                GenerateRandomFile(Path.Combine(levelPath[level], Path.GetRandomFileName()), maxSizeInKB);
                numberOfFile--;

                int fileSize = Math.Min(maxSizeInKB, 4 * 1024 /*Limit random files to 4 MB*/);
                var random = new Random(level);
                while (numberOfFile > 0)
                {
                    int randomLevel = random.Next(level);
                    GenerateRandomFile(Path.Combine(levelPath[randomLevel], Path.GetRandomFileName()), fileSize);
                    fileSize = Math.Max(fileSize / 2, 1 /*File should be atleast 1KB*/);
                    numberOfFile--;
                }
            }
            catch(Exception)
            {
                if(!string.IsNullOrEmpty(levelPath[0]) && Directory.Exists(levelPath[0]))
                {
                    Directory.Delete(levelPath[0], true);
                }

                throw;
            }

            return levelPath[0];
        }

        private static void GenerateRandomFile(string fileName, int sizeInKB)
        {
            using (var stream = new FileStream(fileName, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                stream.SetLength(sizeInKB * 1024);
            }
        }

        private void AssertThrows<T>(Action task)
        {
            try
            {
                task();
            }
            catch (Exception ex)
            {
                Assert.IsInstanceOfType(ex, typeof(T), string.Format("Expected exception of type {0} but exception {1} was thrown.", typeof(T), ex));
                return;
            }
            Assert.Fail(string.Format("Expected exception of type {0} but no exception was thrown.", typeof(T)));
        }
    }
}