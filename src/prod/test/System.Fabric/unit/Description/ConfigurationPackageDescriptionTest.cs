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
    public class ConfigurationPropertyTest
    {
        internal class Helper : DescriptionElementTestBase<ConfigurationProperty, ConfigurationParameterInfo>
        {
            public override ConfigurationProperty Factory(ConfigurationParameterInfo info)
            {
                return ConfigurationProperty.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override ConfigurationParameterInfo CreateDefaultInfo()
            {
                return new ConfigurationParameterInfo
                {
                    Name = "parameterName",
                    Value = "parameterValue",
                };
            }

            public override ConfigurationProperty CreateDefaultDescription()
            {
                return new ConfigurationProperty
                {
                    MustOverride = false,
                    Name = "parameterName",
                    Value = "parameterValue",
                };
            }

            public override void Compare(ConfigurationProperty expected, ConfigurationProperty actual)
            {
                Assert.AreEqual<bool>(expected.MustOverride, actual.MustOverride);
                Assert.AreEqual(expected.Name, actual.Name);
                Assert.AreEqual(expected.Value, actual.Value);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ConfigurationProperty_ParseTest()
        {
            HelperInstance.ParseTestHelper(null, null);
        }
    }

    [TestClass]
    public class ConfigurationSectionTest
    {
        internal class Helper : DescriptionElementTestBase<ConfigurationSection, ConfigurationSectionInfo>
        {

            public override ConfigurationSection Factory(ConfigurationSectionInfo info)
            {
                return ConfigurationSection.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override ConfigurationSectionInfo CreateDefaultInfo()
            {
                return new ConfigurationSectionInfo
                {
                    Parameters =
                    {
                        ConfigurationPropertyTest.HelperInstance.CreateDefaultInfo()
                    },
                    SectionName = "fooSection",
                };
            }

            public override ConfigurationSection CreateDefaultDescription()
            {
                return new ConfigurationSection
                {
                    Parameters =
                    {
                        ConfigurationPropertyTest.HelperInstance.CreateDefaultDescription()
                    },
                    Name = "fooSection"
                };
            }

            public override void Compare(ConfigurationSection expected, ConfigurationSection actual)
            {
                Assert.AreEqual(expected.Name, actual.Name);
                MiscUtility.CompareEnumerables(expected.Parameters, actual.Parameters, ConfigurationPropertyTest.HelperInstance.Compare);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("apramery")]
        public void ConfigurationSection_ParseBasic()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ConfigurationSection_EmptySectionIsParsed()
        {
            HelperInstance.ParseTestHelper((info) => info.Parameters.Clear(), (description) => description.Parameters.Clear());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ConfigurationSection_NullParameterListIsParsed()
        {
            HelperInstance.ParseTestHelper((info) =>
                {
                    info.Parameters.Clear();
                    info.Parameters.ReturnNullIfEmpty = true;
                },
                (description) => description.Parameters.Clear());
        }
    }

    [TestClass]
    public class ConfigurationTest
    {
        internal class Helper : DescriptionElementTestBase<System.Fabric.Description.ConfigurationSettings, ConfigurationSettingsInfo>
        {

            public override System.Fabric.Description.ConfigurationSettings Factory(ConfigurationSettingsInfo info)
            {
                return System.Fabric.Description.ConfigurationSettings.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override ConfigurationSettingsInfo CreateDefaultInfo()
            {
                return new ConfigurationSettingsInfo
                {
                    Sections =
                    {
                        ConfigurationSectionTest.HelperInstance.CreateDefaultInfo()
                    }
                };
            }

            public override System.Fabric.Description.ConfigurationSettings CreateDefaultDescription()
            {
                return new System.Fabric.Description.ConfigurationSettings
                {
                    Sections = 
                    {
                        ConfigurationSectionTest.HelperInstance.CreateDefaultDescription()
                    }
                };
            }

            public override void Compare(System.Fabric.Description.ConfigurationSettings expected, System.Fabric.Description.ConfigurationSettings actual)
            {
                MiscUtility.CompareEnumerables(expected.Sections, actual.Sections, ConfigurationSectionTest.HelperInstance.Compare);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void Configuration_Basic()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void Configuration_EmptyConfigIsParsed()
        {
            HelperInstance.ParseTestHelper((info) => info.Sections.Clear(), (description) => description.Sections.Clear());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void Configuration_NullConfigIsParsed()
        {
            HelperInstance.ParseTestHelper((info) =>
                {
                    info.Sections.Clear();
                    info.Sections.ReturnNullIfEmpty = true;
                },
                (description) => description.Sections.Clear());
        }
    }

    [TestClass]
    public class ConfigurationPackageTest
    {
        internal class Helper : DescriptionElementTestBase<ConfigurationPackage, ConfigurationPackageInfo>
        {

            public override ConfigurationPackage Factory(ConfigurationPackageInfo info)
            {
                return ConfigurationPackage.CreateFromNative((NativeRuntime.IFabricConfigurationPackage)info);
            }

            public override ConfigurationPackageInfo CreateDefaultInfo()
            {
                return new ConfigurationPackageInfo
                {
                    Name = "fooConfigPkg",
                    PathProperty = "path",
                    Version = "1.0",
                    ServiceManifestName = "DefaultManifest",
                    ServiceManifestVersion = "DefaultManifestVersion",
                    SettingsProperty = ConfigurationTest.HelperInstance.CreateDefaultInfo()
                };
            }

            public ConfigurationPackageInfo CreateDefaultInfo(string manifestName, string manifestVersion)
            {
                return new ConfigurationPackageInfo
                {
                    Name = "fooConfigPkg",
                    PathProperty = "path",
                    Version = "1.0",
                    ServiceManifestName = manifestName,
                    ServiceManifestVersion = manifestVersion,
                    SettingsProperty = ConfigurationTest.HelperInstance.CreateDefaultInfo()
                };
            }

            public override ConfigurationPackage CreateDefaultDescription()
            {
                return new ConfigurationPackage
                {
                    Description = new ConfigurationPackageDescription()
                    {
                        Name = "fooConfigPkg",
                        Version = "1.0",
                    },
                    Path = "path",
                    Settings = ConfigurationTest.HelperInstance.CreateDefaultDescription()
                };
            }

            public override void Compare(ConfigurationPackage expected, ConfigurationPackage actual)
            {
                PackageDescriptionTest.ComparePackageDescriptionInternal(expected.Description, actual.Description);
                Assert.AreEqual<string>(expected.Path, actual.Path);
                ConfigurationTest.HelperInstance.Compare(expected.Settings, actual.Settings);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ConfigurationPackageDescription_BasicParse()
        {
            HelperInstance.ParseTestHelper(null, null);
        }
    }
}