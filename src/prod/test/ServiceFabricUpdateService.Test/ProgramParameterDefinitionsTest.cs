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
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using ParameterNames = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ParameterNames;

    [TestClass]
    public class ProgramParameterDefinitionsTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateTest()
        {
            Assert.IsTrue(ProgramParameterDefinitions.Validate(new Dictionary<string, string>() { { "lala", "piapia" } }));
            Assert.IsFalse(ProgramParameterDefinitions.Validate(new Dictionary<string, string>()));
        }
    }
}