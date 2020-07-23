// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using Microsoft.ServiceFabric.DeploymentManager.Model;

    [TestClass]
    public class TelemetryCookieTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            string filePath = "hiahia";
            TraceLogger traceLogger = new TraceLogger(new MockUpTraceEventProvider());
            DateTime lastProcessedTimeUtc = new DateTime(2001, 1, 1).ToUniversalTime();
            TelemetryCookie result = new TelemetryCookie(filePath, traceLogger, lastProcessedTimeUtc, 5);

            Assert.AreEqual(filePath, result.FilePath);
            Assert.AreSame(traceLogger, result.Logger);
            Assert.AreEqual(lastProcessedTimeUtc, result.LastProcessedUtc);
            Assert.AreEqual(5, result.TelemetriesLastUploaded);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void LoadAndAdvanceTest()
        {
            string filePath = Path.Combine(TestUtility.TestDirectory, "cookieFile1.json");
            TraceLogger traceLogger = new TraceLogger(new MockUpTraceEventProvider());
            DateTime lastProcessedUtc = DateTime.MinValue.ToUniversalTime();

            Assert.IsFalse(File.Exists(filePath));
            TelemetryCookie result = TelemetryCookie.LoadFromFile(filePath, traceLogger);
            Assert.AreEqual(filePath, result.FilePath);
            Assert.AreSame(traceLogger, result.Logger);
            Assert.AreEqual(lastProcessedUtc, result.LastProcessedUtc);
            Thread.Sleep(1000);

            for (int i = 0; i < 2; i++)
            {
                result.Advance(50);
                Assert.IsTrue(File.Exists(filePath));
                result = TelemetryCookie.LoadFromFile(filePath, traceLogger);
                Assert.AreEqual(filePath, result.FilePath);
                Assert.AreSame(traceLogger, result.Logger);
                Assert.AreEqual(50, result.TelemetriesLastUploaded);
                Assert.IsTrue(result.LastProcessedUtc > lastProcessedUtc);
                lastProcessedUtc = result.LastProcessedUtc;
                Thread.Sleep(1000);
            }

            File.Delete(filePath);
        }
    }
}