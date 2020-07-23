// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Fabric.Management.ImageBuilder.SingleInstance;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ApplicationPackageGeneratorTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryMatchSecretRefTest_ValidSecretReference()
        {
            var validSecretRef = "[reference('seCrets/secretName_-10/valUes/1.0-alpha_Beta')]";
            if (!ApplicationPackageGenerator.TryMatchSecretRef(ref validSecretRef))
            {
                Assert.Fail("Valid secret ref is treated as invalid.");
            }

            Assert.AreEqual("secretName_-10:1.0-alpha_Beta", validSecretRef);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryMatchSecretRefTest_InvalidSecretReference()
        {
            // Invalid secret reference: secret name contains ".";
            var invalidSecretRef = "[reference('secrets/secretName._-10/values/1.0-alpha_Beta')]";
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref invalidSecretRef))
            {
                Assert.Fail("Invalid secret ref (secret name contains '.') is treated as valid.");
            }

            // Invalid secret reference: invalid format.
            invalidSecretRef = "[ref(\"secrets/secretName_-10/values/1.0-alpha_Beta\")]";
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref invalidSecretRef))
            {
                Assert.Fail("Invalid secret ref (invalid format) is treated as valid.");
            }

            // Invalid secret reference: invalid version.
            invalidSecretRef = "[reference('secrets/secretName._-10/values/*')]";
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref invalidSecretRef))
            {
                Assert.Fail("Invalid secret ref (invalid version) is treated as valid.");
            }

            // Invalid secret reference: missing version.
            invalidSecretRef = "[reference('secrets/secretName._-10/values/')]";
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref invalidSecretRef))
            {
                Assert.Fail("Invalid secret ref (missing version) is treated as valid.");
            }

            // Invalid secret reference: missing secret name.
            invalidSecretRef = "[reference('secrets//values/1.0')]";
            if (ApplicationPackageGenerator.TryMatchSecretRef(ref invalidSecretRef))
            {
                Assert.Fail("Invalid secret ref (missing secret name) is treated as valid.");
            }
        }
    }
}
