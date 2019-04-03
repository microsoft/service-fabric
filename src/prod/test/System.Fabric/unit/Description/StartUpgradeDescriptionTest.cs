// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Interop;

    [TestClass]
    class StartUpgradeDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<ConfigurationUpgradeDescription, object>
        {
            public override ConfigurationUpgradeDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override ConfigurationUpgradeDescription CreateDefaultDescription()
            {
                var configUpgradeDesc = new ConfigurationUpgradeDescription()
                {
                    ClusterConfiguration = "config path",
                };

                var serviceTypeHealthPolicy = new Health.ServiceTypeHealthPolicy();
                serviceTypeHealthPolicy.MaxPercentUnhealthyServices = 15;

                var appHealthPolicy = new Health.ApplicationHealthPolicy();
                appHealthPolicy.ServiceTypeHealthPolicyMap.Add("TestService", serviceTypeHealthPolicy);

                configUpgradeDesc.ApplicationHealthPolicies.Add(new Uri("fabric:/TestApp1"), appHealthPolicy);

                return configUpgradeDesc;
            }

            public override void Compare(ConfigurationUpgradeDescription expected, ConfigurationUpgradeDescription actual)
            {
                Assert.AreEqual<string>(expected.ClusterConfiguration, actual.ClusterConfiguration);
                Assert.AreEqual<int>(expected.ApplicationHealthPolicies.Count, actual.ApplicationHealthPolicies.Count);

                Health.ApplicationHealthPolicy expectedHealthPolicy;
                if (expected.ApplicationHealthPolicies.TryGetValue(new Uri("fabric:/TestApp1"), out expectedHealthPolicy))
                {
                    Health.ApplicationHealthPolicy actualHealthPolicy;
                    if (actual.ApplicationHealthPolicies.TryGetValue(new Uri("fabric:/TestApp1"), out actualHealthPolicy))
                    {
                        Assert.IsTrue(Health.ApplicationHealthPolicy.AreEqual(expectedHealthPolicy, actualHealthPolicy));
                    }
                    else
                    {
                        Assert.Fail("Application Name not found in actual value");
                    }
                }
                else
                {
                    Assert.Fail("Application Name not found in expected value");
                }
                Assert.AreEqual<string>(expected.ApplicationHealthPolicies.ToString(), actual.ApplicationHealthPolicies.ToString());
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("jkochhar")]
        public void ConfigurationUpgradeDescription_ParseTest()
        {
            ConfigurationUpgradeDescription expected = HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                IntPtr native = expected.ToNative(pc);

                ConfigurationUpgradeDescription actual = ConfigurationUpgradeDescription.CreateFromNative(native);

                HelperInstance.Compare(expected, actual);
            }
        }
    }
}
