// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;


    public class ServiceTypeDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<ServiceTypeDescription, ServiceTypeInfo>
        {
            public override ServiceTypeDescription Factory(ServiceTypeInfo info)
            {
                return ServiceTypeDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override ServiceTypeInfo CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override ServiceTypeDescription CreateDefaultDescription()
            {
                throw new NotImplementedException();
            }

            public override void Compare(ServiceTypeDescription expected, ServiceTypeDescription actual)
            {
                TestUtility.Compare<ServiceTypeDescription, StatefulServiceTypeDescription, StatelessServiceTypeDescription>(expected, actual, StatefulServiceTypeDescriptionTest.HelperInstance.Compare, StatelessServiceTypeDescriptionTest.HelperInstance.Compare);
            }

            public void CompareCommonProperties(ServiceTypeDescription expected, ServiceTypeDescription actual)
            {
                Assert.AreEqual(expected.ServiceTypeName, actual.ServiceTypeName);
                Assert.AreEqual(expected.PlacementConstraints, actual.PlacementConstraints);

                if (expected.ServiceTypeKind == ServiceDescriptionKind.Stateful)
                {
                    MiscUtility.CompareEnumerables(expected.LoadMetrics as IEnumerable<StatefulServiceLoadMetricDescription>, actual.LoadMetrics as IEnumerable<StatefulServiceLoadMetricDescription>, StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.Compare);
                }
                else
                {
                    MiscUtility.CompareEnumerables(expected.LoadMetrics as IEnumerable<StatelessServiceLoadMetricDescription>, actual.LoadMetrics as IEnumerable<StatelessServiceLoadMetricDescription>, StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.Compare);
                }

                MiscUtility.CompareEnumerables(expected.Extensions.Keys, actual.Extensions.Keys);
                foreach (string key in expected.Extensions.Keys)
                {
                    Assert.AreEqual(expected.Extensions[key], actual.Extensions[key]);
                }
            }
        }

        internal static Helper HelperInstance
        {
            get
            {
                return new Helper();
            }
        }


        internal static void NullCollectionTestHelper<TDescription, TInfo>(DescriptionElementTestBase<TDescription, TInfo> helper)
            where TDescription : ServiceTypeDescription
            where TInfo : ServiceTypeInfo
        {
            helper.ParseTestHelper(
                (info) =>
                {
                    info.Extensions.Clear();
                    info.Extensions.ReturnNullIfEmpty = true;

                    info.LoadMetrics.Clear();
                    info.LoadMetrics.ReturnNullIfEmpty = true;
                },
                (description) =>
                {
                    description.Extensions.Clear();
                    description.LoadMetrics.Clear();
                });
        }
    }

    [TestClass]
    public class StatelessServiceTypeDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<StatelessServiceTypeDescription, StatelessServiceTypeInfo>
        {

            public override StatelessServiceTypeDescription Factory(StatelessServiceTypeInfo info)
            {
                return StatelessServiceTypeDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNativeDerived()));
            }

            public override StatelessServiceTypeInfo CreateDefaultInfo()
            {
                return new StatelessServiceTypeInfo
                {
                    Extensions =
                    {
                        DescriptionExtensionTest.HelperInstance.CreateDefaultInfo(),
                    },
                    LoadMetrics = 
                    {
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateDefaultInfo(),
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateIsolatedServiceLoadMetricInfo()
                    },
                    PlacementConstraints = "fooplacementcon",
                    ServiceTypeName = "service_type_name_stateless"
                };
            }

            public override StatelessServiceTypeDescription CreateDefaultDescription()
            {
                var d = new StatelessServiceTypeDescription
                {
                    ServiceTypeName = "service_type_name_stateless",
                    PlacementConstraints = "fooplacementcon",
                    LoadMetrics =
                    {
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateDefaultDescription(),
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateIsolatedServiceLoadMetricDescription()
                    }
                };

                var def = DescriptionExtensionTest.HelperInstance.CreateDefaultDescription();
                d.Extensions.Add(def.Item1, def.Item2);

                return d;
            }

            public override void Compare(StatelessServiceTypeDescription expected, StatelessServiceTypeDescription actual)
            {
                ServiceTypeDescriptionTest.HelperInstance.CompareCommonProperties(expected, actual);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatelessServiceTypeDescription_BasicParse()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatelessServiceTypeDescription_NullCollectionIsParsed()
        {
            ServiceTypeDescriptionTest.NullCollectionTestHelper(HelperInstance);
        }
    }

    [TestClass]
    public class StatefulServiceTypeDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<StatefulServiceTypeDescription, StatefulServiceTypeInfo>
        {

            public override StatefulServiceTypeDescription Factory(StatefulServiceTypeInfo info)
            {
                return StatefulServiceTypeDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNativeDerived()));
            }

            public override StatefulServiceTypeInfo CreateDefaultInfo()
            {
                return new StatefulServiceTypeInfo
                {
                    Extensions =
                    {
                        DescriptionExtensionTest.HelperInstance.CreateDefaultInfo(),
                    },
                    LoadMetrics =
                    {
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultInfo(),
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateIsolatedServiceLoadMetricInfo()
                    },
                    HasPersistedState = true,
                    PlacementConstraints = "stateful_placement_constarints",
                    ServiceTypeName = "statefulService"
                };
            }

            public override StatefulServiceTypeDescription CreateDefaultDescription()
            {
                var d = new StatefulServiceTypeDescription
                {
                    ServiceTypeName = "statefulService",
                    PlacementConstraints = "stateful_placement_constarints",
                    LoadMetrics =
                    {
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultDescription(),
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateIsolatedServiceLoadMetricDescription()
                    },
                    HasPersistedState = true,
                };

                var def = DescriptionExtensionTest.HelperInstance.CreateDefaultDescription();
                d.Extensions.Add(def.Item1, def.Item2);

                return d;
            }

            public override void Compare(StatefulServiceTypeDescription expected, StatefulServiceTypeDescription actual)
            {
                ServiceTypeDescriptionTest.HelperInstance.CompareCommonProperties(expected, actual);
                Assert.AreEqual<bool>(expected.HasPersistedState, actual.HasPersistedState);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulServiceTypeDescription_BasicParse()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulServiceTypeDescription_NullCollectionIsParsed()
        {
            ServiceTypeDescriptionTest.NullCollectionTestHelper(HelperInstance);
        }
    }
}