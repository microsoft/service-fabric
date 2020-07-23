// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;


    [TestClass]
    public class DescriptionExtensionTest
    {
        internal class Helper : DescriptionElementTestBase<Tuple<string, string>, DescriptionExtensionInfo>
        {
            public override Tuple<string, string> Factory(DescriptionExtensionInfo info)
            {
                return DescriptionExtension.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override DescriptionExtensionInfo CreateDefaultInfo()
            {
                return new DescriptionExtensionInfo
                {
                    Name = "DescriptionExtName",
                    Value = "abc",
                };
            }

            public override Tuple<string, string> CreateDefaultDescription()
            {
                return Tuple.Create("DescriptionExtName", "abc");
            }

            public override void Compare(Tuple<string, string> expected, Tuple<string, string> actual)
            {
                Assert.AreEqual(expected.Item1, actual.Item1);
                Assert.AreEqual(expected.Item2, actual.Item2);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DescriptionExtension_BasicParse()
        {
            HelperInstance.ParseTestHelper(null, null);
        }
    }
}