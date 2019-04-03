// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing.Test
{
    using System.Diagnostics.Tracing;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;

    /// <summary>
    /// Provides unit tests for the <see cref="TraceConfig"/> class.
    /// </summary>
    [TestClass]
    public class TraceConfigTest
    {
        private const string ConfigFileName = "TraceConfigTest.cfg";
        private static readonly object ContextLock = new object();

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestEmptyConfigStoreTraceConfig()
        {
            lock (ContextLock)
            {
                var mockConfigReader = new Mock<TraceConfig.IConfigReader>(MockBehavior.Strict);
                mockConfigReader.Setup(cr => cr.GetSections()).Returns(new string[0]);
                TraceConfig.InitializeFromConfigStore(mockConfigReader.Object);
                AssertDefaultTracingConfig();
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestConfigStoreTraceConfig()
        {
            lock (ContextLock)
            {
                var onFilterUpdateCalledCount = 0;
                TraceConfig.OnFilterUpdate += sinkType =>
                {
                    switch (sinkType)
                    {
                        case TraceSinkType.ETW:
                            Assert.IsTrue(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Informational, EventTask.None, string.Empty),
                                "ETW should allow info traces.");
                            Assert.IsFalse(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Verbose, EventTask.None, string.Empty),
                                "ETW shouldn't allow verbose traces.");
                            Assert.IsTrue(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Verbose, FabricEvents.Tasks.FabricDCA, "ServiceResolver"),
                                "ETW should allow DCA.ServiceResolver verbose traces.");
                            Assert.IsTrue(TraceConfig.GetEventProvisionalFeatureStatus(sinkType), "ETW should have Provisional Featue Enabled");
                            break;
                        case TraceSinkType.Console:
                            Assert.IsTrue(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Error, EventTask.None, string.Empty),
                                "Console should allow error traces.");
                            Assert.IsFalse(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Warning, EventTask.None, string.Empty),
                                "Console shouldn't allow warning traces.");
                            break;
                        case TraceSinkType.TextFile:
                            Console.WriteLine("Entering - TextFile");
                            Assert.IsTrue(
                                TraceConfig.GetEventEnabledStatus(sinkType, EventLevel.Verbose, EventTask.None, string.Empty),
                                "Text file should allow all traces.");
                            break;
                        default:
                            Assert.Fail("Unknown sink type filter updated" + sinkType);
                            break;
                    }

                    onFilterUpdateCalledCount++;
                };

                var mockConfigReader = new Mock<TraceConfig.IConfigReader>(MockBehavior.Strict);
                mockConfigReader.Setup(cr => cr.GetSections()).Returns(new[] { "Trace/File", "Trace/Etw", "Trace/Console" });

                SetMockConfig(mockConfigReader, "Trace/File", EventLevel.Verbose, null, "DoesntMatter.trace");

                SetMockConfig(mockConfigReader, "Trace/Etw", EventLevel.Informational, "FabricDCA.ServiceResolver:5");

                SetMockConfig(mockConfigReader, "Trace/Console", EventLevel.Error);

                TraceConfig.InitializeFromConfigStore(mockConfigReader.Object);
                Assert.AreEqual(3, onFilterUpdateCalledCount);
            }
        }

        private static void SetMockConfig(
            Mock<TraceConfig.IConfigReader> mockConfigReader,
            string section,
            EventLevel level,
            string filter = null,
            string path = null,
            int samplingRatio = 0,
            bool isProvisionalFeatureEnabled = true)
        {
            mockConfigReader.Setup(cr => cr.ReadTraceLevelKey(section)).Returns(level);

            mockConfigReader.Setup(cr => cr.ReadFiltersKey(section)).Returns(filter);

            if (!string.IsNullOrEmpty(path))
            {
                mockConfigReader.Setup(cr => cr.ReadPathKey()).Returns(path);
            }

            mockConfigReader.Setup(cr => cr.ReadTraceSamplingKey(section)).Returns(samplingRatio);

            mockConfigReader.Setup(cr => cr.ReadOptionKey()).Returns(string.Empty);

            mockConfigReader.Setup(cr => cr.ReadTraceProvisionalFeatureStatus(section)).Returns(isProvisionalFeatureEnabled);
        }

        private static void AssertDefaultTracingConfig()
        {
            // ETW/Text should allow info level traces
            Assert.IsTrue(
                TraceConfig.GetEventEnabledStatus(TraceSinkType.ETW, EventLevel.Informational, EventTask.None, string.Empty),
                "ETW should allow info traces.");
            Assert.IsTrue(
                TraceConfig.GetEventEnabledStatus(TraceSinkType.TextFile, EventLevel.Informational, EventTask.None, string.Empty),
                "Text file should allow info traces.");
            Assert.IsFalse(
                TraceConfig.GetEventEnabledStatus(TraceSinkType.ETW, EventLevel.Verbose, EventTask.None, string.Empty),
                "ETW shouldn't allow verbose traces.");
            Assert.IsFalse(
                TraceConfig.GetEventEnabledStatus(TraceSinkType.TextFile, EventLevel.Verbose, EventTask.None, string.Empty),
                "Text file shouldn't allow verbose traces.");

            Assert.IsFalse(
                TraceConfig.GetEventEnabledStatus(TraceSinkType.Console, EventLevel.Critical, EventTask.None, string.Empty),
                "Console sink should be disabled.");
        }
    }
}