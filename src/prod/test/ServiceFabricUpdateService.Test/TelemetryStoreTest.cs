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
    using TelemetryAggregation;

    [TestClass]
    public class TelemetryStoreTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            string dcaRootStoreLocation = "biabia";
            TraceLogger traceLogger = new TraceLogger(new MockUpTraceEventProvider());
            TelemetryStore result = new TelemetryStore(dcaRootStoreLocation, traceLogger);
            Assert.AreEqual(dcaRootStoreLocation, result.DiagnosticsStoreRootLocation);
            Assert.AreSame(traceLogger, result.Logger);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetTelemetriesTest()
        {
            string dcaRootFolder = Path.Combine(TestUtility.TestDirectory, "myDcaLocation11");
            TelemetryStoreTest.PrepareTelemetryData(dcaRootFolder);

            TelemetryStore store = new TelemetryStore(dcaRootFolder, new TraceLogger(new MockUpTraceEventProvider()));
            TelemetryCollection[] result = store.GetTelemetries();
            Assert.AreEqual(4, result.Length);
            Assert.IsTrue(result.All(p => p != null));

            Directory.Delete(dcaRootFolder, recursive: true);
        }

        internal static void PrepareTelemetryData(string dcaRootFolder)
        {
            string telemetryFolder = Path.Combine(dcaRootFolder, "fabrictelemetries-09");

            for (int i = 0; i < 5; i++)
            {
                string directory = Path.Combine(telemetryFolder, "vm" + i.ToString());
                Directory.CreateDirectory(directory);
                if (i < 4)
                {
                    File.Copy(Path.Combine(TestUtility.TestDirectory, "telemetry111.json"), Path.Combine(directory, "tfile.json"));
                }
            }
        }
    }
}