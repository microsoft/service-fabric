// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for <see cref="VariantId"/>.
    /// </summary>
    [TestClass]
    public class VariantIdTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestVariantIdBounds()
        {
            int count = 5;
            VariantId variantId = new VariantId(count);

            // Add upto capacity
            for (int i = 0; i < count; ++i)
            {
                variantId.AddIdElement("element" + i);
            }

            Exception exceptionEncountered = null;
            try
            {
                variantId.AddIdElement("outofboundElement");
            }
            catch (Exception exp)
            {
                exceptionEncountered = exp;
            }

            Assert.IsTrue(
                exceptionEncountered != null && exceptionEncountered.GetType() == typeof(InvalidOperationException),
                "An exception is expected when we add elements beyond capacity");
        }

        /// <summary>
        /// Test Variant ID Equality function
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestVariantEquality()
        {
            // Self-Equality
            VariantId first = new VariantId(2);
            first.AddIdElement("one");
            first.AddIdElement("two");
            Assert.IsTrue(first.Equals(first));

            // Equality with a larger capacity variant
            VariantId larger = new VariantId(3);
            larger.AddIdElement("one");
            larger.AddIdElement("two");
            Assert.IsFalse(first.Equals(larger));

            // Equality with a similar size but underfilled
            VariantId third = new VariantId(2);
            third.AddIdElement("one");
            Assert.IsFalse(first.Equals(third));

            // Equality with similar ID
            third.AddIdElement("two");
            Assert.IsTrue(first.Equals(third));

            // Equality with same size and values but different order
            VariantId fourth = new VariantId(2);
            fourth.AddIdElement("two");
            fourth.AddIdElement("one");
            Assert.IsFalse(first.Equals(fourth));
        }
    }
}