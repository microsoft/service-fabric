// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.IO;
    using System.Fabric.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class FabricDirectoryTest
    {
        private const string TestPathPrefix = @"%_NTTREE%\FabricUnitTests\log\";
        private const string TestFolderName = @"TestDirectoryTest";
        private const string TestRenameFolderName = @"TestRenameDirectoryTest";
        private const string TestFileName = "TestFile.txt";
        private const string TestString = "TestString";

        private const string BadPath = @"BadDrive:\BadPath";

        private string testPath;

        [TestInitialize]
        public void TestInitialize()
        {
            this.testPath = Path.Combine(Environment.ExpandEnvironmentVariables(TestPathPrefix), TestFolderName);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            if (FabricDirectory.Exists(this.testPath))
            {
                FabricDirectory.Delete(this.testPath, true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_CreateNegative()
        {
            try
            {
                LogHelper.Log("FabricDirectory.Create {0}", BadPath);
                FabricDirectory.CreateDirectory(BadPath);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_CopyNegative()
        {
            try
            {
                var src = Environment.ExpandEnvironmentVariables(@"%_NTTREE%\FabricDrop");
                LogHelper.Log("FabricDirectory.Copy from {0} to {1}", src, BadPath);
                FabricDirectory.Copy(src, BadPath, true);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_DeleteNegative()
        {
            try
            {
                LogHelper.Log("FabricDirectory.Delete {0}", BadPath);
                FabricDirectory.Delete(BadPath, true, false);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void FabricDirectory_DeleteNonRecursiveEmptyFolder()
        {
            Directory.CreateDirectory(this.testPath);

            LogHelper.Log("FabricDirectory.Delete {0}", this.testPath);
            FabricDirectory.Delete(this.testPath, false, false);
            Assert.IsFalse(FabricDirectory.Exists(this.testPath));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void FabricDirectory_DeleteNonRecursiveNonEmptyFolder()
        {
            try
            {
                CreateNonEmptyDirectory(this.testPath);

                LogHelper.Log("FabricDirectory.Delete {0}", this.testPath);
                FabricDirectory.Delete(this.testPath, false, false);
                Assert.Fail("An IOException is expected before this.");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void FabricDirectory_DeleteRecursiveNonEmptyFolder()
        {
            CreateNonEmptyDirectory(this.testPath);

            LogHelper.Log("FabricDirectory.Delete {0}", this.testPath);
            FabricDirectory.Delete(this.testPath, true, false);
            Assert.IsFalse(FabricDirectory.Exists(this.testPath));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void FabricDirectory_DeleteRecursiveNonExistentFolder()
        {
            try
            {
                LogHelper.Log("FabricDirectory.Delete {0}", this.testPath);
                FabricDirectory.Delete(this.testPath, true, false);
                Assert.Fail("A DirectoryNotFoundException is expected before this.");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is DirectoryNotFoundException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_ExistsNegative()
        {
            LogHelper.Log("FabricDirectory.Exists {0}", BadPath);
            Assert.IsFalse(FabricDirectory.Exists(BadPath));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_GetDirectoriesNegative()
        {
            LogHelper.Log("FabricDirectory.GetDirectories {0}", BadPath);
            var res = FabricDirectory.GetDirectories(BadPath);
            Assert.AreEqual(0, res.Length);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void FabricDirectory_GetFilesNegative()
        {
            LogHelper.Log("FabricDirectory.GetFiles {0}", BadPath);
            var res = FabricDirectory.GetFiles(BadPath);
            Assert.AreEqual(0, res.Length);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("yihchen")]
        public void FabricDirectory_Rename()
        {
            string renameFolderName = Path.Combine(Environment.ExpandEnvironmentVariables(TestPathPrefix), TestRenameFolderName);
            CreateNonEmptyDirectory(this.testPath);

            LogHelper.Log("FabricDirectory.Rename {0}", this.testPath);
            FabricDirectory.Rename(this.testPath, renameFolderName, true);
            Assert.IsFalse(FabricDirectory.Exists(this.testPath));
            Assert.IsTrue(FabricDirectory.Exists(renameFolderName));
            FabricDirectory.Delete(renameFolderName, true, false);
            Assert.IsFalse(FabricDirectory.Exists(renameFolderName));
        }

        private static void CreateNonEmptyDirectory(string path)
        {
            Directory.CreateDirectory(path);
            var file = File.CreateText(Path.Combine(path, TestFileName));
            file.Write(TestString);
            file.Close(); 
        }
    }
}