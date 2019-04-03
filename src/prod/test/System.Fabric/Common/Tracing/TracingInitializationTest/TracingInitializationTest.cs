// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Diagnostics.Eventing;
    using System.Fabric.Common.Tracing;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides tests for the static initialization of the FabricEvents class.
    /// </summary>
    [TestClass]
    public class TracingInitializationTest
    {
        /// <summary>
        /// Tests that without additional config info traces are sent to ETW.  Noise/verbose should be filtered.
        /// </summary>
        [TestMethod]
        public void TestTracingInitialization()
        {
            var mockEventWriter = new VariantEventWriterMock
            {
                IsEnabled = (level, keyword) => true,
                VariantWrite = (desc, argCount) => { Assert.AreEqual(8, argCount); }
            };

            FabricEvents.VariantEventWriterOverride = mockEventWriter;

            // Specific trace and params don't matter. Should be Info level.
            FabricEvents.Events.TestStructuredTrace8(
                0, 
                null, 
                DateTime.UtcNow, 
                default(Guid), 
                0, 
                0, 
                false, 
                0);

            // Doesn't have same arg count. Noise should be disabled by default.
            FabricEvents.Events.FabricDCA_NoiseText(null, null, null);

            Assert.IsTrue(mockEventWriter.VariantWriteWasCalled, "Expect to receive a write call without additional configuration.");
            Assert.IsTrue(mockEventWriter.IsEnabledWasCalled);
        }

        /// <summary>
        /// Normal mock doesn't support ref parameters as used in VariantWrite.
        /// </summary>
        private class VariantEventWriterMock : IVariantEventWriter
        {
            public bool VariantWriteWasCalled { get; set; }

            public Action<GenericEventDescriptor, int> VariantWrite { get; set; }

            public bool IsEnabledWasCalled { get; set; }

            public Func<byte, long, bool> IsEnabled { get; set; }

            void IVariantEventWriter.VariantWrite(ref GenericEventDescriptor eventDescriptor, int argCount, Variant v0, Variant v1, Variant v2, Variant v3, Variant v4, Variant v5, Variant v6, Variant v7, Variant v8, Variant v9, Variant v10, Variant v11, Variant v12, Variant v13)
            {
                this.VariantWriteWasCalled = true;
                this.VariantWrite(eventDescriptor, argCount);
            }

            bool IVariantEventWriter.IsEnabled(byte level, long keywords)
            {
                this.IsEnabledWasCalled = true;
                return this.IsEnabled(level, keywords);
            }
        }
    }
}