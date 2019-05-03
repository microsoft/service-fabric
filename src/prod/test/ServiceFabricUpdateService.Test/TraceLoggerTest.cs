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
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;
    using Microsoft.Diagnostics.Tracing.Session;

    [TestClass]
    public class TraceLoggerTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WriteLogTest()
        {
            TraceEventProvider.Renew();
            string logDir = TestUtility.TestDirectory + "\\traceLogTest";

            for (int i = 0; i < 10; i++)
            {
                if (!Directory.Exists(logDir))
                {
                    Directory.CreateDirectory(logDir);
                }

                using (TraceLogger logger = new TraceLogger(logDir))
                {
                    logger.WriteInfo("component1", "{0}  {1}", "hia", "lolo");
                    logger.WriteWarning("component1", "{0}  {1}", "hia", "lolo");

                    try
                    {
                        throw new IndexOutOfRangeException("cuo la");
                    }
                    catch (Exception ex)
                    {
                        logger.WriteError("component1", ex, "{0}  {1}", "hia", "lolo");
                    }
                }

                Assert.AreEqual(1, Directory.GetFiles(logDir, "*").Length);
                Directory.Delete(logDir, recursive: true);
            }
        }

        public void RestartTest()
        {
            string logDir = TestUtility.TestDirectory + "\\traceLogTest";
            if (!Directory.Exists(logDir))
            {
                Directory.CreateDirectory(logDir);
            }

            TraceEventProvider.Renew();
            using (TraceLogger logger = new TraceLogger(logDir))
            {
                for (int i = 0; i < 1000; i++)
                {
                    logger.WriteInfo("component1", "{0}  {1}", "hia", "lolo");
                }
            }
            
            int originalFileCount = Directory.GetFiles(logDir, "*").Length;

            System.Threading.Thread.Sleep(5000);
            TraceEventProvider.Renew();
            using (TraceLogger logger = new TraceLogger(logDir))
            {
                logger.WriteInfo("component1", "{0}  {1}", "hia", "lolo");
            }

            int newFileCount = Directory.GetFiles(logDir, "*").Length;
            Assert.AreEqual(1, newFileCount - originalFileCount);
            
            Directory.Delete(logDir, recursive: true);
        }
    }
}