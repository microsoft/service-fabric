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
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    using ExecutionModes = Microsoft.ServiceFabric.ServiceFabricUpdateService.ProgramParameterDefinitions.ExecutionModes;

    [TestClass]
    internal class ServiceFabricUpdateServiceTest : ServiceFabricUpdateService
    {
        public ServiceFabricUpdateServiceTest()
            : base (null, null)
        {
        }

        public ServiceFabricUpdateServiceTest(ProgramConfig serviceConfig, TraceLogger logger)
            : base(serviceConfig, logger)
        {
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            AppConfig appConfig = new AppConfig(new Uri("http://hiahia"), "hiahia", TestUtility.TestDirectory, null, TimeSpan.Zero, TimeSpan.Zero);
            ProgramConfig config = new ProgramConfig(ExecutionModes.Install, appConfig);
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());
            ServiceFabricUpdateService result = new ServiceFabricUpdateService(config, logger);
            Assert.AreSame(config, result.ServiceConfig);
            Assert.AreSame(logger, result.Logger);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void DryRunTest()
        {
            ServiceFabricUpdateServiceTest svc = new ServiceFabricUpdateServiceTest(null, new TraceLogger(new MockUpTraceEventProvider()));
            svc.OnStart(null);
            svc.OnStop();
            svc.OnStart(null);
            svc.OnShutdown();
        }

        protected override ServiceletBase[] OnLoaingServicelets()
        {
            return new ServiceletBase[] { new MockupServicelet(this.ServiceConfig, this.Logger) };
        }
    }
}