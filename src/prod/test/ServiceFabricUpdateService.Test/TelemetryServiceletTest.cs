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
    public class TelemetryServiceletTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            ProgramConfig svcConfig = new ProgramConfig(
                ProgramParameterDefinitions.ExecutionModes.Service,
                new AppConfig(new Uri("http://hiahia.net:433", UriKind.Absolute), null, TestUtility.TestDirectory, new Uri("http://goalstate"), TimeSpan.FromHours(12), TimeSpan.FromHours(24)));
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());

            TelemetryServicelet result = new TelemetryServicelet(svcConfig, logger);
            Assert.IsFalse(result.IsEnabled);
            Assert.IsNull(result.Timer);

            svcConfig = new ProgramConfig(
                ProgramParameterDefinitions.ExecutionModes.Service,
                new AppConfig(new Uri("http://hiahia.net:433", UriKind.Absolute), "hiahiaRoot111", TestUtility.TestDirectory, new Uri("http://goalstate"), TimeSpan.FromHours(12), TimeSpan.FromHours(23)));

            result = new TelemetryServicelet(svcConfig, logger);
            Assert.IsTrue(result.IsEnabled);
            Assert.IsNotNull(result.Timer);
            Assert.IsNotNull(result.Store);
            Assert.IsNotNull(result.Uploader);
            Assert.AreEqual(TimeSpan.FromHours(23), result.ScheduleInterval);
            Assert.IsNotNull(result.Cookie);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void IntegrationTest()
        {
            string dcaRootLocation = Path.Combine(TestUtility.TestDirectory, "IntegrationDca22");
            TelemetryStoreTest.PrepareTelemetryData(dcaRootLocation);

            TraceLogger traceLogger = new TraceLogger(new MockUpTraceEventProvider());
            ProgramConfig config = new ProgramConfig(
                ProgramParameterDefinitions.ExecutionModes.Service,
                new AppConfig(
                    new Uri("http://localhost", UriKind.Absolute),
                    dcaRootLocation,
                    TestUtility.TestDirectory + "\\Data",
                    new Uri("http://localhost", UriKind.Absolute),
                    TimeSpan.FromSeconds(300000),
                    TimeSpan.FromSeconds(2)));

            this.MockUpIntegrationTest(config, traceLogger);
            this.RealIntegrationTest(config, traceLogger);

            Directory.Delete(TestUtility.TestDirectory + "\\Data", recursive: true);
            Directory.Delete(dcaRootLocation, recursive: true);
        }

        private void MockUpIntegrationTest(ProgramConfig config, TraceLogger traceLogger)
        {
            MockUpTelemetryUploader uploader = new MockUpTelemetryUploader();
            TelemetryServicelet server = new TelemetryServicelet(config, traceLogger, uploader);
            server.Start();

            // ensure that the telemetry data have been uploaded at least twice
            Thread.Sleep(10000);

            Assert.IsTrue(uploader.TelemetriesUploaded > 8);
            Assert.IsTrue(server.Cookie.LastProcessedUtc > DateTime.UtcNow - TimeSpan.FromSeconds(5));
            Assert.AreEqual(4, server.Cookie.TelemetriesLastUploaded);

            server.Stop();

            int originalUploaded = uploader.TelemetriesUploaded;
            Thread.Sleep(5000);
            Assert.AreEqual(originalUploaded, uploader.TelemetriesUploaded);
        }

        private void RealIntegrationTest(ProgramConfig config, TraceLogger traceLogger)
        {
            TelemetryServicelet server = new TelemetryServicelet(config, traceLogger);
            server.Start();

            // ensure that the telemetry data have been uploaded at least twice
            Thread.Sleep(10000);

            Assert.IsTrue(server.Cookie.LastProcessedUtc > DateTime.UtcNow - TimeSpan.FromSeconds(5));
            Assert.AreEqual(4, server.Cookie.TelemetriesLastUploaded);

            server.Stop();
        }
    }
}