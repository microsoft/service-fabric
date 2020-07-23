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
    using System.Reflection;
    using System.ServiceProcess;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using ExecutionModes = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ExecutionModes;

    [TestClass]
    public class WindowsServiceInstallerTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ExecuteTest()
        {
            ProgramConfig config;
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());

            Assert.IsFalse(ServiceController.GetServices().Any(p => p.ServiceName == "ServiceFabricUpdateService"));

            config = new ProgramConfig(ExecutionModes.Install, null);
            WindowsServiceInstaller.Execute(config, logger);
            while (!ServiceController.GetServices().Any(p => p.ServiceName == "ServiceFabricUpdateService"))
            {
                Thread.Sleep(3000);
            }

            TestUtility.UninstallService("ServiceFabricUpdateService");
            while (ServiceController.GetServices().Any(p => p.ServiceName == "ServiceFabricUpdateService"))
            {
                Thread.Sleep(3000);
            }
        }
    }
}