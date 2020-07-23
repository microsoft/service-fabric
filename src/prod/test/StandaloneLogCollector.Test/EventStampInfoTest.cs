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
    public class EventStampInfoTest
    {
        private KeyValuePair<DateTime, string>[] testFiles = new KeyValuePair<DateTime, string>[]
            {
                new KeyValuePair<DateTime, string>(new DateTime(2000, 1, 2), null),
                new KeyValuePair<DateTime, string>(new DateTime(3000, 1, 2), null),
                new KeyValuePair<DateTime, string>(new DateTime(4000, 1, 2), null),
                new KeyValuePair<DateTime, string>(new DateTime(5000, 1, 2), null),
                new KeyValuePair<DateTime, string>(new DateTime(6000, 1, 2), null),
            };

        [TestInitialize]
        public void TestInitialize()
        {
            for (int i = 0; i < this.testFiles.Length; i++)
            {
                string filePath = string.Format("{0}\\2_2_3_6.7.0.1_4_5_{1}_0000000000.dtr.zip", TestUtility.TestDirectory, this.testFiles[i].Key.Ticks);
                File.Create(filePath).Dispose();
                this.testFiles[i] = new KeyValuePair<DateTime, string>(this.testFiles[i].Key, filePath);
            }
        }

        [TestCleanup]
        public void Cleanup()
        {
            foreach (string filePath in this.testFiles.Select(p => p.Value))
            {
                File.Delete(filePath);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            DateTime expectedFirstStamp = new DateTime(2, 2, 2, 2, 2, 2, DateTimeKind.Utc);
            DateTime expectedLastStamp = new DateTime(3, 3, 3, 3, 3, 3, DateTimeKind.Utc);
            EventStampInfo info = new EventStampInfo(expectedFirstStamp, expectedLastStamp);
            Assert.AreEqual(expectedFirstStamp, info.FirstEventStamp);
            Assert.AreEqual(expectedLastStamp, info.LastEventStamp);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetLastEventStampTest()
        {
            DateTime expectedValue = DateTime.UtcNow;
            long ticks = expectedValue.Ticks;
            string filePath = string.Format("{0}\\1_2_3_6.0.0.1_4_5_{1}_0000000000.dtr.zip", TestUtility.TestDirectory, ticks);
            File.Create(filePath).Dispose();

            DateTime result = EventStampInfo.GetLastEventStamp(new FileInfo(filePath));
            Assert.AreEqual(expectedValue, result);
            File.Delete(filePath);

            string[] invalidfilePaths = new string[]
            {
                string.Format("{0}\\1_2_3_6.0.0.1_4_5_hiahia_0000000000.dtr.zip", TestUtility.TestDirectory),
                string.Format("{0}\\1_2_3_6.0.0.1_4_5_99999999999999999999999999999999999999999999999999_0000000000.dtr.zip", TestUtility.TestDirectory),
                string.Format("{0}\\0000000000.dtr.zip", TestUtility.TestDirectory),
            };

            foreach (string invalidFilePath in invalidfilePaths)
            {
                File.Create(invalidFilePath).Dispose();
                Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), EventStampInfo.GetLastEventStamp(new FileInfo(invalidFilePath)));
                File.Delete(invalidFilePath);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructFromFileInfosTest()
        {
            List<FileInfo> fileInfos = new List<FileInfo>(this.testFiles.Select(p => new FileInfo(p.Value)));
            EventStampInfo[] result = EventStampInfo.ConstructFromFileInfos(fileInfos).ToArray();

            Assert.AreEqual(this.testFiles.Count(), result.Count());
            Assert.AreEqual(result[result.Length - 1].LastEventStamp, result.Max(p => p.LastEventStamp));

            HashSet<DateTime> firstEventStamps = new HashSet<DateTime>(result.Select(p => p.FirstEventStamp));
            Assert.AreEqual(result.Length, firstEventStamps.Count);
            HashSet<DateTime> lastEventStamps = new HashSet<DateTime>(result.Select(p => p.LastEventStamp));
            Assert.AreEqual(result.Length, lastEventStamps.Count);

            for (int i = 0; i < result.Length; i++)
            {
                if (i == 0)
                {
                    Assert.AreEqual(DateTime.MinValue.ToUniversalTime(), result[i].FirstEventStamp);
                }
                else
                {
                    Assert.AreEqual(result[i - 1].LastEventStamp, result[i].FirstEventStamp);
                }
            }
        }
    }
}