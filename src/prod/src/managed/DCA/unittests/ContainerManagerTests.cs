// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Tests
{
    using System;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using System.IO;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Fabric.Dca;
    using System.Threading.Tasks;

    [TestClass()]
    public class ContainerManagerTests
    {
        private static Mock<AppInstanceManager> mockAppInstanceManager;
        private static Mock<ContainerEnvironment> mockContainerEnvironment;
        private static string logRootFolder;
        private static Dictionary<string, Object> appInstances = new Dictionary<string, Object>();

        private static readonly string[] emptyFolderMarkerFilesList = { };
        private static readonly string[] processFolderMarkerFilesList = { ContainerEnvironment.ContainerProcessLogMarkerFileName };
        private static readonly string[] removeFolderMarkerFileList = { ContainerEnvironment.ContainerRemoveLogMarkerFileName };
        private static readonly string[] processRemoveFolderMarkerFileList = { ContainerEnvironment.ContainerProcessLogMarkerFileName, ContainerEnvironment.ContainerRemoveLogMarkerFileName };

        [ClassInitialize]
        public static void ClassSetup(TestContext ctx)
        {
            ContainerManagerTests.logRootFolder = Path.Combine(Environment.CurrentDirectory, "DcaTestArtifacts_ContainerManagerTests", "Logs");

            // Creating mock AppInstanceManager
            mockAppInstanceManager = new Mock<AppInstanceManager>();

            mockAppInstanceManager.Setup(apmgr => apmgr.CreateApplicationInstance(It.IsAny<string>(), null))
                .Callback((string appInstance, object obj) =>
                {
                    ContainerManagerTests.appInstances.Add(appInstance, obj);
                });

            mockAppInstanceManager.Setup(apmgr => apmgr.Contains(It.IsAny<string>()))
                .Returns((string appInstance) =>
                {
                    return ContainerManagerTests.appInstances.ContainsKey(appInstance);
                });

            mockAppInstanceManager.Setup(apmgr => apmgr.DeleteApplicationInstance(It.IsAny<string>()))
                .Callback((string appInstance) =>
                {
                    ContainerManagerTests.appInstances.Remove(appInstance);
                });

            // Creating mock ContainerEnvironment
            mockContainerEnvironment = new Mock<ContainerEnvironment>();
        }

        private static void CreateEmptyFolders(string folderNameSuffix, int numberOfEmptyFolders)
        {
            IEnumerable<string> folderNames = ContainerManagerTests.CreateContainerFolderNames(folderNameSuffix, numberOfEmptyFolders);
            ContainerManagerTests.CreateContainerFoldersWithFiles(folderNames, ContainerManagerTests.emptyFolderMarkerFilesList);
        }

        private static void CreateProcessFolders(string folderNameSuffix, int numberOfEmptyFolders)
        {
            IEnumerable<string> folderNames = ContainerManagerTests.CreateContainerFolderNames(folderNameSuffix, numberOfEmptyFolders);
            ContainerManagerTests.CreateContainerFoldersWithFiles(folderNames, ContainerManagerTests.processFolderMarkerFilesList);
        }

        private static void CreateRemoveFolders(string folderNameSuffix, int numberOfEmptyFolders)
        {
            IEnumerable<string> folderNames = ContainerManagerTests.CreateContainerFolderNames(folderNameSuffix, numberOfEmptyFolders);
            ContainerManagerTests.CreateContainerFoldersWithFiles(folderNames, ContainerManagerTests.removeFolderMarkerFileList);
        }

        private static void CreateProcessRemoveFolders(string folderNameSuffix, int numberOfEmptyFolders)
        {
            IEnumerable<string> folderNames = ContainerManagerTests.CreateContainerFolderNames(folderNameSuffix, numberOfEmptyFolders);
            ContainerManagerTests.CreateContainerFoldersWithFiles(folderNames, ContainerManagerTests.processRemoveFolderMarkerFileList);
        }

        private static IEnumerable<string> CreateContainerFolderNames(string folderNameSuffix, int numberOfFolders)
        {
            string[] folderNames = new string[numberOfFolders];
            for (int i = 0; i < numberOfFolders; i++)
            {
                folderNames[i] = GetFolderNameFromSuffixAndNumber(folderNameSuffix, i);
            }

            return folderNames;
        }

        private static string GetFolderNameFromSuffixAndNumber(string folderNameSuffix, int fileNumber)
        {
            return $"{fileNumber:00000}-{folderNameSuffix}";
        }

        private static void CreateContainerFoldersWithFiles(IEnumerable<string> foldersFullPath, IEnumerable<string> fileNamesInsideFolder)
        {
            foreach (string containerName in foldersFullPath)
            {
                string path = Path.Combine(ContainerManagerTests.mockContainerEnvironment.Object.ContainerLogRootDirectory, containerName);
                Directory.CreateDirectory(path);
                ContainerManagerTests.CreateFilesInsideFolder(path, fileNamesInsideFolder);
            }
        }

        private static void CreateFilesInsideFolder(string folderFullPath, IEnumerable<string> fileNames)
        {
            foreach (string fileName in fileNames)
            {
                File.CreateText(Path.Combine(folderFullPath, fileName)).Dispose();
            }
        }

        private static void CreateFilesInsideFoldersWithSuffix(string folderPath, string folderNameSuffix, IEnumerable<string> fileNames)
        {
            List<string> foldersToAddFiles = new List<string>();

            var folders = Directory.EnumerateDirectories(folderPath);
            foreach (string folder in folders)
            {
                string folderName = Path.GetFileName(folder);
                if (folderName.EndsWith(folderNameSuffix, StringComparison.Ordinal))
                {
                    foldersToAddFiles.Add(folder);
                }
            }

            foreach (string folder in foldersToAddFiles)
            {
                ContainerManagerTests.CreateFilesInsideFolder(folder, fileNames);
            }
        }

        private static void SetMockCleanupAndMaxWaitProcessingTimes(TimeSpan cleanupTimeInterval, TimeSpan maxProcessingWaitTime)
        {
            mockContainerEnvironment.Setup(ce => ce.ContainerFolderCleanupTimerInterval).Returns(cleanupTimeInterval);
            mockContainerEnvironment.Setup(ce => ce.MaxContainerLogsProcessingWaitTime).Returns(maxProcessingWaitTime);
        }

        private static void SetMockLogContainerFolder(string folderName)
        {
            mockContainerEnvironment.SetupGet(ce => ce.ContainerLogRootDirectory).Returns(Path.Combine(logRootFolder, folderName, "Containers"));
        }

        private static int GetNumberOfContainerFolders()
        {
            return Directory.EnumerateDirectories(ContainerManagerTests.mockContainerEnvironment.Object.ContainerLogRootDirectory).Count();
        }

        private static void CreateAndKeepRemoveFolderHeld(string folderName, TimeSpan timeToHold)
        {
            ContainerManagerTests.CreateContainerFoldersWithFiles(new string[] { folderName }, emptyFolderMarkerFilesList);

            StreamWriter fileHolder = File.CreateText(Path.Combine(mockContainerEnvironment.Object.ContainerLogRootDirectory, folderName, ContainerEnvironment.ContainerRemoveLogMarkerFileName));
            Thread.Sleep((int)timeToHold.TotalMilliseconds);
            fileHolder.Dispose();
        }

        private static void ContainerManagerStartupAndFileCreationTest(string logFolderNameForTest, TimeSpan cleanupTimeInterval, TimeSpan maxWaitProcessingTime, int numberOfEachFolderType)
        {
            // variables to control the time given to portions of the test to finish its tasks.
            // this are used with cleanupTimeInterval and the numberOfFolders below
            int threadCleanupWaitTimeMultiplier = 5;
            int threadWaitTimeMultiplier = 15;

            int numberOfEmptyFolders = numberOfEachFolderType * 2;
            int numberOfProcessFolders = numberOfEachFolderType;
            int numberOfRemoveFolders = numberOfEachFolderType;
            int numberOfProcessRemoveFolders = numberOfEachFolderType;

            string emptyFolderNameSuffix = "sf-empty-startup";
            string processFolderNameSuffix = "sf-process-startup";
            string removeFolderNameSuffix = "sf-remove-startup";
            string processRemoveFolderNameSuffix = "sf-process-remove-startup";

            // setting up mock ContainerEnvironment
            ContainerManagerTests.SetMockLogContainerFolder(logFolderNameForTest);
            ContainerManagerTests.SetMockCleanupAndMaxWaitProcessingTimes(
                    cleanupTimeInterval,
                    maxWaitProcessingTime);

            // cleanup any folders from previous test
            if (Directory.Exists(ContainerManagerTests.mockContainerEnvironment.Object.ContainerLogRootDirectory))
            {
                Directory.Delete(ContainerManagerTests.mockContainerEnvironment.Object.ContainerLogRootDirectory, true);
            }

            // Creating initial folders before starting testing
            ContainerManagerTests.CreateEmptyFolders(emptyFolderNameSuffix, numberOfEmptyFolders);
            ContainerManagerTests.CreateProcessFolders(processFolderNameSuffix, numberOfProcessFolders);
            ContainerManagerTests.CreateRemoveFolders(removeFolderNameSuffix, numberOfRemoveFolders);

            // Creating the last batch of folders in parallel to test the race condition in startup of ContainerManager
            // with its file watchers and initial listing of folders
            Task createFolders = Task.Run(() =>
            {
                ContainerManagerTests.CreateProcessRemoveFolders(processRemoveFolderNameSuffix, numberOfProcessRemoveFolders);
            });

            ContainerManager containerManager = new ContainerManager(
                ContainerManagerTests.mockAppInstanceManager.Object,
                ContainerManagerTests.mockContainerEnvironment.Object);

            createFolders.Wait();

            // wait for a few cycles of cleanup which should give enough time for finishing removing folders still processing.
            Thread.Sleep(threadCleanupWaitTimeMultiplier * (int)cleanupTimeInterval.TotalMilliseconds);

            // Testing if the number of AppInstances is correct and the number of folders is expected.
            Assert.AreEqual(numberOfProcessFolders, ContainerManagerTests.appInstances.Count);
            Assert.AreEqual(numberOfProcessFolders + numberOfEmptyFolders, ContainerManagerTests.GetNumberOfContainerFolders());

            // start processing empty folders this should increase the number of appInstances
            ContainerManagerTests.CreateFilesInsideFoldersWithSuffix(
                mockContainerEnvironment.Object.ContainerLogRootDirectory,
                emptyFolderNameSuffix,
                processFolderMarkerFilesList);

            // giving some time to process all callbacks for file created
            Thread.Sleep(numberOfEmptyFolders * threadWaitTimeMultiplier);

            Assert.AreEqual(numberOfProcessFolders + numberOfEmptyFolders, ContainerManagerTests.appInstances.Count);
            Assert.AreEqual(numberOfProcessFolders + numberOfEmptyFolders, ContainerManagerTests.GetNumberOfContainerFolders());

            // flag all empty folders (not empty at this point) for deletion
            ContainerManagerTests.CreateFilesInsideFoldersWithSuffix(
                mockContainerEnvironment.Object.ContainerLogRootDirectory,
                emptyFolderNameSuffix,
                removeFolderMarkerFileList);

            // flag all process folders for removal
            ContainerManagerTests.CreateFilesInsideFoldersWithSuffix(
                mockContainerEnvironment.Object.ContainerLogRootDirectory,
                processFolderNameSuffix,
                removeFolderMarkerFileList);

            // giving some time to process all callbacks for file created
            Thread.Sleep((numberOfEmptyFolders + numberOfProcessFolders) * threadWaitTimeMultiplier + threadCleanupWaitTimeMultiplier * (int)cleanupTimeInterval.TotalMilliseconds);

            Assert.AreEqual(0, ContainerManagerTests.appInstances.Count);
            Assert.AreEqual(0, ContainerManagerTests.GetNumberOfContainerFolders());

            // Creating folders to be immediately removed and adding a handle to file to prevent it from being removed initially.
            ContainerManagerTests.CreateRemoveFolders(removeFolderNameSuffix, numberOfRemoveFolders);

            ContainerManagerTests.CreateAndKeepRemoveFolderHeld("temp-held-folder", TimeSpan.FromSeconds(2));

            Thread.Sleep((numberOfRemoveFolders) * threadWaitTimeMultiplier + threadCleanupWaitTimeMultiplier * (int)cleanupTimeInterval.TotalMilliseconds);

            Assert.AreEqual(0, ContainerManagerTests.appInstances.Count);
            Assert.AreEqual(0, ContainerManagerTests.GetNumberOfContainerFolders());

            // Creating folders to be processed and removed
            ContainerManagerTests.CreateProcessRemoveFolders(processRemoveFolderNameSuffix, numberOfProcessRemoveFolders);
            Thread.Sleep((numberOfProcessRemoveFolders) * threadWaitTimeMultiplier + threadCleanupWaitTimeMultiplier * (int)cleanupTimeInterval.TotalMilliseconds);

            Assert.AreEqual(0, ContainerManagerTests.appInstances.Count);
            Assert.AreEqual(0, ContainerManagerTests.GetNumberOfContainerFolders());
        }

        [TestMethod()]
        public void ContainerManagerMaxWaitShorterThanCleanupTimer()
        {
            TimeSpan cleanupTimeInterval = TimeSpan.FromSeconds(1);
            TimeSpan maxWaitProcessingTimeShorter = TimeSpan.FromSeconds(0.5);
            TimeSpan maxWaitProcessingTimeLonger = TimeSpan.FromSeconds(1.5);

            ContainerManagerStartupAndFileCreationTest("shorterMaxWaitTime", cleanupTimeInterval, maxWaitProcessingTimeShorter, 10);
            ContainerManagerStartupAndFileCreationTest("longerMaxWaitTime", cleanupTimeInterval, maxWaitProcessingTimeLonger, 10);
        }
    }
}
