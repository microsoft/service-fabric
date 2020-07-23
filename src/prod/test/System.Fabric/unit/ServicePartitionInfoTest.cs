// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Threading.Tasks;
    using System.Fabric;
    using System.Fabric.Interop;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServicePartitionInfoTest
    {
        private static ServicePartitionInformation ServicePartitionParsingTestHelper(NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION nativePartitionInfo)
        {
            return ServicePartitionInformation.FromNative(nativePartitionInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_PartitionInfo_SingletonPartitionsAreParsed()
        {
            NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo = new NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION
            {
                Id = Guid.NewGuid()
            };

            var pin = new PinCollection();
            pin.AddBlittable(singletonPartitionInfo);

            var p = new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
            {
                Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON,
                Value = pin.AddrOfPinnedObject()
            };

            var actual = ServicePartitionInfoTest.ServicePartitionParsingTestHelper(p);
            Assert.AreEqual<Guid>(singletonPartitionInfo.Id, actual.Id);
            Assert.AreEqual(actual.Kind, ServicePartitionKind.Singleton);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_PartitionInfo_Int64PartitionsAreParsed()
        {
            unsafe
            {
                long highKeyExpected = 2, lowKeyExpected = 3;
                NativeTypes.FABRIC_INT64_RANGE_PARTITION_INFORMATION rangePartitionInfo = new NativeTypes.FABRIC_INT64_RANGE_PARTITION_INFORMATION
                {
                    Id = Guid.NewGuid(),
                    LowKey = lowKeyExpected,
                    HighKey = highKeyExpected
                };

                var pin = new PinCollection();
                pin.AddBlittable(rangePartitionInfo);

                var p = new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
                {
                    Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE,
                    Value = pin.AddrOfPinnedObject()
                };

                var actual = ServicePartitionInfoTest.ServicePartitionParsingTestHelper(p);
                Assert.AreEqual<Guid>(rangePartitionInfo.Id, actual.Id);
                Assert.AreEqual(actual.Kind, ServicePartitionKind.Int64Range);
                Assert.AreEqual<long>(highKeyExpected, ((Int64RangePartitionInformation)actual).HighKey);
                Assert.AreEqual<long>(lowKeyExpected, ((Int64RangePartitionInformation)actual).LowKey);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_PartitionInfo_NamedPartitionsAreParsed()
        {
            string partitionNameExpected = "abc";

            unsafe
            {
                var pin = new PinCollection();
                pin.AddBlittable(partitionNameExpected);

                NativeTypes.FABRIC_NAMED_PARTITION_INFORMATION namedPartitionInfo = new NativeTypes.FABRIC_NAMED_PARTITION_INFORMATION
                {
                    Id = Guid.NewGuid(),
                    Name = pin.AddrOfPinnedObject()
                };

                var p = new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
                {
                    Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_NAMED,
                    Value = pin.AddBlittable(namedPartitionInfo)
                };

                var actual = ServicePartitionInfoTest.ServicePartitionParsingTestHelper(p);
                Assert.AreEqual<Guid>(namedPartitionInfo.Id, actual.Id);
                Assert.AreEqual(actual.Kind, ServicePartitionKind.Named);
                Assert.AreEqual<string>(partitionNameExpected, ((NamedPartitionInformation)actual).Name);
            }
        }
    }
}