// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing.Test
{
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Tests for validating provisional traces.
    /// </summary>
    [TestClass]
    public class ProvisionalTraceTest
    {
        private static TestConfigReader testReader = new TestConfigReader();

        /// <summary>
        /// This is required for these tests to work. Today FabricEvent singleton internally
        /// create bunch of TraceEvents object and populate them with VariantWriter.
        /// We can update the variant writer in FabricEvent but not on TraceEvent. So if we want
        /// to use our own Variant, we need to update it before TraceEvents are created i.e. at
        /// the start of the tests in this assembly. Other way could be to update FabricEvent
        /// class to add an abstraction to be able to update TraceEvent but that's more work.
        /// </summary>
        /// <param name="context"></param>
        [AssemblyInitialize]
        public static void AssemblyInit(TestContext context)
        {
            FabricEvents.VariantEventWriterOverride = TestVariantWriter.SingleInstance;
        }

        [TestInitialize]
        public void TestInitialize()
        {
            TraceConfig.InitializeFromConfigStore(testReader);
            TraceConfig.SetDefaultLevel(TraceSinkType.ETW, EventLevel.Informational);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestProvisionalTraceExpiredAndFlushed()
        {
            int id = 10;

            // Start Chain.
            FabricEvents.Events.TestProvisionalTraceChainStart(id, "start2");

            // Sleep to give Cache sweep to go through
            Threading.Thread.Sleep(TimeSpan.FromSeconds(AgeBasedCache<int, int>.DefaultCleanupDurationInSec + 1));

            // Validate that the trace was flushed.
            this.ValidateArguments(TestVariantWriter.SingleInstance.ReceivedArgs, id, new List<string> { "start2" });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestProvisionalTraceReplacedAndOnlyEndFlushed()
        {
            int id = 11;

            // Start Chain.
            FabricEvents.Events.TestProvisionalTraceChainStart(id, "start2");

            // End Chain
            FabricEvents.Events.TestProvisionalTraceChainEnd(id, "end2", "end3");

            // Validate that only the 2nd trace got flushed.
            this.ValidateArguments(TestVariantWriter.SingleInstance.ReceivedArgs, id, new List<string> { "end2", "end3" });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestProvisionalTraceReplacedBySelfOnlyLastOneFlushedOnExpire()
        {
            int id = 12;

            // Start Chain.
            FabricEvents.Events.TestProvisionalTraceChainStart(id, "start2");

            // Some sleep between event logs
            Threading.Thread.Sleep(TimeSpan.FromMilliseconds(100));

            // Log the same Event [same ID but different data].
            FabricEvents.Events.TestProvisionalTraceChainStart(id, "end2");

            // Sleep to give Cache sweep to go through
            Threading.Thread.Sleep(TimeSpan.FromSeconds(AgeBasedCache<int, int>.DefaultCleanupDurationInSec + 1));

            // Validate that only the 2nd trace gets flushed.
            this.ValidateArguments(TestVariantWriter.SingleInstance.ReceivedArgs, id, new List<string> { "end2" });
        }

        private void ValidateArguments(IList<Variant[]> argsList, int id, IList<string> args)
        {
            var itemWithMatchId = argsList.SingleOrDefault(item => item.Length > 0 && item[0].Equals(id));

            Assert.IsTrue(itemWithMatchId != null, "Couldn't find Element with ID " + id);
            Assert.IsTrue(itemWithMatchId.Length == 1 + args.Count);
            for (int i = 0; i < args.Count; i++)
            {
                Assert.IsTrue(itemWithMatchId[i + 1].Equals(args[i]), string.Format("Expected : {0}, Actual: {1}", args[i], itemWithMatchId[i]));
            }
        }

        private class TestConfigReader : TraceConfig.IConfigReader
        {
            /// <inheritdoc />
            public IEnumerable<string> GetSections()
            {
                return new[] { "Trace/Etw" };
            }

            /// <inheritdoc />
            public string ReadOptionKey()
            {
                throw new NotImplementedException();
            }

            /// <inheritdoc />
            public string ReadFiltersKey(string configSection)
            {
                return null;
            }

            /// <inheritdoc />
            public int ReadTraceSamplingKey(string configSection)
            {
                return 0;
            }

            /// <inheritdoc />
            public bool ReadTraceProvisionalFeatureStatus(string configSection)
            {
                return true;
            }

            /// <inheritdoc />
            public string ReadPathKey()
            {
                return null;
            }

            /// <inheritdoc />
            public EventLevel ReadTraceLevelKey(string configSection)
            {
                return EventLevel.Informational;
            }
        }

        /// <summary>
        /// Test IVariant Writer
        /// </summary>
        private class TestVariantWriter : IVariantEventWriter
        {
            private IList<Variant[]> argsList = new List<Variant[]>();

            private TestVariantWriter()
            {
            }

            /// <summary>
            /// Expose Singleton Instance.
            /// </summary>
            public static readonly TestVariantWriter SingleInstance = new TestVariantWriter();

            /// <inheritdoc />
            public void VariantWrite(
                ref GenericEventDescriptor genericEventDescriptor,
                int argCount,
                Variant v0 = new Variant(),
                Variant v1 = new Variant(),
                Variant v2 = new Variant(),
                Variant v3 = new Variant(),
                Variant v4 = new Variant(),
                Variant v5 = new Variant(),
                Variant v6 = new Variant(),
                Variant v7 = new Variant(),
                Variant v8 = new Variant(),
                Variant v9 = new Variant(),
                Variant v10 = new Variant(),
                Variant v11 = new Variant(),
                Variant v12 = new Variant(),
                Variant v13 = new Variant())
            {
                var args = new Variant[argCount];
                if (argCount == 1)
                {
                    args[0] = v0;
                }
                if (argCount == 2)
                {
                    args[0] = v0;
                    args[1] = v1;
                }
                if (argCount == 3)
                {
                    args[0] = v0;
                    args[1] = v1;
                    args[2] = v2;
                }

                // Rest Ifs not needed right now.
                this.argsList.Add(args);
            }

            /// <inheritdoc />
            public bool IsEnabled(byte level, long keywords)
            {
                return true;
            }

            public IList<Variant[]> ReceivedArgs
            {
                get { return this.argsList; }
            }
        }
    }
}