// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceGroupDescriptionTest
    {

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("anulb")]
        public void TestServiceGroupDescription()
        {
            RunServiceGroupDescriptionTest(memberCount: 0, metricCount: 0, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 0, metricCount: 0, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 0, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 0, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 0, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 0, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 1, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 1, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 1, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 1, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 100, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 100, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 100, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 100, metricCount: 100, isStateful: true);

            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 100, isStateful: false);
            RunServiceGroupDescriptionTest(memberCount: 1, metricCount: 100, isStateful: true);
        }

        private void RunServiceGroupDescriptionTest(int memberCount, int metricCount, bool isStateful)
        {
            var description = CreateServiceGroupDescription(
                memberCount: memberCount,
                metricCount: metricCount,
                isStateful: isStateful);

            using (var pin = new PinCollection())
            {
                IntPtr nativePtr = InteropHelpers.ServiceGroupDescriptionHelper.ToNative(pin, description);

                var marshalled = ServiceGroupDescription.CreateFromNative(new ServiceGroupDescriptionResult(nativePtr));

                ValidateServiceGroupDescription(description, marshalled);
            }
        }

        public static void ValidateServiceGroupDescription(ServiceGroupDescription original, ServiceGroupDescription marshalled)
        {
            Assert.IsNotNull(marshalled);
            Assert.IsNotNull(marshalled.ServiceDescription);

            Assert.AreEqual(original.ServiceDescription.GetType(), marshalled.ServiceDescription.GetType());

            Assert.AreEqual(original.ServiceDescription.ApplicationName, marshalled.ServiceDescription.ApplicationName);
            Assert.AreEqual(original.ServiceDescription.ServiceName, marshalled.ServiceDescription.ServiceName);
            Assert.AreEqual(original.ServiceDescription.ServiceTypeName, marshalled.ServiceDescription.ServiceTypeName);

            Assert.AreEqual(original.MemberDescriptions.Count, marshalled.MemberDescriptions.Count);

            foreach (var originalMember in original.MemberDescriptions)
            {
                var marshalledMember = marshalled.MemberDescriptions.Where(m => m.ServiceName == originalMember.ServiceName).FirstOrDefault();
                Assert.IsNotNull(marshalledMember);

                Assert.AreEqual(originalMember.ServiceTypeName, marshalledMember.ServiceTypeName);

                if (originalMember.InitializationData == null)
                {
                    Assert.IsTrue(marshalledMember.InitializationData == null || marshalledMember.InitializationData.Length == 0);
                }
                else
                {
                    Assert.IsNotNull(marshalledMember.InitializationData);
                    Assert.AreEqual(originalMember.InitializationData.Length, marshalledMember.InitializationData.Length);
                }

                Assert.AreEqual(originalMember.Metrics.Count, marshalledMember.Metrics.Count);

                foreach (var originalMetric in originalMember.Metrics)
                {
                    var marshalledMetric = marshalledMember.Metrics.Where(m => m.Name == originalMetric.Name).FirstOrDefault();
                    Assert.IsNotNull(marshalledMetric);

                    if (original.ServiceDescription.Kind == ServiceDescriptionKind.Stateful)
                    {
                        Assert.AreEqual((originalMetric as StatefulServiceLoadMetricDescription).PrimaryDefaultLoad, (marshalledMetric as StatefulServiceLoadMetricDescription).PrimaryDefaultLoad);
                        Assert.AreEqual((originalMetric as StatefulServiceLoadMetricDescription).SecondaryDefaultLoad, (marshalledMetric as StatefulServiceLoadMetricDescription).SecondaryDefaultLoad);
                    }
                    else if (original.ServiceDescription.Kind == ServiceDescriptionKind.Stateless)
                    {
                        Assert.AreEqual((originalMetric as StatelessServiceLoadMetricDescription).DefaultLoad, (marshalledMetric as StatelessServiceLoadMetricDescription).DefaultLoad);
                    }

                        Assert.AreEqual(originalMetric.Weight, marshalledMetric.Weight);
                }
            }
        }

        public static ServiceGroupDescription CreateServiceGroupDescription(int memberCount, int metricCount, bool isStateful)
        {
            ServiceGroupDescription description = new ServiceGroupDescription
            {
                ServiceDescription = CreateServiceDescription(isStateful)
            };

            for (int i = 0; i < memberCount; i++)
            {
                description.MemberDescriptions.Add(CreateServiceGroupMemberDescription(description.ServiceDescription.ServiceName, i, metricCount, isStateful));
            }

            return description;
        }

        private static ServiceGroupMemberDescription CreateServiceGroupMemberDescription(Uri serviceGroupName, int index, int metricCount, bool isStateful)
        {
            ServiceGroupMemberDescription description = new ServiceGroupMemberDescription
            {
                ServiceName = new Uri(serviceGroupName.ToString() + index),
                ServiceTypeName = "MemberType" + index,
                InitializationData = index == 0 ? null : new byte[10 * index]
            };

            for (int i = 0; i < metricCount; i++)
            {
                description.Metrics.Add(CreateServiceLoadMetricDescription(i, isStateful));
            }

            return description;
        }

        private static ServiceLoadMetricDescription CreateServiceLoadMetricDescription(int index, bool isStateful)
        {
            if (isStateful)
            {
                return new StatefulServiceLoadMetricDescription
                {
                    Name = "Metric" + index,
                    PrimaryDefaultLoad = (int)(Math.E * index),
                    SecondaryDefaultLoad = (int)(Math.E * index),
                    Weight = (ServiceLoadMetricWeight)(index % (int)ServiceLoadMetricWeight.High),
                };
            }
            else
            {
                return new StatelessServiceLoadMetricDescription
                {
                    Name = "Metric" + index,
                    DefaultLoad = (int)(Math.E * index),
                    Weight = (ServiceLoadMetricWeight)(index % (int)ServiceLoadMetricWeight.High),
                };
            }
            
        }

        private static ServiceDescription CreateServiceDescription(bool isStateful)
        {
            if (isStateful)
            {
                return new StatefulServiceDescription
                {
                    ApplicationName = new Uri("fabric:/app"),
                    ServiceName = new Uri("fabric:/app/sfsg"),
                    ServiceTypeName = "SFSG",
                    PartitionSchemeDescription = new SingletonPartitionSchemeDescription()
                };
            }
            else
            {
                return new StatelessServiceDescription
                {
                    ApplicationName = new Uri("fabric:/app"),
                    ServiceName = new Uri("fabric:/app/slsg"),
                    ServiceTypeName = "SLSG",
                    PartitionSchemeDescription = new SingletonPartitionSchemeDescription()
                };
            }
        }

        private class ServiceGroupDescriptionResult : NativeClient.IFabricServiceGroupDescriptionResult
        {
            private readonly IntPtr ptr;

            public ServiceGroupDescriptionResult(IntPtr ptr)
            {
                this.ptr = ptr;
            }

            public IntPtr get_Description()
            {
                return this.ptr;
            }
        }
    }
}