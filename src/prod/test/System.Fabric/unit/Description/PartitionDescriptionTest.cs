// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class NamedPartitionDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<NamedPartitionSchemeDescription, object>
        {

            public override void Compare(NamedPartitionSchemeDescription expected, NamedPartitionSchemeDescription actual)
            {
                MiscUtility.CompareEnumerables(expected.PartitionNames, actual.PartitionNames);
            }

            public override NamedPartitionSchemeDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override NamedPartitionSchemeDescription CreateDefaultDescription()
            {
                return new NamedPartitionSchemeDescription
                {
                    PartitionNames = 
                    {
                        "abc",
                        "def"
                    }
                };
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
        public void NamedPartitionDescription_Test()
        {
            NamedPartitionSchemeDescription expected = HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                NamedPartitionSchemeDescription actual = NamedPartitionSchemeDescription.CreateFromNative(expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }
    }

    [TestClass]
    public class UniformInt64ServicePartitionDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<UniformInt64RangePartitionSchemeDescription, object>
        {
            public override void Compare(UniformInt64RangePartitionSchemeDescription expected, UniformInt64RangePartitionSchemeDescription actual)
            {
                Assert.AreEqual<int>(expected.PartitionCount, actual.PartitionCount);
                Assert.AreEqual<long>(expected.HighKey, actual.HighKey);
                Assert.AreEqual<long>(expected.LowKey, actual.LowKey);
            }

            public override UniformInt64RangePartitionSchemeDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override UniformInt64RangePartitionSchemeDescription CreateDefaultDescription()
            {
                return new UniformInt64RangePartitionSchemeDescription
                {
                    HighKey = 4,
                    LowKey = 2,
                    PartitionCount = 1,
                };
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
        public void UniformInt64ServicePartitionDescriptionTest_Test()
        {
            var expected = HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                var actual = UniformInt64RangePartitionSchemeDescription.CreateFromNative(expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }
    }

    [TestClass]
    public class SingletonPartitionDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<SingletonPartitionSchemeDescription, object>
        {

            public override SingletonPartitionSchemeDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override SingletonPartitionSchemeDescription CreateDefaultDescription()
            {
                return new SingletonPartitionSchemeDescription();
            }

            public override void Compare(SingletonPartitionSchemeDescription expected, SingletonPartitionSchemeDescription actual)
            {

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
        public void Singleton_Test()
        {
            var expected = HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                var actual = SingletonPartitionSchemeDescription.CreateFromNative(expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }
    }

    [TestClass]
    public class ServicePartitionDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<PartitionSchemeDescription, object>
        {
            public override void Compare(PartitionSchemeDescription expected, PartitionSchemeDescription actual)
            {
                TestUtility.Compare<PartitionSchemeDescription, SingletonPartitionSchemeDescription, UniformInt64RangePartitionSchemeDescription, NamedPartitionSchemeDescription>(expected, actual,
                    SingletonPartitionDescriptionTest.HelperInstance.Compare,
                    UniformInt64ServicePartitionDescriptionTest.HelperInstance.Compare,
                    NamedPartitionDescriptionTest.HelperInstance.Compare);
            }

            public override PartitionSchemeDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override PartitionSchemeDescription CreateDefaultDescription()
            {
                return NamedPartitionDescriptionTest.HelperInstance.CreateDefaultDescription();
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
        public void PartitionDescription_NamedPartitionDescriptionIsParsed()
        {
            var expected = NamedPartitionDescriptionTest.HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                var actual = PartitionSchemeDescription.CreateFromNative((NativeTypes.FABRIC_PARTITION_SCHEME)expected.Scheme, expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PartitionDescription_Int64PartitionDescriptionIsParsed()
        {
            var expected = UniformInt64ServicePartitionDescriptionTest.HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                var actual = PartitionSchemeDescription.CreateFromNative((NativeTypes.FABRIC_PARTITION_SCHEME)expected.Scheme, expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PartitionDescription_SingletonPartitionDescriptionIsParsed()
        {
            var expected = SingletonPartitionDescriptionTest.HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                var actual = PartitionSchemeDescription.CreateFromNative((NativeTypes.FABRIC_PARTITION_SCHEME)expected.Scheme, expected.ToNative(pc));
                HelperInstance.Compare(expected, actual);
            }
        }
    }
}