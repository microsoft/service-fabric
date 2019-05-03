// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<ServiceDescription, object>
        {
            public override ServiceDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override ServiceDescription CreateDefaultDescription()
            {
                throw new NotImplementedException();
            }

            public override void Compare(ServiceDescription expected, ServiceDescription actual)
            {
                TestUtility.Compare<ServiceDescription, StatefulServiceDescription, StatelessServiceDescription>(expected, actual, StatefulServiceDescriptionTest.HelperInstance.Compare, StatelessServiceDescriptionTest.HelperInstance.Compare);
            }

            public void CompareInternal(ServiceDescription expected, ServiceDescription actual)
            {
                Assert.AreEqual<Uri>(expected.ApplicationName, actual.ApplicationName);
                MiscUtility.CompareEnumerables(expected.Correlations, actual.Correlations, ServiceCorrelationDescriptionTest.HelperInstance.Compare);

                if (expected.InitializationData == null)
                {
                    Assert.AreEqual(0, actual.InitializationData.Length);
                }
                else
                {
                    MiscUtility.CompareEnumerables(expected.InitializationData, actual.InitializationData);
                }

                if (expected.Kind == ServiceDescriptionKind.Stateful)
                {
                    MiscUtility.CompareEnumerables(expected.Metrics as IEnumerable<StatefulServiceLoadMetricDescription>, actual.Metrics as IEnumerable<StatefulServiceLoadMetricDescription>, StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.Compare);
                }
                else
                {
                    MiscUtility.CompareEnumerables(expected.Metrics as IEnumerable<StatelessServiceLoadMetricDescription>, actual.Metrics as IEnumerable<StatelessServiceLoadMetricDescription>, StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.Compare);
                }

                ServicePartitionDescriptionTest.HelperInstance.Compare(expected.PartitionSchemeDescription, actual.PartitionSchemeDescription);
                Assert.AreEqual(expected.PlacementConstraints, actual.PlacementConstraints);
                Assert.AreEqual<Uri>(expected.ServiceName, actual.ServiceName);
                Assert.AreEqual(expected.ServiceTypeName, actual.ServiceTypeName);
            }
        }

        internal static Helper HelperInstance
        {
            get
            {
                return new Helper();
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceDescription_StatelessDescriptionIsParsed()
        {
            ServiceDescription expected = StatelessServiceDescriptionTest.HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                IntPtr native = expected.ToNative(pc);

                ServiceDescription actual = ServiceDescription.CreateFromNative(native);

                HelperInstance.Compare(expected, actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceDescription_StatefulDescriptionIsParsed()
        {
            ServiceDescription expected = StatefulServiceDescriptionTest.HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                IntPtr native = expected.ToNative(pc);

                ServiceDescription actual = ServiceDescription.CreateFromNative(native);

                HelperInstance.Compare(expected, actual);
            }
        }
    }

    [TestClass]
    public class StatefulServiceDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<StatefulServiceDescription, object>
        {

            public override StatefulServiceDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override StatefulServiceDescription CreateDefaultDescription()
            {
                return new StatefulServiceDescription
                {
                    ApplicationName = new Uri("fabric:/someapp"),
                    Correlations =
                    {
                        ServiceCorrelationDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    HasPersistedState = true,
                    InitializationData = new byte[] { 0, 1 },
                    Metrics =
                    {
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultDescription(),
                    },
                    MinReplicaSetSize = 3,
                    PartitionSchemeDescription = ServicePartitionDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    PlacementConstraints = "placement_constraints",
                    ServiceName = new Uri("fabric:/g"),
                    ServiceTypeName = "myServiceTypeName",
                    TargetReplicaSetSize = 1,
                };
            }

            public StatefulServiceDescription CreateDefaultMetricDescription()
            {
                return new StatefulServiceDescription
                {
                    ApplicationName = new Uri("fabric:/someapp"),
                    Correlations =
                    {
                        ServiceCorrelationDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    HasPersistedState = true,
                    InitializationData = new byte[] { 0, 1 },
                    Metrics =
                    {
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultMetrics(0),
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultMetrics(1),
                        StatefulServiceLoadMetricDescriptionTest.StatefulHelperInstance.CreateDefaultMetrics(2),
                    },
                    MinReplicaSetSize = 3,
                    PartitionSchemeDescription = ServicePartitionDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    PlacementConstraints = "placement_constraints",
                    ServiceName = new Uri("fabric:/g"),
                    ServiceTypeName = "myServiceTypeName",
                    TargetReplicaSetSize = 1,
                };
            }

            public override void Compare(StatefulServiceDescription expected, StatefulServiceDescription actual)
            {
                ServiceDescriptionTest.HelperInstance.CompareInternal(expected, actual);

                Assert.AreEqual(expected.HasPersistedState, actual.HasPersistedState);
                Assert.AreEqual(expected.TargetReplicaSetSize, actual.TargetReplicaSetSize);
                Assert.AreEqual(expected.MinReplicaSetSize, actual.MinReplicaSetSize);
            }
        }

        internal static Helper HelperInstance
        {
            get
            {
                return new Helper();
            }
        }

        private void TestHelper(Action<StatefulServiceDescription> setup)
        {
            var expected = HelperInstance.CreateDefaultDescription();

            if (setup != null)
            {
                setup(expected);
            }

            using (var pc = new PinCollection())
            {
                NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind;
                IntPtr native = expected.ToNative(pc, out kind);


                var actual = StatefulServiceDescription.CreateFromNative(native);

                Assert.AreEqual(NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, kind);
                HelperInstance.Compare(expected, actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatefulServiceDescription_Test()
        {
            this.TestHelper(null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatefulServiceDescription_EmptyCorrelation()
        {
            this.TestHelper((d) => d.Correlations.Clear());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatefulServiceDescription_EmptyMetrics()
        {
            this.TestHelper((d) => d =  StatefulServiceDescriptionTest.HelperInstance.CreateDefaultMetricDescription());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatefulServiceDescription_InitializationData()
        {
            this.TestHelper((d) => d.InitializationData = null);
            this.TestHelper((d) => d.InitializationData = new byte[0]);
        }
    }

    [TestClass]
    public class StatelessServiceDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<StatelessServiceDescription, object>
        {

            public override StatelessServiceDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override StatelessServiceDescription CreateDefaultDescription()
            {
                return new StatelessServiceDescription
                {
                    ApplicationName = new Uri("fabric:/someapp"),
                    Correlations =
                    {
                        ServiceCorrelationDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    InitializationData = new byte[] { 0, 1 },
                    InstanceCount = 41,
                    Metrics =
                    {
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateDefaultDescription(),
                    },
                    PartitionSchemeDescription = ServicePartitionDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    PlacementConstraints = "placement_constraints",
                    ServiceName = new Uri("fabric:/g"),
                    ServiceTypeName = "myServiceTypeName",
                };
            }

            public StatelessServiceDescription CreateDefaultMetricDescription()
            {
                return new StatelessServiceDescription
                {
                    ApplicationName = new Uri("fabric:/someapp"),
                    Correlations =
                    {
                        ServiceCorrelationDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    InitializationData = new byte[] { 0, 1 },
                    InstanceCount = 41,
                    Metrics =
                    {
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateDefaultMetrics(0),
                        StatelessServiceLoadMetricDescriptionTest.StatelessHelperInstance.CreateDefaultMetrics(1),
                    },
                    PartitionSchemeDescription = ServicePartitionDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    PlacementConstraints = "placement_constraints",
                    ServiceName = new Uri("fabric:/g"),
                    ServiceTypeName = "myServiceTypeName",
                };
            }


            public override void Compare(StatelessServiceDescription expected, StatelessServiceDescription actual)
            {
                ServiceDescriptionTest.HelperInstance.CompareInternal(expected, actual);
                Assert.AreEqual(expected.InstanceCount, actual.InstanceCount);
            }
        }

        internal static Helper HelperInstance
        {
            get
            {
                return new Helper();
            }
        }

        private void TestHelper(Action<StatelessServiceDescription> setup)
        {
            var expected = HelperInstance.CreateDefaultDescription();

            if (setup != null)
            {
                setup(expected);
            }

            using (var pc = new PinCollection())
            {
                NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind;
                IntPtr native = expected.ToNative(pc, out kind);


                var actual = StatelessServiceDescription.CreateFromNative(native);

                Assert.AreEqual(NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS, kind);
                HelperInstance.Compare(expected, actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatelessServiceDescription_Test()
        {
            this.TestHelper(null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatelessServiceDescription_EmptyCorrelation()
        {
            this.TestHelper((d) => d.Correlations.Clear());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatelessServiceDescription_EmptyMetrics()
        {
            this.TestHelper((d) => StatelessServiceDescriptionTest.HelperInstance.CreateDefaultMetricDescription());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StatelessServiceDescription_InitializationData()
        {
            this.TestHelper((d) => d.InitializationData = null);
            this.TestHelper((d) => d.InitializationData = new byte[0]);
        }
    }
}