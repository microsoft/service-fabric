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
    using ExecutionModes = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ExecutionModes;

    [TestClass]
    public class ProgramConfigTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            AppConfig appConfig = new AppConfig(null, "hiahia", TestUtility.TestDirectory, null, TimeSpan.Zero, TimeSpan.Zero);
            ProgramConfig result = new ProgramConfig(ExecutionModes.Install, appConfig);
            Assert.AreEqual(ExecutionModes.Install, result.Mode);
            Assert.AreSame(appConfig, result.AppConfig);

            result = ProgramConfig.Create(new Dictionary<string, string>() { { ParameterNames.Install, "true" } }, appConfig);
            Assert.AreEqual(ExecutionModes.Install, result.Mode);
            Assert.AreSame(appConfig, result.AppConfig);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetModeTest()
        {
            ExecutionModes result;
            
            result = ProgramConfig.GetMode(new Dictionary<string, string>());
            Assert.AreEqual(ExecutionModes.Service, result);

            result = ProgramConfig.GetMode(new Dictionary<string, string>() { { ParameterNames.Install, "true" } });
            Assert.AreEqual(ExecutionModes.Install, result);
        }
    }
}