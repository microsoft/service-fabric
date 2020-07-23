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
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    using FileMetadataInfo = Microsoft.ServiceFabric.StandaloneLogCollector.FileMetadataFilter.FileMetadataInfo;

    [TestClass]
    public class FileMetadataFilterTest
    {
        private static Tuple<string, DateTime, DateTime>[] testFiles = new Tuple<string, DateTime, DateTime>[]
            {
                new Tuple<string, DateTime, DateTime>(TestUtility.TestDirectory + "\\" + "1.txt", new DateTime(2000, 1, 1), new DateTime(2000, 1, 2)),
                new Tuple<string, DateTime, DateTime>(TestUtility.TestDirectory + "\\" + "2.txt", new DateTime(3000, 1, 1), new DateTime(3000, 1, 2)),
                new Tuple<string, DateTime, DateTime>(TestUtility.TestDirectory + "\\" + "3.txt", new DateTime(4000, 1, 1), new DateTime(4000, 1, 2)),
                new Tuple<string, DateTime, DateTime>(TestUtility.TestDirectory + "\\" + "4.txt", new DateTime(5000, 1, 1), new DateTime(5000, 1, 2)),
                new Tuple<string, DateTime, DateTime>(TestUtility.TestDirectory + "\\" + "5.txt", new DateTime(6000, 1, 1), new DateTime(6000, 1, 2)),
            };

        [TestInitialize]
        public void TestInitialize()
        {
            foreach (Tuple<string, DateTime, DateTime> testFile in FileMetadataFilterTest.testFiles)
            {
                File.Create(testFile.Item1).Dispose();
                File.SetCreationTime(testFile.Item1, testFile.Item2);
                File.SetLastWriteTime(testFile.Item1, testFile.Item3);
            }
        }

        [TestCleanup]
        public void Cleanup()
        {
            foreach (string filePath in FileMetadataFilterTest.testFiles.Select(p => p.Item1))
            {
                File.Delete(filePath);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FileMetadataInfoConstructorTest()
        {
            DateTime expectedCreationTime = new DateTime(2, 2, 2, 2, 2, 2, DateTimeKind.Utc);
            DateTime expectedLastWriteTime = new DateTime(3, 3, 3, 3, 3, 3, DateTimeKind.Utc);
            FileMetadataInfo info = new FileMetadataInfo(expectedCreationTime, expectedLastWriteTime);
            Assert.AreEqual(expectedCreationTime, info.CreationTimeUtc);
            Assert.AreEqual(expectedLastWriteTime, info.LastWriteTimeUtc);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetFileMetadataCutsTest()
        {
            FileMetadataInfo[] infos = new FileMetadataInfo[]
            {
                new FileMetadataInfo(new DateTime(1000, 1, 1), new DateTime(1000, 1, 2)),
                new FileMetadataInfo(new DateTime(2000, 1, 1), new DateTime(2000, 1, 2)),
                new FileMetadataInfo(new DateTime(3000, 1, 1), new DateTime(3000, 1, 2)),
                new FileMetadataInfo(new DateTime(4000, 1, 1), new DateTime(4000, 1, 2)),
                new FileMetadataInfo(new DateTime(5000, 1, 1), new DateTime(5000, 1, 2)),
            };

            DateTime startTime;
            DateTime endTime;
            DateTime startCut;
            DateTime endCut;

            startTime = DateTime.MinValue.ToUniversalTime();
            endTime = startTime + TimeSpan.FromDays(1);
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), startCut);
            Assert.AreEqual(new DateTime(1000, 1, 2), endCut);

            startTime = DateTime.MaxValue.ToUniversalTime() - TimeSpan.FromDays(1);
            endTime = DateTime.MaxValue.ToUniversalTime();
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(5000, 1, 1), startCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), endCut);

            startTime = new DateTime(2000, 1, 1);
            endTime = new DateTime(3000, 1, 2);
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(1000, 1, 1), startCut);
            Assert.AreEqual(new DateTime(4000, 1, 2), endCut);

            startTime = new DateTime(2000, 1, 3);
            endTime = new DateTime(4000, 1, 3);
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(2000, 1, 1), startCut);
            Assert.AreEqual(new DateTime(5000, 1, 2), endCut);

            startTime = new DateTime(100, 1, 1);
            endTime = new DateTime(9000, 1, 1);
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), startCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), endCut);

            startTime = DateTime.MinValue.ToUniversalTime();
            endTime = new DateTime(4000, 1, 3);
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), startCut);
            Assert.AreEqual(new DateTime(5000, 1, 2), endCut);

            startTime = new DateTime(4000, 1, 1);
            endTime = DateTime.MaxValue.ToUniversalTime();
            FileMetadataFilter.GetFileMetadataCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(3000, 1, 1), startCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), endCut);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateFilterFuncTest()
        {
            List<FileInfo> fileInfos = new List<FileInfo>(FileMetadataFilterTest.testFiles.Select(p => new FileInfo(p.Item1)));

            FileMetadataFilter filter = new FileMetadataFilter();
            Func<FileInfo, bool> filterFunc = filter.CreateFilterFunc(fileInfos, new DateTime(4000, 1, 1), new DateTime(4000, 2, 2));
            IEnumerable<FileInfo> result = fileInfos.Where(filterFunc);
            Assert.AreEqual(3, result.Count());
            Assert.IsTrue(result.Contains(fileInfos[1]) && result.Contains(fileInfos[2]) && result.Contains(fileInfos[3]));
        }
    }
}