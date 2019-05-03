// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    [TestClass]
    public class ProgramTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryParseArgsTest()
        {
            Dictionary<string, string> parsedValues;

            Assert.IsTrue(Program.TryParseArgs(new string[] { "install" }, out parsedValues));

            Assert.IsFalse(Program.TryParseArgs(new string[] { "heihei" }, out parsedValues));
            Assert.IsFalse(Program.TryParseArgs(new string[] {}, out parsedValues));
        }
    }
}