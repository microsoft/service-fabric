// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class UploadTaskTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            Task task = new Task(delegate {});
            string src = "hiahia";
            string dst = "heihei";
            UploadTask result = new UploadTask(task, src, dst);

            Assert.IsNotNull(result.Task);
            Assert.AreSame(task, result.Task);
            Assert.AreEqual(src, result.Source);
            Assert.AreEqual(dst, result.Destination);
        }
    }
}