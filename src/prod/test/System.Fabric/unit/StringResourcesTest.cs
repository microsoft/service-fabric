// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Fabric;
    using System.Fabric.Strings;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class StringResourcesTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("alexwun")]
        public void ReadResource()
        {
            Assert.IsFalse(StringResources.General_Reserved.Length <= 0);
        }
    }
}