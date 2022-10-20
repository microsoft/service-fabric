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
    public class UtilityTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void OutputErrorToConsoleTest()
        {
            Utility.OutputErrorToConsole("hiahia");
            Utility.OutputErrorToConsole("hiahia {0} heihei {1}", "lolo", "bibi");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ExecuteWithRetryTest()
        {
            int tryCount = Utility.ExecuteWithRetry(delegate { }, TimeSpan.FromSeconds(1), 6);
            Assert.AreEqual(1, tryCount);

            try
            {
                Utility.ExecuteWithRetry(delegate { throw new OutOfMemoryException(); }, TimeSpan.FromSeconds(1), 3);
            }
            catch(OutOfMemoryException)
            {
            }

            int globalTry = 1;
            tryCount = Utility.ExecuteWithRetry(
                delegate
                {
                    if (globalTry <= 3)
                    {
                        globalTry++;
                        throw new OutOfMemoryException();
                    }
                },
                TimeSpan.FromSeconds(1),
                6);
            Assert.AreEqual(4, tryCount);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateOrOpenEventSessionTest()
        {
            string sessionName = "hiahiaSession";
            Assert.IsNull(TraceEventSession.GetActiveSession(sessionName));

            string logDir = TestUtility.TestDirectory + "\\hiahiaLogs";
            if (!Directory.Exists(logDir))
            {
                Directory.CreateDirectory(logDir);
            }

            string logfile = logDir + "\\hiahia.etl";
            TraceEventSession session = Utility.CreateOrOpenEventSession(sessionName, logfile);
            session.EnableProvider(TraceEventProvider.Instance.Name);

            // create again
            session = Utility.CreateOrOpenEventSession(sessionName, logfile);
            session.EnableProvider(TraceEventProvider.Instance.Name);
            session.Dispose();

            Assert.IsNull(TraceEventSession.GetActiveSession(sessionName));

            Directory.Delete(logDir, recursive: true);
        }
    }
}