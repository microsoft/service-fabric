// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceManifestTest
    {
        internal class Helper : DescriptionElementTestBase<ServiceManifest, ServiceManifestInfo>
        {

            public override ServiceManifest Factory(ServiceManifestInfo info)
            {
                return ServiceManifest.CreateFromNative(new CodePackageActivationContextStub() { ServiceManifestInfo_Internal = info });
            }

            public override ServiceManifestInfo CreateDefaultInfo()
            {
                string manifestName = "service_manifest";
                string manifestVersion = "1.1";
                return new ServiceManifestInfo
                {
                    CodePackages =
                    {
                        CodePackageTest.HelperInstance.CreateDefaultInfo(manifestName, manifestVersion)
                    },
                    ConfigurationPackages =
                    {
                        ConfigurationPackageTest.HelperInstance.CreateDefaultInfo(manifestName, manifestVersion),
                    },
                    DataPackages =
                    {
                        DataPackageTest.HelperInstance.CreateDefaultInfo(manifestName, manifestVersion),
                    },
                    Endpoints =
                    {
                        EndpointDescriptionTest.HelperInstance.CreateDefaultInfo(),
                    },
                    Name = manifestName,
                    ServiceTypes =
                    {
                        StatefulServiceTypeDescriptionTest.HelperInstance.CreateDefaultInfo(),
                        StatelessServiceTypeDescriptionTest.HelperInstance.CreateDefaultInfo(),
                    },
                    Version = manifestVersion
                };
            }

            public override ServiceManifest CreateDefaultDescription()
            {
                return new ServiceManifest
                {
                    CodePackages =
                    {
                        CodePackageTest.HelperInstance.CreateDefaultDescription()
                    },
                    ConfigurationPackages =
                    {
                        ConfigurationPackageTest.HelperInstance.CreateDefaultDescription(),
                    },
                    DataPackages =
                    {
                        DataPackageTest.HelperInstance.CreateDefaultDescription(),
                    },
                    Endpoints =
                    {
                        EndpointDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    Name = "service_manifest",
                    ServiceTypeDescriptions =
                    {
                        StatefulServiceTypeDescriptionTest.HelperInstance.CreateDefaultDescription(),
                        StatelessServiceTypeDescriptionTest.HelperInstance.CreateDefaultDescription(),
                    },
                    Version = "1.1"
                };
            }

            public override void Compare(ServiceManifest expected, ServiceManifest actual)
            {
                Assert.AreEqual(expected.Name, actual.Name);
                Assert.AreEqual(expected.Version, actual.Version);
                MiscUtility.CompareEnumerables(expected.CodePackages, actual.CodePackages, CodePackageTest.HelperInstance.Compare);
                MiscUtility.CompareEnumerables(expected.DataPackages, actual.DataPackages, DataPackageTest.HelperInstance.Compare);
                MiscUtility.CompareEnumerables(expected.ConfigurationPackages, actual.ConfigurationPackages, ConfigurationPackageTest.HelperInstance.Compare);
                MiscUtility.CompareEnumerables(expected.ServiceTypeDescriptions, actual.ServiceTypeDescriptions, ServiceTypeDescriptionTest.HelperInstance.Compare);
                MiscUtility.CompareEnumerables(expected.Endpoints, actual.Endpoints, EndpointDescriptionTest.HelperInstance.Compare);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceManifest_ParseBasic()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceManifest_ParseEmptyCollections()
        {
            HelperInstance.ParseTestHelper((info) =>
                {
                    var nativeCollections = new INativeCollectionWrapper[] 
                    {
                        info.ConfigurationPackages,
                        info.DataPackages,
                        info.Endpoints,
                        info.ServiceTypes
                    };

                    foreach (var item in nativeCollections)
                    {
                        item.Clear();
                        item.ReturnNullIfEmpty = true;
                    }
                },
                (manifest) =>
                {
                    manifest.ConfigurationPackages.Clear();
                    manifest.DataPackages.Clear();
                    manifest.Endpoints.Clear();
                    manifest.ServiceTypeDescriptions.Clear();
                });

        }
    }

}