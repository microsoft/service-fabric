// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;

    [VS.TestClass]
    public class CodePackageTest
    {
        internal class Helper : DescriptionElementTestBase<CodePackage, CodePackageInfo>
        {
            public override void Compare(CodePackage expected, CodePackage actual)
            {
                PackageDescriptionTest.ComparePackageDescriptionInternal(expected.Description, actual.Description);
                EntryPointDescriptionTest.HelperInstance.Compare(expected.Description.EntryPoint, actual.Description.EntryPoint);
                ExecutableEntryPointDescriptionTest.HelperInstance.Compare(expected.Description.SetupEntryPoint, actual.Description.SetupEntryPoint);
                Assert.AreEqual<bool>(expected.Description.IsShared, actual.Description.IsShared);
                Assert.AreEqual<string>(expected.Description.Name, actual.Description.Name);
                Assert.AreEqual<string>(expected.Description.Version, actual.Description.Version);

                Assert.AreEqual<string>(expected.Path, actual.Path);
            }

            public override CodePackage CreateDefaultDescription()
            {
                return new CodePackage()

                {
                    Description = new CodePackageDescription()
                    {
                        SetupEntryPoint = new ExeHostEntryPointDescription()
                        {
                            Program = "Setup.exe",
                            Arguments = null,
                            WorkingFolder = ExeHostWorkingFolder.Work
                        },
                        EntryPoint = ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultDescription(),
                        IsShared = true,
                        Name = "fooCodePkg",
                        Version = "1.1"
                    },
                    Path = "xyz"
                };
            }

            public override CodePackageInfo CreateDefaultInfo()
            {
                return new CodePackageInfo
                {
                    SetupEntryPoint = new ExecutableEntryPointInfo()
                    {
                        Program = "Setup.exe",
                        Arguments = null,
                        WorkingFolder = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_WORK
                    },
                    EntryPoint = new EntryPointInfo
                    {
                        Description = ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultInfo()
                    },
                    IsShared = true,
                    Name = "fooCodePkg",
                    Version = "1.1",
                    ServiceManifestName = "DefaultManifest",
                    ServiceManifestVersion = "DefaultManifestVersion",
                    PathProperty = "xyz"
                };
            }

            public CodePackageInfo CreateDefaultInfo(string manifestName, string manifestVersion)
            {
                return new CodePackageInfo
                {
                    SetupEntryPoint = new ExecutableEntryPointInfo()
                    {
                        Program = "Setup.exe",
                        Arguments = null,
                        WorkingFolder = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_WORK
                    },
                    EntryPoint = new EntryPointInfo
                    {
                        Description = ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultInfo()
                    },
                    IsShared = true,
                    Name = "fooCodePkg",
                    PathProperty = "xyz",
                    Version = "1.1",
                    ServiceManifestName = manifestName,
                    ServiceManifestVersion = manifestVersion
                };
            }

            public override CodePackage Factory(CodePackageInfo info)
            {
                return CodePackage.CreateFromNative((NativeRuntime.IFabricCodePackage)info);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageDescription_BasicPropertiesAreParsed()
        {
            // tests the name, path, version, periodic interval in seconds, entrypoint etc
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CodePackageDescription_EmptyEntryPointFails()
        {
            HelperInstance.ParseErrorTestHelper<ArgumentException>((info) => info.EntryPoint = null);
        }
    }
}