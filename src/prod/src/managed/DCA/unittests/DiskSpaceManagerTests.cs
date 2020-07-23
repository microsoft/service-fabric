// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Linq;
    using System.Threading;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for the <see cref="DiskSpaceManager"/> class.
    /// </summary>
    [TestClass]
    public class DiskSpaceManagerTests
    {
        private const int MB = 1 << 20;
        private const long TestMaxDiskQuota = 100 * MB;
        private const long TestDiskFullSafetySpace = 100 * MB;
        private const double TestDiskQuotaUsageTargetPercent = 80;
        private const int DefaultBytesPerFile = 1 * MB;

        [ClassInitialize]
        public static void InitializeClass(TestContext context)
        {
            Utility.TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.Test);
            Utility.LogDirectory = Environment.CurrentDirectory; // Not used other than it needs to resolve
            HealthClient.Disabled = true;
        }

        /// <summary>
        /// Tests that no cleanup is required for single user, no usage.
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceNoCleanupNeededTest()
        {
            const long DiskSpaceUsed = 0;
            const long ExpectedDiskSpaceToFree = 0;
            ValidateSingleInstance(DiskSpaceUsed, ExpectedDiskSpaceToFree, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Tests that single instance is solely responsible for reaching target at max.
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceOverMaxTest()
        {
            const long DiskSpaceUsed = TestMaxDiskQuota;
            const long ExpectedDiskSpaceToFree = (long)(TestMaxDiskQuota * (100.0 - TestDiskQuotaUsageTargetPercent) / 100.0);
            ValidateSingleInstance(DiskSpaceUsed, ExpectedDiskSpaceToFree, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Tests that single instance should clean up above target threshold.
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceOverTargetTest()
        {
            const long DiskSpaceUsed = (long)(TestMaxDiskQuota * (TestDiskQuotaUsageTargetPercent + 5) / 100.0); ;
            const long TargetUsage = (long)(TestMaxDiskQuota * TestDiskQuotaUsageTargetPercent / 100.0);
            const long ExpectedDiskSpaceToFree = DiskSpaceUsed - TargetUsage;
            ValidateSingleInstance(DiskSpaceUsed, ExpectedDiskSpaceToFree, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Single instance used all remaining disk space on drive.
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceDiskSpaceFullTest()
        {
            ValidateSingleInstance(TestDiskFullSafetySpace, TestDiskFullSafetySpace, 0);
        }

        /// <summary>
        /// Single instance used half of safety space.  Can't be asked to cleanup more than used.
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceHalfDiskSpaceFullTest()
        {
            ValidateSingleInstance(TestDiskFullSafetySpace / 2, TestDiskFullSafetySpace / 2, 0);
        }

        /// <summary>
        /// Single instance used double the safety space. 
        /// </summary>
        [TestMethod]
        public void DiskManagerSingleInstanceDoubleDiskSpaceFullTest()
        {
            // Only need to free enough to reach safety limit.
            ValidateSingleInstance(TestDiskFullSafetySpace * 2, TestDiskFullSafetySpace, 0, TestDiskFullSafetySpace * 10);
        }

        /// <summary>
        /// Test 2 instances producing equal amounts are cleaned up equally.
        /// </summary>
        [TestMethod]
        public void DiskManagerMultiInstanceEqualTest()
        {
            var testInstances = GetMultiInstanceEqualTestInstances();
            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Test 2 instances producing equal amounts are cleaned up equally. All files are marked as unsafe to delete.
        /// </summary>
        [TestMethod]
        public void DiskManagerOverLimitSafetyTest()
        {
            var testInstances = GetMultiInstanceEqualTestInstances().Select(ti =>
            {
                ti.SafeToDeleteCallback = f => false;
                return ti;
            }).ToArray();
            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// One user's files are safe to delete, the other unsafe.  Make sure safe files are deleted first
        /// </summary>
        [TestMethod]
        public void DiskManagerMultiInstanceSafetyFirstTest()
        {
            var testInstances = GetMultiInstanceEqualTestInstances();

            testInstances[0].SafeToDeleteCallback = f => false;
            testInstances[0].ExpectedDiskSpaceToFree = 0; 

            // Redistribute space
            foreach (var ti in testInstances.Skip(1))
            {
                ti.ExpectedDiskSpaceToFree += ti.ExpectedDiskSpaceToFree / (testInstances.Length - 1);
            }

            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Test 2 instances, older instance should have its files cleaned up first.
        /// </summary>
        [TestMethod]
        public void DiskManagerMultiInstanceSingleResponsibleTest()
        {
            const long DiskSpaceUsed = TestMaxDiskQuota / 2;
            const long ExpectedDiskSpaceToFree = (long)(TestMaxDiskQuota * (100.0 - TestDiskQuotaUsageTargetPercent) / 100.0);

            var testInstances = new[]
                                    {
                                        new TestInstance(DiskSpaceUsed, ExpectedDiskSpaceToFree, GetFileCreateCallback(TimeSpan.FromMinutes(1))),
                                        new TestInstance(DiskSpaceUsed, 0, GetFileCreateCallback(TimeSpan.Zero))
                                    };
            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        [TestMethod]
        public void DiskManagerMultiInstanceLocalFileRetentionTest()
        {
            // Make sure total is less than global policy threshold so that only local retention is performed.
            // We cannot guarantee order of global vs local because the folder is registered after the 
            // worker thread is created.
            var testInstances = GetMultiInstanceEqualTestInstances(TestMaxDiskQuota / 2);

            // Change first instance to not needing any files retained
            testInstances[0].FileRetentionPolicy = f => false;
            testInstances[0].ExpectedDiskSpaceToFree = testInstances[0].DiskSpaceUsed;

            // Redistribute saved space
            foreach (var ti in testInstances.Skip(1))
            {
                ti.ExpectedDiskSpaceToFree -= ti.ExpectedDiskSpaceToFree / (testInstances.Length - 1);
            }

            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        /// <summary>
        /// Test that local file retention policies adhere to safety callback.
        /// </summary>
        [TestMethod]
        public void DiskManagerMultiInstanceLocalFileDeletionSafetyTest()
        {
            // Make sure total is less than global policy threshold so that only local retention is performed.
            // We cannot guarantee order of global vs local because the folder is registered after the 
            // worker thread is created.
            var testInstances = GetMultiInstanceEqualTestInstances(TestMaxDiskQuota / 2);

            // Change first instance to needing to retain all files
            testInstances[0].SafeToDeleteCallback = f => false;
            testInstances[0].ExpectedDiskSpaceToFree = 0;

            // Redistribute used space
            foreach (var ti in testInstances.Skip(1))
            {
                ti.ExpectedDiskSpaceToFree += ti.ExpectedDiskSpaceToFree / (testInstances.Length - 1);
            }

            ValidateMultipleInstances(testInstances, TestDiskFullSafetySpace);
        }

        private static void ValidateSingleInstance(
            long diskSpaceUsed,
            long expectedDiskSpaceToFree,
            long diskSpaceRemaining,
            long maxDiskQuota = TestMaxDiskQuota,
            long diskFullSafetySpace = TestDiskFullSafetySpace,
            double diskQuotaUsageTargetPercent = TestDiskQuotaUsageTargetPercent)
        {
            ValidateMultipleInstances(
                new[] { new TestInstance(diskSpaceUsed, expectedDiskSpaceToFree, GetFileCreateCallback(TimeSpan.Zero)) },
                diskSpaceRemaining,
                maxDiskQuota,
                diskFullSafetySpace,
                diskQuotaUsageTargetPercent);
        }

        private static void ValidateMultipleInstances(
            TestInstance[] testInstances,
            long diskSpaceRemaining,
            long maxDiskQuota = TestMaxDiskQuota, 
            long diskFullSafetySpace = TestDiskFullSafetySpace, 
            double diskQuotaUsageTargetPercent = TestDiskQuotaUsageTargetPercent)
        {
            const string User = "test";
            var beginPreparationEvent = new ManualResetEvent(false);
            var endPreparationEvent = new ManualResetEvent(false);

            // Use big enough timespan to prevent spinning before folders are registered.
            using (var diskSpaceManager = new DiskSpaceManager(
                    () => maxDiskQuota, 
                    () => diskFullSafetySpace,
                    () => diskQuotaUsageTargetPercent, 
                    TimeSpan.FromMilliseconds(5)))
            {
                diskSpaceManager.RetentionPassCompleted += () =>
                {
                    beginPreparationEvent.Set();
                    endPreparationEvent.WaitOne();
                };

                beginPreparationEvent.WaitOne();
                var guid = Guid.NewGuid();
                var traceFolder = Directory.CreateDirectory(guid.ToString());

                for (var i = 0; i < testInstances.Length; ++i)
                {
                    Directory.CreateDirectory(Path.Combine(traceFolder.FullName, i.ToString()));
                    Assert.IsTrue(testInstances[i].DiskSpaceUsed % DefaultBytesPerFile == 0, "Must allocate in blocks of 1MB");
                    for (var j = 0; j < testInstances[i].DiskSpaceUsed / DefaultBytesPerFile; ++j)
                    {
                        // Create under folder unique to call and test instance.
                        var file = new FileInfo(Path.Combine(traceFolder.FullName, i.ToString(), j.ToString()));
                        testInstances[i].CreateFileCallback(file);
                    }
                }

                // Make sure files to remove are unique
                var filesToRemove = testInstances.Select(t => new HashSet<FileInfo>()).ToArray();
                Func<FileInfo, bool> testDeleteFunc = f =>
                {
                    var testFolderIndex = int.Parse(Path.GetFileName(Path.GetDirectoryName(f.FullName)));
                    filesToRemove[testFolderIndex].Add(f);
                    f.Delete();
                    return true;
                };
                diskSpaceManager.GetAvailableSpace = d => diskSpaceRemaining + filesToRemove.Sum(u => u.Count) * DefaultBytesPerFile;

                for (var i = 0; i < testInstances.Length; ++i)
                {
                    diskSpaceManager.RegisterFolder(
                        string.Format("{0}{1}", User, i), 
                        new DirectoryInfo(Path.Combine(traceFolder.FullName, i.ToString())),
                        null,
                        testInstances[i].SafeToDeleteCallback,
                        testInstances[i].FileRetentionPolicy,
                        testDeleteFunc);
                }

                var finishedEvent = new ManualResetEvent(false);
                diskSpaceManager.RetentionPassCompleted += () => { finishedEvent.Set(); };
                endPreparationEvent.Set();

                // Need to wait twice to ensure a full pass of both Local and Global policy
                var success = finishedEvent.WaitOne(TimeSpan.FromSeconds(10));
                Assert.IsTrue(success, "Initial pass should finish.");
                finishedEvent.Reset();
                success = finishedEvent.WaitOne(TimeSpan.FromSeconds(10));
                Assert.IsTrue(success, "Second pass should finish");
                for (var i = 0; i < testInstances.Length; ++i)
                {
                    Assert.AreEqual(
                        testInstances[i].ExpectedDiskSpaceToFree, 
                        filesToRemove[i].Count * DefaultBytesPerFile, 
                        "Actual and Expected bytes differ for testInstance {0}", i);
                }
            }
        }

        private static Action<FileInfo> GetFileCreateCallback(TimeSpan age, bool fixedTime = false)
        {
            // Size all files at 1MB so that fileCount == sizeInMb.
            var buffer = new byte[DefaultBytesPerFile];

            var ts = DateTime.UtcNow;
            Action<FileInfo> callback = f =>
            {
                using (var stream = f.Create())
                {
                    stream.Write(buffer, 0, DefaultBytesPerFile);
                }

                f.LastWriteTimeUtc = fixedTime ? ts - age : DateTime.UtcNow - age;
            };

            return callback;
        }

        private static TestInstance[] GetMultiInstanceEqualTestInstances(long totalDiskSpaceUsed = TestMaxDiskQuota)
        {
            var diskSpaceUsed = totalDiskSpaceUsed / 2;
            long expectedDiskSpaceToFree = totalDiskSpaceUsed - (long)(TestMaxDiskQuota * TestDiskQuotaUsageTargetPercent / 100.0);
            expectedDiskSpaceToFree = Math.Max(expectedDiskSpaceToFree, 0);

            Action<FileInfo> splitCallback = f =>
            {
                var index = int.Parse(f.Name);

                // Create file of age equal to index
                GetFileCreateCallback(TimeSpan.FromMinutes(index))(f);
            };
            return new[]
                                    {
                                        new TestInstance(diskSpaceUsed, expectedDiskSpaceToFree / 2, splitCallback),
                                        new TestInstance(diskSpaceUsed, expectedDiskSpaceToFree / 2, splitCallback)
                                    };
        }

        private class TestInstance
        {
            public TestInstance(
                long diskSpaceUsed, 
                long expectedDiskSpaceToFree,
                Action<FileInfo> createFileCallback)
            {
                this.DiskSpaceUsed = diskSpaceUsed;
                this.ExpectedDiskSpaceToFree = expectedDiskSpaceToFree;
                this.CreateFileCallback = createFileCallback;
            }

            public long DiskSpaceUsed { get; set; }

            public long ExpectedDiskSpaceToFree { get; set; }

            public Action<FileInfo> CreateFileCallback { get; set; }

            public Func<FileInfo, bool> SafeToDeleteCallback { get; set; }

            public Func<FileInfo, bool> FileRetentionPolicy { get; set; }
        }
    }
}