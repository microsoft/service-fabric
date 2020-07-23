// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class StatelessPartitionTest
    {
        private static readonly ServiceInitializationParameters DefaultInitializationParameters = new StatelessServiceInitializationParameters
        {
            ServiceTypeName = "foo",
            ServiceName = new Uri("http://foo"),
        };

        private const long DefaultInstanceId = 123;

        private static PinCollection pinCollection;

        private static readonly NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION DefaultPartitionInfo;

        static StatelessPartitionTest()
        {
            StatelessPartitionTest.pinCollection = new PinCollection();
            StatelessPartitionTest.pinCollection.AddBlittable(
                new NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION
                {
                    Id = Guid.NewGuid()
                }
            );

            StatelessPartitionTest.DefaultPartitionInfo = new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
            {
                Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON,
                Value = StatelessPartitionTest.pinCollection.AddrOfPinnedObject()
            };
        }


        private Tuple<IStatelessServicePartition, StatelessNativePartitionStubForStatelessPartitionCreation> Create()
        {
            var nativePartitionStub = new StatelessNativePartitionStubForStatelessPartitionCreation
            {
                NativePartitionInfo = StatelessPartitionTest.DefaultPartitionInfo
            };

            var broker = new StatelessServiceBroker(new Stubs.StatelessServiceStubBase(), StatelessPartitionTest.DefaultInitializationParameters, StatelessPartitionTest.DefaultInstanceId);

            var partition = new StatelessServicePartition(broker, nativePartitionStub);

            return Tuple.Create((IStatelessServicePartition)partition, nativePartitionStub);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatelessPartition_Creation_PartitionInfoIsParsed()
        {
            var data = this.Create();

            var stub = data.Item2;
            var partition = data.Item1;

            var expected = ServicePartitionInformation.FromNative(StatelessPartitionTest.DefaultPartitionInfo);
            var actual = ((StatelessServicePartition)partition).PartitionInfo;

            Assert.AreEqual<Guid>(expected.Id, actual.Id);
            Assert.AreEqual(expected.GetType(), actual.GetType());
            Assert.AreEqual<Guid>(expected.Id, (Guid)partition.PartitionInfo.Id);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatelessPartition_ReportFault()
        {
            var data = this.Create();
            var stub = data.Item2;
            var partition = data.Item1;

            NativeTypes.FABRIC_FAULT_TYPE expected = NativeTypes.FABRIC_FAULT_TYPE.FABRIC_FAULT_TYPE_PERMANENT;

            partition.ReportFault(FaultType.Permanent);

            Assert.AreEqual<int>(1, stub.FaultType.Count);
            Assert.AreEqual<NativeTypes.FABRIC_FAULT_TYPE>(expected, stub.FaultType[0]);
        }

        class StatelessNativePartitionStubForStatelessPartitionCreation : Stubs.StatelessPartitionStubBase
        {
            public StatelessNativePartitionStubForStatelessPartitionCreation()
            {
                this.FaultType = new List<NativeTypes.FABRIC_FAULT_TYPE>();
            }

            public NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION NativePartitionInfo { get; set; }

            public override IntPtr GetPartitionInfo()
            {
                GCHandle handle = GCHandle.Alloc(this.NativePartitionInfo, GCHandleType.Pinned);
                return handle.AddrOfPinnedObject();
            }

            public IList<NativeTypes.FABRIC_FAULT_TYPE> FaultType { get; private set; }

            public override void ReportFault(NativeTypes.FABRIC_FAULT_TYPE faultType)
            {
                this.FaultType.Add(faultType);
            }
        }
    }
}