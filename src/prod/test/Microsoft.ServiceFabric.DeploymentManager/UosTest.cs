// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Fabric.Description;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class UosTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConfigUpgradeDescConstructorTest()
        {
            ConfigurationUpgradeDescription desc = new ConfigurationUpgradeDescription();
            Assert.AreEqual(null, desc.ClusterConfiguration);
            Assert.AreEqual(TimeSpan.FromSeconds(600), desc.HealthCheckRetryTimeout);
            Assert.AreEqual(TimeSpan.Zero, desc.HealthCheckStableDuration);
            Assert.AreEqual(TimeSpan.Zero, desc.HealthCheckWaitDuration);
            Assert.AreEqual(0, desc.MaxPercentDeltaUnhealthyNodes);
            Assert.AreEqual(0, desc.MaxPercentUnhealthyApplications);
            Assert.AreEqual(0, desc.MaxPercentUnhealthyNodes);
            Assert.AreEqual(0, desc.MaxPercentUpgradeDomainDeltaUnhealthyNodes);
            Assert.AreEqual(TimeSpan.MaxValue, desc.UpgradeDomainTimeout);
            Assert.AreEqual(TimeSpan.MaxValue, desc.UpgradeTimeout);

            Console.WriteLine(desc.ToStringDescription());
        }
    }
}