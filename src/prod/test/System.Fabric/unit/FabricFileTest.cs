// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;

    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Win32.SafeHandles;

    [TestClass]
    public class FabricFileTest
    {
        private readonly string srcPath = @"%_NTTREE%\FabricDrop\bin\FabricHost.exe";
        private readonly string badPath = @"BadDrive:\BadPath.txt";
        private readonly string testPathPrefix = @"%_NTTREE%\FabricUnitTests\log\";
        private readonly string testFolderName = @"TestFolderName";
        private readonly string testFileName = "TestFile.txt";
        private readonly string testHardLinkedFileName = "TestHardLink.txt";
        private readonly string testReplaceFileName = "TestReplace.txt";
        private readonly string testBackupFileName = "TestBackup.txt";
        private readonly string testString = "TestString";

        private readonly string testNewString = "TestNewString";
        
        private string testPath;

        [TestInitialize]
        public void TestInitialize()
        {
            this.testPath = Path.Combine(Environment.ExpandEnvironmentVariables(this.testPathPrefix), this.testFolderName);
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
        [Owner("mcoskun")]
        public void FabricFile_ShortPath_HardLinkThenReplace_HardLinkShouldPointToOld()
        {
            this.TestNoneIntersectingHardLinkAndReplace(false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_LongPath_HardLinkThenReplace_HardLinkShouldPointToOld()
        {
            this.TestNoneIntersectingHardLinkAndReplace(true);
        }

        /// <summary>
        /// This unit test method confirms that if replace is called on the original file while the hard link file is being read
        /// FabricFile.Replace throws FileLoadException.
        /// Because if this behavior we cannot use hard links in Replicator Stack backup.
        /// If we did, our checkpoints would be blocked by user reading their backups.
        /// Note: We are not using hard links for now, waiting for the behavior to be fixed in Windows. The test detects the 
        /// behavior change, then we can start using hard links.
        /// </summary>
        [TestMethod]
        [Ignore]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_ReplaceWhileHardLinkIsOpen_ReadWrite_ThrowsFileLoadException()
        {
            this.TestIntersectingHardLinkAndReplace(true, FileShare.ReadWrite);
        }

        /// <summary>
        /// This unit test method confirms that if replace is called on the original file while the hard link file is being read
        /// FabricFile.Replace throws FileLoadException.
        /// Because if this behavior we cannot use hard links in Replicator Stack backup.
        /// If we did, our checkpoints would be blocked by user reading their backups.
        /// Note: We are not using hard links for now, waiting for the behavior to be fixed in Windows. The test detects the 
        /// behavior change, then we can start using hard links.
        /// </summary>
        [TestMethod]
        [Ignore]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_ReplaceWhileHardLinkIsOpen_Delete_ThrowsFileLoadException()
        {
            this.TestIntersectingHardLinkAndReplace(true, FileShare.Delete);
        }

        /// <summary>
        /// This unit test method confirms that if replace is called on the original file while the hard link file is being read
        /// FabricFile.Replace throws FileLoadException.
        /// Because if this behavior we cannot use hard links in Replicator Stack backup.
        /// If we did, our checkpoints would be blocked by user reading their backups.
        /// Note: We are not using hard links for now, waiting for the behavior to be fixed in Windows. The test detects the 
        /// behavior change, then we can start using hard links.
        /// </summary>
        [TestMethod]
        [Ignore]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_ReplaceWhileHardLinkIsOpen_None_ThrowsFileLoadException()
        {
            this.TestIntersectingHardLinkAndReplace(true, FileShare.None);
        }
        
        /// <summary>
        /// This unit test mimics the backup scenario in Replicator Stack.
        /// It confirms that ReplaceFile (used in checkpoint) can be called while the copied file (used in backup) is read.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_ReplaceWhileCopyFileIsOpen_Success()
        {
            this.TestIntersectingCopyAndReplace(true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_Copy_CopyOnTopOfExistingFileWithOutOverWrite_ThrowsFabricException()
        {
            this.CopyTest(true, true, false);
        }

        /// <summary>
        /// This unit test mimics the restore scenario in Replicator Stack.
        /// It confirms that RestoreCheckpoint file can overwrite the current checkpoint.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_Copy_TargetExists_Success()
        {
            this.CopyTest(true, true, true);
        }

        /// <summary>
        /// This unit test mimics the restore scenario in Replicator Stack.
        /// It confirms that RestoreCheckpoint file can create the checkpoint file if it does not exist.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_Copy_TargetDoesNotExist_Success()
        {
            this.CopyTest(false, true, true);
        }

        /// <summary>
        /// This unit test mimics the backup scenario in Replicator Stack.
        /// It confirms that ReplaceFile (used in checkpoint) can be called while the copied file (used in backup) is read.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void FabricFile_ReplaceWhileCopyFileIsOpen_ThrowsFileLoadException()
        {
            this.TestIntersectingCopyAndReplace(true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("jesseb")]
        public void FabricFile_GetSize_FileOpenInReadShare_Succeeds()
        {
            string filePath = Path.Combine(this.testPath, this.testFileName);

            LogHelper.Log("FabricDirectory.Create {0}", this.testPath);
            FabricDirectory.CreateDirectory(this.testPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (var writer = new StreamWriter(FabricFile.Create(filePath)))
            {
                writer.WriteLine(this.testString);
            }

            LogHelper.Log("FabricFile.Open {0}", filePath);
            using (var filestream = FabricFile.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                long fileSize = FabricFile.GetSize(filePath);
                LogHelper.Log("FabricFile.GetSize {0}", fileSize);
                Assert.IsTrue(fileSize > 0, "FabricFile.GetSize should return non-zero file size.");
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("subiswal")]
        public void FabricFile_EndToEndPositive()
        {
            var folderPath = this.testPath;
            folderPath = this.ExtendPath(folderPath);

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(filePath.Length > 260);

            LogHelper.Log("FabricDirectory.Create {0}", folderPath);
            FabricDirectory.CreateDirectory(folderPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(filePath)))
            {
                LogHelper.Log("Write {0}", this.testString);
                streamWriter.WriteLine(this.testString);
            }

            LogHelper.Log("FabricDirectory.GetDirectories {0}", this.testPath);
            var result = FabricDirectory.GetDirectories(this.testPath);
            Assert.AreEqual(1, result.Length);

            LogHelper.Log("FabricDirectory.GetFiles {0}", this.testPath);
            result = FabricDirectory.GetFiles(this.testPath);
            Assert.AreEqual(0, result.Length);

            LogHelper.Log("FabricDirectory.GetFiles {0}, AllDirectories", this.testPath);
            result = FabricDirectory.GetFiles(this.testPath, "*", SearchOption.AllDirectories);
            Assert.AreEqual(1, result.Length);

            LogHelper.Log("FabricDirectory.GetDirectories {0}", folderPath);
            result = FabricDirectory.GetDirectories(folderPath);
            Assert.AreEqual(0, result.Length);

            LogHelper.Log("FabricDirectory.GetFiles {0}", folderPath);
            result = FabricDirectory.GetFiles(folderPath);
            Assert.AreEqual(1, result.Length);

            LogHelper.Log("FabricFile.Open {0}", filePath);
            using (StreamReader streamReader = new StreamReader(FabricFile.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.None)))
            {
                string actual = streamReader.ReadLine();
                LogHelper.Log("Read {0}", actual);
                Assert.AreEqual(this.testString, actual);
            }

            LogHelper.Log("FabricFile.GetSize {0}", filePath);
            long size = FabricFile.GetSize(filePath);
            Assert.IsTrue(size > 0);

            LogHelper.Log("FabricPath.GetDirectoryName {0}", filePath);
            string directoryName = FabricPath.GetDirectoryName(filePath);
            Assert.AreEqual(folderPath, directoryName);

            LogHelper.Log("FabricFile.GetLastWriteTime {0}", filePath);
            DateTime oldTime = FabricFile.GetLastWriteTime(filePath);
            Thread.Sleep(TimeSpan.FromSeconds(1));
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Open(filePath, FileMode.Open, FileAccess.Write)))
            {
                LogHelper.Log("Write {0}", this.testString);
                streamWriter.WriteLine(this.testString);
            }

            DateTime newTime = FabricFile.GetLastWriteTime(filePath);
            Assert.IsTrue(newTime > oldTime);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("subiswal")]
        public void FabricFile_CreateNegative()
        {
            try
            {
                LogHelper.Log("FabricFile.Create {0}", this.badPath);
                FabricFile.Create(this.badPath);
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
        [Owner("subiswal")]
        public void FabricFile_OpenNegative()
        {
            try
            {
                LogHelper.Log("FabricFile.Create {0}", this.badPath);
                FabricFile.Open(this.badPath, FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite);
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
        [Owner("subiswal")]
        public void FabricFile_CopyNegative()
        {
            try
            {
                var src = Environment.ExpandEnvironmentVariables(this.srcPath);
                LogHelper.Log("FabricFile.Copy from {0} to {1}", src, this.badPath);
                FabricFile.Copy(src, this.badPath, true);
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
        [Owner("subiswal")]
        public void FabricFile_MoveNegative()
        {
            try
            {
                var src = Environment.ExpandEnvironmentVariables(this.srcPath);
                LogHelper.Log("FabricFile.Move from {0} to {1}", src, this.badPath);
                FabricFile.Move(src, this.badPath);
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
        [Owner("subiswal")]
        public void FabricFile_ExistsNegative()
        {
            LogHelper.Log("FabricFile.Exists {0}", this.badPath);
            Assert.IsFalse(FabricFile.Exists(this.badPath));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("subiswal")]
        public void FabricFile_DeleteNegative()
        {
            try
            {
                LogHelper.Log("FabricFile.Delete {0}", this.badPath);
                FabricFile.Delete(this.badPath);
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
        [Owner("mcoskun")]
        public void SetInformation_ShortPath_IoPriorityHintVeryLow_SetCorrectly()
        {
            this.SetInformationTest(false, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_IoPriorityHintVeryLow_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_ShortPath_IoPriorityHintLow_SetCorrectly()
        {
            this.SetInformationTest(false, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_IoPriorityHintLow_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_ShortPath_IoPriorityHintNormal_SetCorrectly()
        {
            this.SetInformationTest(false, Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_IoPriorityHintNormal_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_ShortPath_DuplicateSet_SetCorrectly()
        {
            this.SetInformationTest(false, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_DuplicateSet_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_NormalThenLow_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mcoskun")]
        public void SetInformation_LongPath_LowThenNormal_SetCorrectly()
        {
            this.SetInformationTest(true, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow, Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal);
        }

        private void SetInformationTest(bool isLongPath, Kernel32Types.PRIORITY_HINT priorityHint)
        {
            this.SetInformationTest(isLongPath, priorityHint, Kernel32Types.PRIORITY_HINT.MaximumIoPriorityHintType);
        }

        private void SetInformationTest(bool isLongPath, Kernel32Types.PRIORITY_HINT priorityHintOne, Kernel32Types.PRIORITY_HINT priorityHintTwo)
        {
            var folderPath = this.testPath;

            if (isLongPath == true)
            {
                folderPath = this.ExtendPath(folderPath);
            }

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(!isLongPath || filePath.Length > 260, "file path must be greater than max path size.");

            FabricDirectory.CreateDirectory(folderPath);

            using (FileStream fileStream = FabricFile.Open(filePath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Read))
            {
                var fileHandle = fileStream.SafeFileHandle;

                // Verify that the default is Normal.
                this.VerifyPriorityHint(fileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal);

                this.SetPriorityHint(fileHandle, priorityHintOne);
                this.VerifyPriorityHint(fileHandle, priorityHintOne);

                if (priorityHintTwo != Kernel32Types.PRIORITY_HINT.MaximumIoPriorityHintType)
                {
                    this.SetPriorityHint(fileHandle, priorityHintTwo);
                    this.VerifyPriorityHint(fileHandle, priorityHintTwo);
                }
            }
        }

        private void SetPriorityHint(SafeFileHandle fileHandle, Kernel32Types.PRIORITY_HINT priorityHint)
        {
            Kernel32Types.FileInformation fileInformation = new Kernel32Types.FileInformation();
            fileInformation.FILE_IO_PRIORITY_HINT_INFO.PriorityHint = priorityHint;

            bool isSet = FabricFile.SetFileInformationByHandle(
                fileHandle,
                Kernel32Types.FILE_INFO_BY_HANDLE_CLASS.FileIoPriorityHintInfo,
                ref fileInformation,
                Marshal.SizeOf(fileInformation.FILE_IO_PRIORITY_HINT_INFO));
            Assert.IsTrue(isSet);
        }

        private void VerifyPriorityHint(SafeFileHandle fileHandle, Kernel32Types.PRIORITY_HINT priorityHint)
        {
            NTTypes.IO_STATUS_BLOCK statusBlock = new NTTypes.IO_STATUS_BLOCK();
            NTTypes.FILE_IO_PRIORITY_HINT_INFORMATION ioPriorityHintInfo = new NTTypes.FILE_IO_PRIORITY_HINT_INFORMATION();
            IntPtr ioPriorityHintInfoPtr = Marshal.AllocHGlobal(Marshal.SizeOf(ioPriorityHintInfo));

            IntPtr queryStatus = FabricFile.NtQueryInformationFile(fileHandle, ref statusBlock, ioPriorityHintInfoPtr, (uint)Marshal.SizeOf(ioPriorityHintInfo), NTTypes.FILE_INFORMATION_CLASS.FileIoPriorityHintInformation);
            bool isQuerySuccessful = (queryStatus == IntPtr.Zero) && (statusBlock.Status == 0);
            Assert.IsTrue(isQuerySuccessful);

            ioPriorityHintInfo = (NTTypes.FILE_IO_PRIORITY_HINT_INFORMATION)Marshal.PtrToStructure(ioPriorityHintInfoPtr, typeof(NTTypes.FILE_IO_PRIORITY_HINT_INFORMATION));
            Assert.AreEqual<int>((int)ioPriorityHintInfo.PriorityHint, (int)priorityHint);
        }

        private void TestNoneIntersectingHardLinkAndReplace(bool isLongPath)
        {
            var folderPath = this.testPath;

            if (true == isLongPath)
            {
                folderPath = this.ExtendPath(folderPath);
            }

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(!isLongPath || filePath.Length > 260, "file path must be greater than max path size.");

            var hardLinkedFilePath = Path.Combine(folderPath, this.testHardLinkedFileName);
            Assert.IsTrue(!isLongPath || hardLinkedFilePath.Length > 260, "hard linked file path must be greater than max path size.");

            var replaceFilePath = Path.Combine(folderPath, this.testReplaceFileName);
            Assert.IsTrue(!isLongPath || replaceFilePath.Length > 260, "replace file path must be greater than max path size.");

            var backupFilePath = Path.Combine(folderPath, this.testBackupFileName);
            Assert.IsTrue(!isLongPath || backupFilePath.Length > 260, "backup file path must be greater than max path size.");

            LogHelper.Log("FabricDirectory.Create {0}", folderPath);
            FabricDirectory.CreateDirectory(folderPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(filePath)))
            {
                LogHelper.Log("Write {0}", this.testString);
                streamWriter.WriteLine(this.testString);
                streamWriter.Flush();
            }

            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(replaceFilePath)))
            {
                LogHelper.Log("Write {0}", this.testNewString);
                streamWriter.WriteLine(this.testNewString);
                streamWriter.Flush();
            }

            FabricFile.CreateHardLink(hardLinkedFilePath, filePath);

            using (StreamReader reader = new StreamReader(FabricFile.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
            {
                var content = reader.ReadLine();
                Assert.AreEqual<string>(this.testString, content, "before replace current file must have the old content.");
            }

            FabricFile.Replace(replaceFilePath, filePath, backupFilePath, false);

            using (StreamReader reader = new StreamReader(FabricFile.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
            {
                var content = reader.ReadLine();
                Assert.AreEqual<string>(this.testNewString, content, "after replace current file must have the new content.");
            }

            using (StreamReader reader = new StreamReader(FabricFile.Open(hardLinkedFilePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
            {
                var content = reader.ReadLine();
                Assert.AreEqual<string>(this.testString, content, "after replace hard link file must have the old content.");
            }
        }

        private void CopyTest(bool copyFileExists, bool isLongPath, bool overWrite)
        {
            var folderPath = this.testPath;

            if (true == isLongPath)
            {
                folderPath = this.ExtendPath(folderPath);
            }

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(!isLongPath || filePath.Length > 260, "file path must be greater than max path size.");

            var copyFilePath = Path.Combine(folderPath, this.testReplaceFileName);
            Assert.IsTrue(!isLongPath || copyFilePath.Length > 260, "replace file path must be greater than max path size.");

            LogHelper.Log("FabricDirectory.Create {0}", folderPath);
            FabricDirectory.CreateDirectory(folderPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(filePath)))
            {
                LogHelper.Log("Write {0}", this.testNewString);
                streamWriter.WriteLine(this.testNewString);
                streamWriter.Flush();
            }

            if (copyFileExists)
            {
                using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(copyFilePath)))
                {
                    LogHelper.Log("Write {0}", this.testString);
                    streamWriter.WriteLine(this.testString);
                    streamWriter.Flush();
                }

                using (StreamReader copyReader = new StreamReader(FabricFile.Open(copyFilePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
                {
                    var content = copyReader.ReadLine();
                    Assert.AreEqual<string>(this.testString, content, "before copy state.");
                }
            }
            else
            {
                Assert.IsFalse(FabricFile.Exists(copyFilePath));
            }

            try
            {
                FabricFile.Copy(filePath, copyFilePath, overWrite);
                Assert.IsTrue(overWrite);
            }
            catch (FabricException e)
            {
                Assert.IsFalse(overWrite);
                LogHelper.Log("Exception thrown as expected {0}", e.GetType().ToString());
            }

            if (true == overWrite || false == copyFileExists)
            {
                using (StreamReader copyReader = new StreamReader(FabricFile.Open(copyFilePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
                {
                    var content = copyReader.ReadLine();
                    Assert.AreEqual<string>(this.testNewString, content, "after copy file must have the new content.");
                    LogHelper.Log("Read {0} as expected", this.testNewString);
                }
            }
        }

        private void TestIntersectingHardLinkAndReplace(bool isLongPath, FileShare fileShare)
        {
            var folderPath = this.testPath;

            if (true == isLongPath)
            {
                folderPath = this.ExtendPath(folderPath);
            }

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(!isLongPath || filePath.Length > 260, "file path must be greater than max path size.");

            var hardLinkedFilePath = Path.Combine(folderPath, this.testHardLinkedFileName);
            Assert.IsTrue(!isLongPath || hardLinkedFilePath.Length > 260, "hard linked file path must be greater than max path size.");

            var replaceFilePath = Path.Combine(folderPath, this.testReplaceFileName);
            Assert.IsTrue(!isLongPath || replaceFilePath.Length > 260, "replace file path must be greater than max path size.");

            var backupFilePath = Path.Combine(folderPath, this.testBackupFileName);
            Assert.IsTrue(!isLongPath || backupFilePath.Length > 260, "backup file path must be greater than max path size.");

            LogHelper.Log("FabricDirectory.Create {0}", folderPath);
            FabricDirectory.CreateDirectory(folderPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(filePath)))
            {
                LogHelper.Log("Write {0}", this.testString);
                streamWriter.WriteLine(this.testString);
                streamWriter.Flush();
            }

            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(replaceFilePath)))
            {
                LogHelper.Log("Write {0}", this.testNewString);
                streamWriter.WriteLine(this.testNewString);
                streamWriter.Flush();
            }

            FabricFile.CreateHardLink(hardLinkedFilePath, filePath);

            using (StreamReader hardLinkReader0 = new StreamReader(FabricFile.Open(hardLinkedFilePath, FileMode.Open, FileAccess.Read, fileShare)))
            {
                var hardLinkContent0 = hardLinkReader0.ReadLine();
                Assert.AreEqual<string>(this.testString, hardLinkContent0, "after replace hard link file must have the old content.");

                try
                {
                    FabricFile.Replace(replaceFilePath, filePath, backupFilePath, false);
                    Assert.Fail();
                }
                catch (FileLoadException)
                {
                }
            }
        }

        private void TestIntersectingCopyAndReplace(bool isLongPath)
        {
            var folderPath = this.testPath;

            if (true == isLongPath)
            {
                folderPath = this.ExtendPath(folderPath);
            }

            var filePath = Path.Combine(folderPath, this.testFileName);
            Assert.IsTrue(!isLongPath || filePath.Length > 260, "file path must be greater than max path size.");

            var copyFilePath = Path.Combine(folderPath, this.testHardLinkedFileName);
            Assert.IsTrue(!isLongPath || copyFilePath.Length > 260, "hard linked file path must be greater than max path size.");

            var replaceFilePath = Path.Combine(folderPath, this.testReplaceFileName);
            Assert.IsTrue(!isLongPath || replaceFilePath.Length > 260, "replace file path must be greater than max path size.");

            var backupFilePath = Path.Combine(folderPath, this.testBackupFileName);
            Assert.IsTrue(!isLongPath || backupFilePath.Length > 260, "backup file path must be greater than max path size.");

            LogHelper.Log("FabricDirectory.Create {0}", folderPath);
            FabricDirectory.CreateDirectory(folderPath);

            LogHelper.Log("FabricFile.Create {0}", filePath);
            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(filePath)))
            {
                LogHelper.Log("Write {0}", this.testString);
                streamWriter.WriteLine(this.testString);
                streamWriter.Flush();
            }

            using (StreamWriter streamWriter = new StreamWriter(FabricFile.Create(replaceFilePath)))
            {
                LogHelper.Log("Write {0}", this.testNewString);
                streamWriter.WriteLine(this.testNewString);
                streamWriter.Flush();
            }

            FabricFile.Copy(filePath, copyFilePath, false);

            using (StreamReader reader = new StreamReader(FabricFile.Open(filePath, FileMode.Open, FileAccess.Read)))
            {
                var content = reader.ReadLine();
                Assert.AreEqual<string>(this.testString, content, "before replace current file must have the old content.");
            }

            using (StreamReader copyReader0 = new StreamReader(FabricFile.Open(copyFilePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
            {
                var hardLinkContent0 = copyReader0.ReadLine();
                Assert.AreEqual<string>(this.testString, hardLinkContent0, "after replace hard link file must have the old content.");

                FabricFile.Replace(replaceFilePath, filePath, backupFilePath, false);

                using (StreamReader fileReader = new StreamReader(FabricFile.Open(filePath, FileMode.Open, FileAccess.Read)))
                {
                    var content = fileReader.ReadLine();
                    Assert.AreEqual<string>(this.testNewString, content, "after replace current file must have the new content.");
                }

                using (StreamReader copyReader1 = new StreamReader(FabricFile.Open(copyFilePath, FileMode.Open, FileAccess.Read, FileShare.Read)))
                {
                    var content = copyReader1.ReadLine();
                    Assert.AreEqual<string>(this.testString, content, "after replace hard link file must have the old content.");
                }
            }
        }

        private string ExtendPath(string original)
        {
            string output = original;

            for (int i = 0; i < 19; i++)
            {
                output = Path.Combine(output, this.testFolderName);
            }

            Assert.IsTrue(output.Length > 260, "folder path must be greater than max path size.");

            return output;
        }
    }
}