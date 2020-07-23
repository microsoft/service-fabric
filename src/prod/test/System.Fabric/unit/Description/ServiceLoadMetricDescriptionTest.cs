// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class StatefulServiceLoadMetricDescriptionTest
    {
        internal class StatefulHelper : DescriptionElementTestBase<StatefulServiceLoadMetricDescription, ServiceLoadMetricInfo>
        {

            public override StatefulServiceLoadMetricDescription Factory(ServiceLoadMetricInfo info)
            {
                return (StatefulServiceLoadMetricDescription)ServiceLoadMetricDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()), true);
            }

            public override ServiceLoadMetricInfo CreateDefaultInfo()
            {
                return new ServiceLoadMetricInfo
                {
                    PrimaryDefaultLoad = 23,
                    SecondaryDefaultLoad = 22,
                    Name = "foo_metric_info",
                    Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW
                };
            }

            public override StatefulServiceLoadMetricDescription CreateDefaultDescription()
            {
                return new StatefulServiceLoadMetricDescription
                {
                    PrimaryDefaultLoad = 23,
                    SecondaryDefaultLoad = 22,
                    Name = "foo_metric_info",
                    Weight = ServiceLoadMetricWeight.Low,
                };
            }

            public StatefulServiceLoadMetricDescription CreateDefaultMetrics(int i)
            {
                switch (i)
                {
                    case 0:
                        return new StatefulServiceLoadMetricDescription
                        {
                            PrimaryDefaultLoad = 0,
                            SecondaryDefaultLoad = 0,
                            Name = "PrimaryCount",
                            Weight = ServiceLoadMetricWeight.High,
                        };
                    case 1:
                        return new StatefulServiceLoadMetricDescription
                        {
                            PrimaryDefaultLoad = 0,
                            SecondaryDefaultLoad = 0,
                            Name = "ReplicaCount",
                            Weight = ServiceLoadMetricWeight.Medium,
                        };
                    case 2:
                        return new StatefulServiceLoadMetricDescription
                        {
                            PrimaryDefaultLoad = 0,
                            SecondaryDefaultLoad = 0,
                            Name = "Count",
                            Weight = ServiceLoadMetricWeight.Low,
                        };
                    default:
                        return new StatefulServiceLoadMetricDescription
                        {
                            PrimaryDefaultLoad = 23,
                            SecondaryDefaultLoad = 22,
                            Name = "foo_metric_info",
                            Weight = ServiceLoadMetricWeight.Low,
                        };
                }
            }

            public override void Compare(StatefulServiceLoadMetricDescription expected, StatefulServiceLoadMetricDescription actual)
            {
                Assert.AreEqual(expected.Name, actual.Name);
                Assert.AreEqual<ServiceLoadMetricWeight>(expected.Weight, actual.Weight);
                Assert.AreEqual(expected.PrimaryDefaultLoad, actual.PrimaryDefaultLoad);
                Assert.AreEqual(expected.SecondaryDefaultLoad, actual.SecondaryDefaultLoad);
            }


            public ServiceLoadMetricInfo CreateIsolatedServiceLoadMetricInfo()
            {
                return new ServiceLoadMetricInfo
                {
                    PrimaryDefaultLoad = 20,
                    SecondaryDefaultLoad = 19,
                    Name = "foo_metric_info_1",
                    Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM
                };
            }

            public StatefulServiceLoadMetricDescription CreateIsolatedServiceLoadMetricDescription()
            {
                return new StatefulServiceLoadMetricDescription
                {
                    PrimaryDefaultLoad = 20,
                    SecondaryDefaultLoad = 19,
                    Name = "foo_metric_info_1",
                    Weight = ServiceLoadMetricWeight.Medium
                };
            }
        }

        internal static StatefulHelper StatefulHelperInstance
        {
            get { return new StatefulHelper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceLoadMetricDescription_ParseBasic()
        {
            StatefulHelperInstance.ParseTestHelper(null, null);
        }

        private void WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT native, ServiceLoadMetricWeight managed)
        {
            StatefulHelperInstance.ParseTestHelper((info) => info.Weight = native, (description) => description.Weight = managed);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceLoadMetricDescription_WeightTranslation()
        {
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, ServiceLoadMetricWeight.High);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW, ServiceLoadMetricWeight.Low);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM, ServiceLoadMetricWeight.Medium);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO, ServiceLoadMetricWeight.Zero);
        }
    }

    [TestClass]
    public class StatelessServiceLoadMetricDescriptionTest
    {
        internal class StatelessHelper : DescriptionElementTestBase<StatelessServiceLoadMetricDescription, ServiceLoadMetricInfo>
        {

            public override StatelessServiceLoadMetricDescription Factory(ServiceLoadMetricInfo info)
            {
                return (StatelessServiceLoadMetricDescription)ServiceLoadMetricDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()), false);
            }

            public override ServiceLoadMetricInfo CreateDefaultInfo()
            {
                return new ServiceLoadMetricInfo
                {
                    PrimaryDefaultLoad = 23,
                    SecondaryDefaultLoad = 0,
                    Name = "bar_metric_info",
                    Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW
                };
            }

            public override StatelessServiceLoadMetricDescription CreateDefaultDescription()
            {
                return new StatelessServiceLoadMetricDescription
                {
                    DefaultLoad = 23,
                    Name = "bar_metric_info",
                    Weight = ServiceLoadMetricWeight.Low,
                };
            }

            public StatelessServiceLoadMetricDescription CreateDefaultMetrics(int i)
            {
                switch (i)
                {
                    case 0:
                        return new StatelessServiceLoadMetricDescription
                        {
                            DefaultLoad = 0,
                            Name = "InstanceCount",
                            Weight = ServiceLoadMetricWeight.High,
                        };
                    case 1:
                        return new StatelessServiceLoadMetricDescription
                        {
                            DefaultLoad = 0,
                            Name = "Count",
                            Weight = ServiceLoadMetricWeight.Medium,
                        };
                    default:
                        return new StatelessServiceLoadMetricDescription
                        {
                            DefaultLoad = 23,
                            Name = "bar_metric_info",
                            Weight = ServiceLoadMetricWeight.Low,
                        };
                }
            }

            public override void Compare(StatelessServiceLoadMetricDescription expected, StatelessServiceLoadMetricDescription actual)
            {
                Assert.AreEqual(expected.Name, actual.Name);
                Assert.AreEqual<ServiceLoadMetricWeight>(expected.Weight, actual.Weight);
                Assert.AreEqual(expected.DefaultLoad, actual.DefaultLoad);
            }


            public ServiceLoadMetricInfo CreateIsolatedServiceLoadMetricInfo()
            {
                return new ServiceLoadMetricInfo
                {
                    PrimaryDefaultLoad = 20,
                    SecondaryDefaultLoad = 0,
                    Name = "bar_metric_info_1",
                    Weight = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM
                };
            }

            public StatelessServiceLoadMetricDescription CreateIsolatedServiceLoadMetricDescription()
            {
                return new StatelessServiceLoadMetricDescription
                {
                    DefaultLoad = 20,
                    Name = "bar_metric_info_1",
                    Weight = ServiceLoadMetricWeight.Medium
                };
            }
        }

        internal static StatelessHelper StatelessHelperInstance
        {
            get { return new StatelessHelper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceLoadMetricDescription_ParseBasic()
        {
            StatelessHelperInstance.ParseTestHelper(null, null);
        }

        private void WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT native, ServiceLoadMetricWeight managed)
        {
            StatelessHelperInstance.ParseTestHelper((info) => info.Weight = native, (description) => description.Weight = managed);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceLoadMetricDescription_WeightTranslation()
        {
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, ServiceLoadMetricWeight.High);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW, ServiceLoadMetricWeight.Low);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM, ServiceLoadMetricWeight.Medium);
            this.WeightTestHelper(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO, ServiceLoadMetricWeight.Zero);
        }
    }
}