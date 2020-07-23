// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Linq;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    using Moq;

    [TestClass]
    public class DcaSettingsTest
    {
        private const string TestApplicationInstanceId = "Test123";

        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            Utility.TraceSource = new ErrorAndWarningFreeTraceEventSource();
            HealthClient.Disabled = true;
        }

        [TestMethod]
        public void TestNoProviders()
        {
            var configReader = new Mock<DCASettings.IDcaSettingsConfigReader>();

            var settings = new DCASettings(TestApplicationInstanceId, configReader.Object);

            Assert.AreEqual(0, settings.ProducerInstances.Count);
            Assert.AreEqual(0, settings.ConsumerInstances.Count);
            Assert.AreEqual(0, settings.Errors.Count());
        }

        [TestMethod]
        public void TestSinglePlugin()
        {
            var configReader = new Mock<DCASettings.IDcaSettingsConfigReader>();
            configReader.Setup(cr => cr.GetProducerInstances()).Returns("test1");
            configReader.Setup(cr => cr.GetProducerType("test1")).Returns(StandardPluginTypes.FolderProducer);

            var settings = new DCASettings(TestApplicationInstanceId, configReader.Object);

            Assert.AreEqual(1, settings.ProducerInstances.Count);
            Assert.AreEqual(0, settings.ConsumerInstances.Count);
            Assert.AreEqual(0, settings.Errors.Count()); 
        }

        [TestMethod]
        public void TestSingleInvalidPlugin()
        {
            Utility.TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.FabricDCA);
            var configReader = new Mock<DCASettings.IDcaSettingsConfigReader>();
            configReader.Setup(cr => cr.GetProducerInstances()).Returns("test1");
            configReader.Setup(cr => cr.GetProducerType("test1")).Returns("invalidPlugin");

            var settings = new DCASettings(TestApplicationInstanceId, configReader.Object);

            Assert.AreEqual(0, settings.ProducerInstances.Count);
            Assert.AreEqual(0, settings.ConsumerInstances.Count);
            Assert.AreEqual(1, settings.Errors.Count());
        }
    }
}