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
    public class DataPackageTest
    {
        internal class Helper : DescriptionElementTestBase<DataPackage, DataPackageInfo>
        {
            public override DataPackage Factory(DataPackageInfo info)
            {
                return DataPackage.CreateFromNative((NativeRuntime.IFabricDataPackage)info);
            }

            public override DataPackageInfo CreateDefaultInfo()
            {
                return new DataPackageInfo
                {
                    Name = "datapkg",
                    PathProperty = "datapkgpath",
                    Version = "23",
                    ServiceManifestName = "DefaultManifest",
                    ServiceManifestVersion = "DefaultManifestVersion"
                };
            }

            public DataPackageInfo CreateDefaultInfo(string manifestName, string manifestVersion)
            {
                return new DataPackageInfo
                {
                    Name = "datapkg",
                    PathProperty = "datapkgpath",
                    Version = "23",
                    ServiceManifestName = manifestName,
                    ServiceManifestVersion = manifestVersion
                };
            }

            public override DataPackage CreateDefaultDescription()
            {
                return new DataPackage()
                {
                    Description = new DataPackageDescription
                        {
                            Name = "datapkg",
                            Version = "23"
                        },
                    Path = "datapkgpath"
                };
            }

            public override void Compare(DataPackage expected, DataPackage actual)
            {
                PackageDescriptionTest.ComparePackageDescriptionInternal(expected.Description, actual.Description);
                Assert.AreEqual<string>(expected.Path, actual.Path);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void DataPackageDescription_ParseBasic()
        {
            HelperInstance.ParseTestHelper(null, null);
        }
    }
}