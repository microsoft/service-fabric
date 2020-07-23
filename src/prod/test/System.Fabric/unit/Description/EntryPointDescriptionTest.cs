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

    [TestClass]
    public class DllDescriptionTest
    {
        internal class ManagedDllHelper : DescriptionElementTestBase<DllHostHostedManagedDllDescription, DllHostItemInfo>
        {
            public override void Compare(DllHostHostedManagedDllDescription expected, DllHostHostedManagedDllDescription actual)
            {
                Assert.AreEqual<DllHostHostedDllKind>(expected.Kind, actual.Kind);
                Assert.AreEqual<string>(expected.AssemblyName, expected.AssemblyName);
            }

            public override DllHostHostedManagedDllDescription CreateDefaultDescription()
            {
                return new DllHostHostedManagedDllDescription
                {
                    AssemblyName = "foo"
                };
            }

            public override DllHostItemInfo CreateDefaultInfo()
            {
                return new DllHostItemInfo
                {
                    Item = "foo",
                    Type = Fabric.Interop.NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED
                };
            }

            public override DllHostHostedManagedDllDescription Factory(DllHostItemInfo info)
            {
                return DllHostHostedDllDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative())) as DllHostHostedManagedDllDescription;
            }
        }

        internal class UnmanagedDllHelper : DescriptionElementTestBase<DllHostHostedUnmanagedDllDescription, DllHostItemInfo>
        {
            public override void Compare(DllHostHostedUnmanagedDllDescription expected, DllHostHostedUnmanagedDllDescription actual)
            {
                Assert.AreEqual<DllHostHostedDllKind>(expected.Kind, actual.Kind);
                Assert.AreEqual<string>(expected.DllName, expected.DllName);
            }

            public override DllHostHostedUnmanagedDllDescription CreateDefaultDescription()
            {
                return new DllHostHostedUnmanagedDllDescription
                {
                    DllName = "foo"
                };
            }

            public override DllHostItemInfo CreateDefaultInfo()
            {
                return new DllHostItemInfo
                {
                    Item = "foo",
                    Type = Fabric.Interop.NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED
                };
            }

            public override DllHostHostedUnmanagedDllDescription Factory(DllHostItemInfo info)
            {
                return DllHostHostedDllDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative())) as DllHostHostedUnmanagedDllDescription;
            }
        }

        internal static ManagedDllHelper ManagedDllHelperInstance
        {
            get
            {
                return new ManagedDllHelper();
            }
        }

        internal static UnmanagedDllHelper UnmanagedDllHelperInstance
        {
            get
            {
                return new UnmanagedDllHelper();
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void AssemblyDescription_ManagedDllIsParsed()
        {
            ManagedDllHelperInstance.ParseTestHelper(
                (info) => info.Item = "x",
                (description) => description.AssemblyName = "x");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void AssemblyDescription_UnmanagedDllIsParsed()
        {
            UnmanagedDllHelperInstance.ParseTestHelper(
                (info) => info.Item = "x",
                (description) => description.DllName = "x");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void AssemblyDescription_InvalidAssemblyTypeThrows()
        {
            ManagedDllHelperInstance.ParseErrorTestHelper<ArgumentException>((info) => info.Type = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_INVALID);
        }


    }

    [TestClass]
    public class FabricHostEntryPointDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<DllHostEntryPointDescription, DllHostEntryPointInfo>
        {
            public override void Compare(DllHostEntryPointDescription expected, DllHostEntryPointDescription actual)
            {
                MiscUtility.CompareEnumerables(expected.HostedDlls, actual.HostedDlls, Helper.Compare);
            }

            public static void Compare(DllHostHostedDllDescription expected, DllHostHostedDllDescription actual)
            {
                Assert.AreEqual<DllHostHostedDllKind>(expected.Kind, actual.Kind);

                switch (expected.Kind)
                {
                    case DllHostHostedDllKind.Managed:
                        Assert.AreEqual<string>(((DllHostHostedManagedDllDescription)expected).AssemblyName, ((DllHostHostedManagedDllDescription)expected).AssemblyName);
                        break;
                    case DllHostHostedDllKind.Unmanaged:
                        Assert.AreEqual<string>(((DllHostHostedUnmanagedDllDescription)expected).DllName, ((DllHostHostedUnmanagedDllDescription)expected).DllName);
                        break;
                    default:
                        Assert.Fail();
                        break;
                }

            }

            public override DllHostEntryPointDescription CreateDefaultDescription()
            {
                return new DllHostEntryPointDescription
                {
                    HostedDlls =
                    {
                        new DllHostHostedManagedDllDescription
                        {
                            AssemblyName = "foo"
                        }
                    }
                };
            }

            public override DllHostEntryPointInfo CreateDefaultInfo()
            {
                return new DllHostEntryPointInfo
                {
                    HostedDlls =
                    {
                        new DllHostItemInfo
                        {
                            Item = "foo",
                            Type = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED
                        }
                    }
                };
            }

            public override DllHostEntryPointDescription Factory(DllHostEntryPointInfo info)
            {
                return DllHostEntryPointDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricHostEntryPointDescription_ParseTest()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricHostEntryPointDescription_NullCollection()
        {
            HelperInstance.ParseTestHelper((info) =>
                {
                    info.HostedDlls.Clear();
                    info.HostedDlls.ReturnNullIfEmpty = true;
                },
                (description) =>
                {
                    description.HostedDlls.Clear();
                });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void FabricHostEntryPointDescription_IsolationPolicyTypeTest()
        {
            this.IsolationPolicyTypeEnumTestHelper(NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_DOMAIN, DllHostIsolationPolicy.DedicatedDomain);
            this.IsolationPolicyTypeEnumTestHelper(NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS, DllHostIsolationPolicy.DedicatedProcess);
            this.IsolationPolicyTypeEnumTestHelper(NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_SHARED_DOMAIN, DllHostIsolationPolicy.SharedDomain);
        }

        private void IsolationPolicyTypeEnumTestHelper(NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY native, DllHostIsolationPolicy isolationPolicy)
        {
            HelperInstance.ParseTestHelper((info) => info.IsolationPolicyType = native, (description) => description.IsolationPolicy = isolationPolicy);
        }
    }

    [TestClass]
    public class ExecutableEntryPointDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<ExeHostEntryPointDescription, ExecutableEntryPointInfo>
        {
            public override void Compare(ExeHostEntryPointDescription expected, ExeHostEntryPointDescription actual)
            {
                Assert.AreEqual(expected.Program, actual.Program);
                Assert.AreEqual(expected.Arguments, actual.Arguments);
                Assert.AreEqual(expected.WorkingFolder, actual.WorkingFolder);
                Assert.AreEqual(expected.PeriodicInterval, actual.PeriodicInterval);
            }

            public override ExeHostEntryPointDescription CreateDefaultDescription()
            {
                return new ExeHostEntryPointDescription
                {
                    Program = "foo",
                    Arguments = null,
                    WorkingFolder = ExeHostWorkingFolder.Work,
                    PeriodicInterval = 3
                };
            }

            public override ExecutableEntryPointInfo CreateDefaultInfo()
            {
                return new ExecutableEntryPointInfo
                {
                    Program = "foo",
                    Arguments = null,
                    WorkingFolder = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_WORK,
                    PeriodicIntervalInSeconds = 3
                };
            }

            public override ExeHostEntryPointDescription Factory(ExecutableEntryPointInfo info)
            {
                return ExeHostEntryPointDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ExecutableEntryPointDescription_ParseTest()
        {
            HelperInstance.ParseTestHelper(null, null);
        }
    }

    [TestClass]
    public class EntryPointDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<EntryPointDescription, EntryPointInfo>
        {
            public override void Compare(EntryPointDescription expected, EntryPointDescription actual)
            {
                TestUtility.Compare<EntryPointDescription, DllHostEntryPointDescription, ExeHostEntryPointDescription>(
                    expected,
                    actual,
                    FabricHostEntryPointDescriptionTest.HelperInstance.Compare,
                    ExecutableEntryPointDescriptionTest.HelperInstance.Compare);
            }

            public override EntryPointDescription CreateDefaultDescription()
            {
                return ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultDescription();
            }

            public override EntryPointInfo CreateDefaultInfo()
            {
                return new EntryPointInfo { Description = ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultInfo() };
            }

            public override EntryPointDescription Factory(EntryPointInfo info)
            {
                return EntryPointDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EntryPointDescription_FabricHostEntryPointIsParsed()
        {
            HelperInstance.ParseTestHelperEx((info) => info.Description = FabricHostEntryPointDescriptionTest.HelperInstance.CreateDefaultInfo(), () => FabricHostEntryPointDescriptionTest.HelperInstance.CreateDefaultDescription());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EntryPointDescription_ExeEntryPointIsParsed()
        {
            HelperInstance.ParseTestHelperEx((info) => info.Description = ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultInfo(), () => ExecutableEntryPointDescriptionTest.HelperInstance.CreateDefaultDescription());
        }
    }
}