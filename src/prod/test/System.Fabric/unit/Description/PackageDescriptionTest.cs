// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    class PackageDescriptionTest
    {
        public static void ComparePackageDescriptionInternal(PackageDescription expected, PackageDescription actual)
        {
            Assert.AreEqual<string>(expected.Name, actual.Name);
            Assert.AreEqual<string>(expected.Version, actual.Version);
        }
    }
}