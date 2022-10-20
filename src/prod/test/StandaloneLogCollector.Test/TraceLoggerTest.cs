// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class TraceLoggerTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WriteLogTest()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                traceLogger.WriteInfo("hiaType", "{0} {1}", "2", "3");
                traceLogger.WriteError("hiaType", new ArgumentNullException(), "{0} {1}", "2", "3");
                traceLogger.WriteWarning("hiaType", "{0} {1}", "2", "3");
                traceLogger.WriteWarning("hiaType", new ArgumentNullException(), "{0} {1}", "2", "3");
            }

            File.Delete(logPath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WriteLogEscapeBracesTest()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                Exception ex = new Exception("{EscapeBraces}");
                traceLogger.WriteError("hiaType", ex, "TestException: {0}", ex.Message);
                traceLogger.WriteWarning("hiaType", ex, "TestException: {0}", ex.Message);
            }

            File.Delete(logPath);
        }
    }
}