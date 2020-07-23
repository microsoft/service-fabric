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
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class ParameterDefinitionTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            ParameterDefinition definition = new ParameterDefinition("hia", true, 3, 4);
            Assert.AreEqual("hia", definition.Name);
            Assert.AreEqual(true, definition.IsSwitch);
            Assert.AreEqual(3, definition.ParameterSets);
            Assert.AreEqual(4, definition.DefaultValue);
        }
    }
}