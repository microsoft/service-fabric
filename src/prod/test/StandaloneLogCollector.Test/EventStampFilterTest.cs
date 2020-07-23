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

    using EventStampInfo = Microsoft.ServiceFabric.StandaloneLogCollector.EventStampFilter.EventStampInfo;

    [TestClass]
    public class EventStampFilterTest
    {
        private static KeyValuePair<string, DateTime>[] testFiles = new KeyValuePair<string, DateTime>[]
            {
                new KeyValuePair<string, DateTime>(string.Format("{0}\\a.b.c_{1}_biabia.dtr.zip", TestUtility.TestDirectory, new DateTime(2000, 1, 1).Ticks), new DateTime(2000, 1, 1)),
                new KeyValuePair<string, DateTime>(string.Format("{0}\\a.b.c_{1}_biabia.dtr.zip", TestUtility.TestDirectory, new DateTime(3000, 1, 1).Ticks), new DateTime(3000, 1, 1)),
                new KeyValuePair<string, DateTime>(string.Format("{0}\\a.b.c_{1}_biabia.dtr.zip", TestUtility.TestDirectory, new DateTime(4000, 1, 1).Ticks), new DateTime(4000, 1, 1)),
                new KeyValuePair<string, DateTime>(string.Format("{0}\\a.b.c_{1}_biabia.dtr.zip", TestUtility.TestDirectory, new DateTime(5000, 1, 1).Ticks), new DateTime(5000, 1, 1)),
                new KeyValuePair<string, DateTime>(string.Format("{0}\\a.b.c_{1}_biabia.dtr.zip", TestUtility.TestDirectory, new DateTime(6000, 1, 1).Ticks), new DateTime(6000, 1, 1)),
            };

        [TestInitialize]
        public void TestInitialize()
        {
            foreach (KeyValuePair<string, DateTime> testFile in EventStampFilterTest.testFiles)
            {
                File.Create(testFile.Key).Dispose();
            }
        }

        [TestCleanup]
        public void Cleanup()
        {
            foreach (string filePath in EventStampFilterTest.testFiles.Select(p => p.Key))
            {
                File.Delete(filePath);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetEventStampCutsTest()
        {
            EventStampInfo[] infos = new EventStampInfo[]
            {
                new EventStampInfo(DateTime.MinValue, new DateTime(2000, 1, 1)),
                new EventStampInfo(new DateTime(2000, 1, 1), new DateTime(3000, 1, 1)),
                new EventStampInfo(new DateTime(3000, 1, 1), new DateTime(4000, 1, 1)),
                new EventStampInfo(new DateTime(4000, 1, 1), new DateTime(5000, 1, 1)),
                new EventStampInfo(new DateTime(5000, 1, 1), new DateTime(6000, 1, 1)),
            };

            DateTime startTime;
            DateTime endTime;
            DateTime startCut;
            DateTime endCut;

            startTime = new DateTime(1000, 1, 1);
            endTime = startTime + TimeSpan.FromDays(1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(2000, 1, 1), startCut);
            Assert.AreEqual(new DateTime(2000, 1, 1), endCut);

            startTime = new DateTime(7000, 1, 1);
            endTime = startTime + TimeSpan.FromDays(1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), startCut);
            Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), endCut);

            startTime = new DateTime(3000, 1, 1);
            endTime = new DateTime(4000, 1, 1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(startTime, startCut);
            Assert.AreEqual(new DateTime(5000, 1, 1), endCut);

            startTime = new DateTime(1000, 1, 1);
            endTime = new DateTime(9000, 1, 1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(startTime, startCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), endCut);

            startTime = new DateTime(1000, 1, 1);
            endTime = new DateTime(3000, 1, 1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(new DateTime(2000, 1, 1), startCut);
            Assert.AreEqual(new DateTime(4000, 1, 1), endCut);

            startTime = new DateTime(3000, 1, 1);
            endTime = new DateTime(9000, 1, 1);
            EventStampFilter.GetEventStampCuts(infos, startTime, endTime, out startCut, out endCut);
            Assert.AreEqual(startTime, startCut);
            Assert.AreEqual(DateTime.MaxValue.ToUniversalTime(), endCut);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateFilterFuncTest()
        {
            List<FileInfo> fileInfos = new List<FileInfo>(EventStampFilterTest.testFiles.Select(p => new FileInfo(p.Key)));

            EventStampFilter filter = new EventStampFilter();
            Func<FileInfo, bool> filterFunc = filter.CreateFilterFunc(fileInfos, new DateTime(3000, 1, 1), new DateTime(4000, 1, 1));
            IEnumerable<FileInfo> result = fileInfos.Where(filterFunc);
            Assert.AreEqual(3, result.Count());
            Assert.IsTrue(result.Contains(fileInfos[1]) && result.Contains(fileInfos[2]) && result.Contains(fileInfos[3]));
        }
    }
}