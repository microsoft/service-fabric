// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for <see cref="Variant"/>.
    /// </summary>
    [TestClass]
    public class VariantTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void DateTimeVariantTest()
        {
            var prettyDate = new DateTime(635875488000000000);
            Variant testValue = prettyDate;
            Assert.AreEqual(typeof(DateTime), testValue.ToObject().GetType());
            Assert.AreEqual(prettyDate.ToString(), testValue.ToObject().ToString());

            // Interpret bits as a long for comparison. Explicit casting is a type conversion under c#
            // so instead typecast to ptr.
            var serializedValue = testValue.ConvertToGuid();
            Assert.AreEqual(prettyDate.ToFileTimeUtc(), *(long*)&serializedValue, "Binary form of Variant is not correct.");
        }

        /// <summary>
        /// Bool types are defined as 4B by legacy windows etw.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void BooleanSizeTest()
        {
            Variant testValue = true;
            Assert.AreEqual((uint)4, testValue.Size);
        }

        /// <summary>
        /// Test Variant Equality
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestVariantEquality()
        {
            Assert.IsTrue(this.VariantEqual(true, true));
            Assert.IsFalse(this.VariantEqual(true, false));
            Assert.IsTrue(this.VariantEqual("one", "one"));
            Assert.IsFalse(this.VariantEqual("one", "One"));

            DateTime time = DateTime.UtcNow;
            Assert.IsTrue(this.VariantEqual(time, time));
            Assert.IsFalse(this.VariantEqual(time, DateTime.UtcNow + TimeSpan.FromMilliseconds(10)));

            Guid guid = Guid.NewGuid();
            Assert.IsTrue(this.VariantEqual(guid, guid));
            Assert.IsFalse(this.VariantEqual(guid, Guid.NewGuid()));
        }

        private bool VariantEqual(Variant first, Variant second)
        {
            return first.Equals(second);
        }
    }
}