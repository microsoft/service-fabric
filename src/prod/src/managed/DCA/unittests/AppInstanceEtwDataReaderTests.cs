// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Common.Tracing;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Tools.EtlReader;

    /// <summary>
    /// Provides unit tests for FabricDCA.
    /// </summary>
    [TestClass]
    public class AppInstanceEtwDataReaderTests
    {
        /// <summary>
        /// Set static state shared between all tests.
        /// </summary>
        /// <param name="context">Test context.</param>
        [ClassInitialize]
        public static void TestSetup(TestContext context)
        {
            Utility.TraceSource = new ErrorAndWarningFreeTraceEventSource(FabricEvents.Tasks.FabricDCA);
        }

        /// <summary>
        /// Tests that an external writer (MA, WAD) writing event 0 heartbeats is safe.
        /// </summary>
        [TestMethod]
        public void TestExternalEventWriterEventZeroIsHandled()
        {
            const string providerName = "TestProvider";
            var providerId = Guid.NewGuid();
            var manifestCache = new ManifestCache(
                new Dictionary<Guid, ProviderDefinition>
                    {
                        { providerId, new ProviderDefinition(new ProviderDefinitionDescription(providerId, providerName, new Dictionary<int, EventDefinitionDescription>()))}
                    }, 
                new Dictionary<string, List<Guid>>());
            var reader = new TestableAppInstanceEtwDataReader(manifestCache);
            reader.TestableOnEtwEventReceived(null, new EventRecordEventArgs(new EventRecord
            {
                EventHeader = new EventHeader
                {
                    EventDescriptor = new EventDescriptor
                    {
                        Id = 0
                    },
                    TimeStamp = DateTime.Now.ToFileTimeUtc()
                }
            }));
        }

        private class TestableAppInstanceEtwDataReader : AppInstanceEtwDataReader
        {
            public TestableAppInstanceEtwDataReader(ManifestCache manifestCache)
                : base((name, id, time, packageName, record, timestamp, file) => { },
                    (name, id, packageName, timestamp, file, inactivity) => { },
                    manifestCache)
            {
            }

            public void TestableOnEtwEventReceived(object sender, EventRecordEventArgs e)
            {
                this.OnEtwEventReceived(sender, e);
            }
        }
    }
}