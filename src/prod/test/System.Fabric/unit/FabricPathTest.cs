// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Fabric.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class FabricPathTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("subiswal")]
        public void GetUncPathTest()
        {
            Assert.AreEqual("\\\\?\\D:\\path", FabricPath.GetUncPath("D:\\path"));
            Assert.AreEqual("\\\\?\\UNC\\server\\share", FabricPath.GetUncPath("\\\\server\\share"));
            Assert.AreEqual("\\\\?\\D:\\path\\abc", FabricPath.GetUncPath("D:\\path\\\\abc"));
            Assert.AreEqual("\\\\?\\D:\\path", FabricPath.GetUncPath("\\\\?\\D:\\path"));
            Assert.AreEqual("path", FabricPath.GetUncPath("path"));
            Assert.AreEqual("D:\\path\\..\\abc", FabricPath.GetUncPath("D:\\path\\..\\abc"));
        }
    }
}