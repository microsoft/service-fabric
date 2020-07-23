// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Description;

    #region Native Stubs/Wrappers/Helpers

    /// <summary>
    /// Wrapper for the native ICodePackageActivationContext object
    /// </summary>
    class CodePackageActivationContextStub : NativeRuntime.IFabricCodePackageActivationContext6
    {
        private readonly RegistrationTracker<NativeRuntime.IFabricCodePackageChangeHandler> codePackageRegistrations = new RegistrationTracker<NativeRuntime.IFabricCodePackageChangeHandler>();
        private readonly RegistrationTracker<NativeRuntime.IFabricConfigurationPackageChangeHandler> configPackageRegistrations = new RegistrationTracker<NativeRuntime.IFabricConfigurationPackageChangeHandler>();
        private readonly RegistrationTracker<NativeRuntime.IFabricDataPackageChangeHandler> dataPackageRegistrations = new RegistrationTracker<NativeRuntime.IFabricDataPackageChangeHandler>();

        public CodePackageActivationContextStub()
        {
            this.ServiceManifestInfo_Internal = new ServiceManifestInfo();
        }

        public string ListenAddress_Internal { get; set; }
        public string PublishAddress_Internal { get; set; }
        public string ApplicationTypeName_Internal { get; set; }
        public string ApplicationName_Internal { get; set; }
        public string CodePackageName_Internal { get; set; }
        public string CodePackageVersion_Internal { get; set; }
        public string ContextId_Internal { get; set; }
        public string LogDirectory_Internal { get; set; }
        public string WorkDirectory_Internal { get; set; }
        public string TempDirectory_Internal { get; set; }
        public ServiceManifestInfo ServiceManifestInfo_Internal { get; set; }

        public IList<NativeRuntime.IFabricCodePackageChangeHandler> CodePackageRegistrations
        {
            get
            {
                return this.codePackageRegistrations.Registrations;
            }
        }

        public IList<NativeRuntime.IFabricDataPackageChangeHandler> DataPackageRegistrations
        {
            get
            {
                return this.dataPackageRegistrations.Registrations;
            }
        }

        public IList<NativeRuntime.IFabricConfigurationPackageChangeHandler> ConfigPackageRegistrations
        {
            get
            {
                return this.configPackageRegistrations.Registrations;
            }
        }

        public IntPtr get_CodePackageName()
        {
            return Marshal.StringToCoTaskMemUni(this.CodePackageName_Internal);
        }

        public IntPtr get_CodePackageVersion()
        {
            return Marshal.StringToCoTaskMemUni(this.CodePackageVersion_Internal);
        }

        public IntPtr get_ContextId()
        {
            return Marshal.StringToCoTaskMemUni(this.ContextId_Internal);
        }

        public NativeCommon.IFabricStringListResult GetCodePackageNames()
        {
            List<string> strings = new List<string>();
            foreach (CodePackageInfo info in this.ServiceManifestInfo_Internal.CodePackages)
            {
                strings.Add(info.Name);
            }

            return new StringListResult(strings);
        }

        public NativeCommon.IFabricStringListResult GetConfigurationPackageNames()
        {
            List<string> strings = new List<string>();
            foreach (ConfigurationPackageInfo info in this.ServiceManifestInfo_Internal.ConfigurationPackages)
            {
                strings.Add(info.Name);
            }

            return new StringListResult(strings);
        }

        public NativeCommon.IFabricStringListResult GetDataPackageNames()
        {
            List<string> strings = new List<string>();
            foreach (DataPackageInfo info in this.ServiceManifestInfo_Internal.DataPackages)
            {
                strings.Add(info.Name);
            }

            return new StringListResult(strings);
        }

        public NativeRuntime.IFabricCodePackage GetCodePackage(IntPtr codePackageName)
        {
            var elem = this.ServiceManifestInfo_Internal.CodePackages.FirstOrDefault(cp => cp.Name == Marshal.PtrToStringUni(codePackageName));
            Assert.IsNotNull(elem, "This implies something in the test is broken");
            return elem;
        }

        public NativeRuntime.IFabricConfigurationPackage GetConfigurationPackage(IntPtr configurationPackageName)
        {
            var elem = this.ServiceManifestInfo_Internal.ConfigurationPackages.FirstOrDefault(cp => cp.Name == Marshal.PtrToStringUni(configurationPackageName));
            Assert.IsNotNull(elem, "This implies something in the test is broken");
            return elem;
        }

        public NativeRuntime.IFabricDataPackage GetDataPackage(IntPtr dataPackageName)
        {
            var elem = this.ServiceManifestInfo_Internal.DataPackages.FirstOrDefault(cp => cp.Name == Marshal.PtrToStringUni(dataPackageName));
            Assert.IsNotNull(elem, "This implies something in the test is broken");
            return elem;
        }

        public IntPtr get_ServiceTypes()
        {
            if (this.ServiceManifestInfo_Internal == null || this.ServiceManifestInfo_Internal.ServiceTypes == null)
            {
                return IntPtr.Zero;
            }

            return TestUtility.StructureToIntPtr(this.ServiceManifestInfo_Internal.ServiceTypes.ToNative());
        }

        public IntPtr get_ServiceGroupTypes()
        {
            if (this.ServiceManifestInfo_Internal == null || this.ServiceManifestInfo_Internal.ServiceGroupTypes == null)
            {
                return IntPtr.Zero;
            }

            return TestUtility.StructureToIntPtr(this.ServiceManifestInfo_Internal.ServiceGroupTypes.ToNative());
        }

        public IntPtr get_ApplicationPrincipals()
        {
            Assert.Fail("CodePackageActivationContextStub.get_ApplicationPrincipals not implemented");
            return IntPtr.Zero;
        }

        public IntPtr get_ServiceEndpointResources()
        {
            if (this.ServiceManifestInfo_Internal == null || this.ServiceManifestInfo_Internal.Endpoints == null)
            {
                return IntPtr.Zero;
            }

            return TestUtility.StructureToIntPtr(this.ServiceManifestInfo_Internal.Endpoints.ToNative());
        }

        public IntPtr GetServiceEndpointResource(IntPtr endpointName)
        {
            Assert.Fail("CodePackageActivationContextStub.GetServiceEndpointResource not implemented");
            return IntPtr.Zero;
        }

        public IntPtr get_LogDirectory()
        {
            return Marshal.StringToCoTaskMemUni(this.LogDirectory_Internal);
        }

        public IntPtr GetServiceManifestDescription()
        {
            if (this.ServiceManifestInfo_Internal == null)
            {
                return IntPtr.Zero;
            }

            return this.ServiceManifestInfo_Internal.ToNative();
        }

        public IntPtr get_WorkDirectory()
        {
            return Marshal.StringToCoTaskMemUni(this.WorkDirectory_Internal);
        }

        public IntPtr get_TempDirectory()
        {
            return Marshal.StringToCoTaskMemUni(this.TempDirectory_Internal);
        }

        public long RegisterCodePackageChangeHandler(NativeRuntime.IFabricCodePackageChangeHandler callback)
        {
            return this.codePackageRegistrations.Add(callback);
        }

        public long RegisterConfigurationPackageChangeHandler(NativeRuntime.IFabricConfigurationPackageChangeHandler callback)
        {
            return this.configPackageRegistrations.Add(callback);
        }

        public long RegisterDataPackageChangeHandler(NativeRuntime.IFabricDataPackageChangeHandler callback)
        {
            return this.dataPackageRegistrations.Add(callback);
        }

        public void UnregisterCodePackageChangeHandler(long callbackHandle)
        {
            this.codePackageRegistrations.Remove(callbackHandle);
        }

        public void UnregisterConfigurationPackageChangeHandler(long callbackHandle)
        {
            this.configPackageRegistrations.Remove(callbackHandle);
        }

        public void UnregisterDataPackageChangeHandler(long callbackHandle)
        {
            this.dataPackageRegistrations.Remove(callbackHandle);
        }

        public IntPtr get_ApplicationName()
        {
            return Marshal.StringToCoTaskMemUni(this.ApplicationName_Internal);
        }

        public IntPtr get_ApplicationTypeName()
        {
            return Marshal.StringToCoTaskMemUni(this.ApplicationTypeName_Internal);
        }

        public NativeCommon.IFabricStringResult GetServiceManifestName()
        {
            return new StringResult(this.ServiceManifestInfo_Internal.Name);
        }

        public NativeCommon.IFabricStringResult GetServiceManifestVersion()
        {
            return new StringResult(this.ServiceManifestInfo_Internal.Version);
        }

        public void ReportApplicationHealth(IntPtr healthInfo)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportApplicationHealth not implemented");
        }

        public void ReportApplicationHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportApplicationHealth2 not implemented");
        }
        
        public void ReportDeployedApplicationHealth(IntPtr healthInfo)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportDeployedApplicationHealth not implemented");
        }

        public void ReportDeployedApplicationHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportDeployedApplicationHealth2 not implemented");
        }

        public void ReportDeployedServicePackageHealth(IntPtr healthInfo)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportDeployedServicePackageHealth not implemented");
        }

        public void ReportDeployedServicePackageHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            Assert.Fail("CodePackageActivationContextStub.ReportDeployedServicePackageHealth2 not implemented");
        }

        public NativeCommon.IFabricStringResult GetDirectory(IntPtr logicalDirectoryName)
        {
            return new StringResult(this.WorkDirectory_Internal);
        }

        public IntPtr get_ServiceListenAddress()
        {
            return Marshal.StringToCoTaskMemUni(this.ListenAddress_Internal);
        }

        public IntPtr get_ServicePublishAddress()
        {
            return Marshal.StringToCoTaskMemUni(this.PublishAddress_Internal);
        }

        private class RegistrationTracker<T>
        {
            private readonly Dictionary<long, T> registrations = new Dictionary<long, T>();
            private long index = 0;

            public IList<T> Registrations
            {
                get
                {
                    return new List<T>(this.registrations.Values);
                }
            }

            public long Add(T obj)
            {
                long handle = this.index++;
                this.registrations[handle] = obj;
                return handle;
            }

            public void Remove(long index)
            {
                this.registrations.Remove(index);
            }
        }
    }

    #region Wrappers for handling the native structs
    /// <summary>
    /// An interface that defines serializability for all the native objects
    /// </summary>
    interface INativeWrapper
    {
        int Size { get; }

        object ToNative();
    }

    /// <summary>
    /// Helper class implementation
    /// </summary>
    abstract class NativeWrapperBase<T> : INativeWrapper
    {
        public int Size
        {
            get { return Marshal.SizeOf(typeof(T)); }
        }

        public object ToNative()
        {
            return (object)this.ToNativeRaw();
        }

        public abstract T ToNativeRaw();
    }

    interface INativeCollectionWrapper : INativeWrapper
    {
        bool ReturnNullIfEmpty { get; set; }

        void Clear();
    }

    /// <summary>
    /// Helper class that wraps the *_LIST structures
    /// It assumes that the children are of TElement (INativeWrapper) and contains a collection of them
    /// The ToNative method then returns the _LIST structure allocating all the items as per the native code
    /// </summary>
    class NativeContainer<TContainer, TElement> : INativeCollectionWrapper, ICollection<TElement>
        where TContainer : new()
        where TElement : INativeWrapper
    {
        private Func<uint, IntPtr, TContainer> containerFactory;
        private IList<TElement> children;

        public NativeContainer(Func<uint, IntPtr, TContainer> containerFactory)
        {
            this.containerFactory = containerFactory;
            this.children = new List<TElement>();
        }

        public bool ReturnNullIfEmpty { get; set; }

        public int Size
        {
            get { return Marshal.SizeOf(typeof(TContainer)); }
        }

        public unsafe object ToNative()
        {
            if (this.children.Count == 0 && this.ReturnNullIfEmpty)
            {
                return null;
            }

            uint count = (uint)this.children.Count;
            IntPtr childrenBuffer = IntPtr.Zero;

            if (this.children.Count != 0)
            {
                int elementSize = this.children[0].Size;
                childrenBuffer = Marshal.AllocHGlobal(elementSize * this.children.Count);

                for (int i = 0; i < this.children.Count; i++)
                {
                    IntPtr ptr = childrenBuffer + elementSize * i;
                    Marshal.StructureToPtr(this.children[i].ToNative(), ptr, false);
                }
            }

            return this.containerFactory(count, childrenBuffer);
        }

        public IEnumerator<TElement> GetEnumerator()
        {
            return this.children.GetEnumerator();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.children.GetEnumerator();
        }

        public void Add(TElement item)
        {
            this.children.Add(item);
        }

        public void Clear()
        {
            this.children.Clear();
        }

        public bool Contains(TElement item)
        {
            return this.children.Contains(item);
        }

        public void CopyTo(TElement[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public int Count
        {
            get { return this.children.Count; }
        }

        public bool IsReadOnly
        {
            get { throw new NotImplementedException(); }
        }

        public bool Remove(TElement item)
        {
            throw new NotImplementedException();
        }
    }

    #endregion

    class StringResult : NativeCommon.IFabricStringResult
    {
        string Value { get; set; }
        IntPtr valuePtr;

        unsafe public StringResult(string value)
        {
            this.Value = value;
            this.valuePtr = Marshal.StringToCoTaskMemUni(this.Value);
        }

        unsafe public IntPtr get_String()
        {
            return this.valuePtr;
        }
    }

    class StringListResult : NativeCommon.IFabricStringListResult
    {
        private List<string> strings { get; set; }
        IntPtr[] stringsPtr;

        unsafe public StringListResult(List<string> strings)
        {
            this.strings = strings;
            this.stringsPtr = new IntPtr[this.strings.Count];

            for (int i = 0; i < this.strings.Count; i++)
            {
                stringsPtr[i] = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
                Marshal.WriteIntPtr(stringsPtr[i], Marshal.StringToCoTaskMemUni(this.strings[i]));
            }
        }

        unsafe public IntPtr GetStrings(out UInt32 itemCount)
        {
            itemCount = (UInt32)strings.Count;
            if (itemCount > 0)
            {
                return stringsPtr[0];
            }
            else
            {
                return IntPtr.Zero;
            }
        }
    }

    /// <summary>
    /// Wrapper for FABRIC_ENDPOINT_RESOURCE_DESCRIPTION
    /// </summary>
    class EndPointInfo : NativeWrapperBase<NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION>
    {
        public string Name { get; set; }
        public string Protocol { get; set; }
        public string Type { get; set; }
        public UInt32 Port { get; set; }
        public string CertificateName { get; set; }
        public string UriScheme { get; set; }
        public string PathSuffix { get; set; }

        private NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1 _additions;

        public override NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION ToNativeRaw()
        {
            IntPtr reserved = IntPtr.Zero;
            if (!String.IsNullOrEmpty(UriScheme) ||
                !String.IsNullOrEmpty(PathSuffix))
            {
                _additions.PathSuffix = Marshal.StringToCoTaskMemUni(PathSuffix);
                _additions.UriScheme = Marshal.StringToCoTaskMemUni(UriScheme);
            }
            return new NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION
            {
                CertificateName = Marshal.StringToCoTaskMemUni(this.CertificateName),
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Port = this.Port,
                Protocol = Marshal.StringToCoTaskMemUni(this.Protocol),
                Type = Marshal.StringToCoTaskMemUni(this.Type),
                Reserved = TestUtility.StructureToIntPtr(_additions)
            };
        }
    }

    #region Packages

    class DllHostItemInfo : NativeWrapperBase<NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION>
    {
        public NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND Type { get; set; }
        public string Item { get; set; }

        public override NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION ToNativeRaw()
        {
            object value = null;
            if (this.Type == NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED)
            {
                value = new NativeTypes.FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION
                {
                    AssemblyName = Marshal.StringToCoTaskMemUni(this.Item)
                };
            }
            else
            {
                value = new NativeTypes.FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION
                {
                    DllName = Marshal.StringToCoTaskMemUni(this.Item)
                };
            }

            return new NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION
            {
                Kind = this.Type,
                Value = value == null ? IntPtr.Zero : TestUtility.StructureToIntPtr(value)
            };
        }
    }

    class DllHostEntryPointInfo : NativeWrapperBase<NativeTypes.FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION>
    {
        public NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY IsolationPolicyType { get; set; }

        public NativeContainer<NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST, DllHostItemInfo> HostedDlls { get; private set; }

        public DllHostEntryPointInfo()
        {
            this.IsolationPolicyType = NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS;
            this.HostedDlls = new NativeContainer<NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST,DllHostItemInfo>(
                (count, buffer) => new NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST { Count = count, Items = buffer });
        }

        public override NativeTypes.FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION
            {
                IsolationPolicyType = this.IsolationPolicyType,
                HostedDlls = TestUtility.StructureToIntPtr(this.HostedDlls.ToNative())
            };
        }
    }

    class ExecutableEntryPointInfo : NativeWrapperBase<NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION>
    {
        public string Program { get; set; }
        public string Arguments { get; set; }
        public NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER WorkingFolder { get; set; }
        public UInt32 PeriodicIntervalInSeconds { get; set; }

        public override NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION
            {
                Program = Marshal.StringToCoTaskMemUni(this.Program),
                Arguments = Marshal.StringToCoTaskMemUni(this.Arguments),
                WorkingFolder = this.WorkingFolder,
                Reserved = TestUtility.StructureToIntPtr(new NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1 { PeriodicIntervalInSeconds = this.PeriodicIntervalInSeconds })
            };
        }
    }

    class EntryPointInfo : NativeWrapperBase<NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION>
    {
        public object Description { get; set; }                

        public T GetDescription<T>() where T : class
        {
            return Description as T;
        }

        public IntPtr GetNativeDescription()
        {
            var exe = this.GetDescription<ExecutableEntryPointInfo>();
            var fabricHost = this.GetDescription<DllHostEntryPointInfo>();

            if (exe != null)
            {
                return TestUtility.StructureToIntPtr(exe.ToNative());
            }
            if (fabricHost != null)
            {
                return TestUtility.StructureToIntPtr(fabricHost.ToNative());
            }

            return IntPtr.Zero;
        }

        public NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND Type
        {
            get
            {
                if (this.Description == null)
                {
                    return NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_NONE;
                }
                if (this.GetDescription<ExecutableEntryPointInfo>() != null)
                {
                    return NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST;
                }
                if (this.GetDescription<DllHostEntryPointInfo>() != null)
                {
                    return NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST;
                }

                return NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_INVALID;
            }
        }

        public override NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION
            {
                Kind = this.Type,
                Value = this.GetNativeDescription()
            };
        }
    }



    /// <summary>
    /// Base interface for dealing with packages in test code
    /// </summary>
    interface IPackageInfo
    {
        string Name { get; }
        string PathProperty { get; }
        string Version { get; set; }
    }

    /// <summary>
    /// FABRIC_CODE_PACKAGE_DESCRIPTION and NativeRuntime.IFabricCodePackage
    /// </summary>
    class CodePackageInfo : NativeWrapperBase<NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION>, 
        NativeRuntime.IFabricCodePackage2,
        IPackageInfo
    {
        public ExecutableEntryPointInfo SetupEntryPoint { get; set; }
        public EntryPointInfo EntryPoint { get; set; }
        public string PathProperty { get; set; }
        public string Name { get; set; }
        public string Version { get; set; }
        public string ServiceManifestName { get; set; }
        public string ServiceManifestVersion { get; set; }
        public bool IsShared { get; set; }
        

        public IntPtr get_Description()
        {
            return TestUtility.StructureToIntPtr(this.ToNativeRaw());
        }

        public IntPtr get_Path()
        {
            return Marshal.StringToCoTaskMemUni(this.PathProperty);
        }

        public IntPtr get_EntryPointRunAsPolicy()
        {
            return IntPtr.Zero;
        }

        public IntPtr get_SetupEntryPointRunAsPolicy()
        {
            return IntPtr.Zero;
        }

        public override NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION
            {
                SetupEntryPoint = this.SetupEntryPoint == null? IntPtr.Zero : TestUtility.StructureToIntPtr(this.SetupEntryPoint.ToNative()),
                EntryPoint = this.EntryPoint == null ? IntPtr.Zero : TestUtility.StructureToIntPtr(this.EntryPoint.ToNative()),
                IsShared = NativeTypes.ToBOOLEAN(this.IsShared),
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Version = Marshal.StringToCoTaskMemUni(this.Version),
                ServiceManifestName = Marshal.StringToCoTaskMemUni(this.ServiceManifestName),
                ServiceManifestVersion = Marshal.StringToCoTaskMemUni(this.ServiceManifestVersion),
            };  
        }
    }


    /// <summary>
    /// FABRIC_DATA_PACKAGE_DESCRIPTION and NativeRuntime.IFabricDataPackage
    /// </summary>
    class DataPackageInfo : NativeWrapperBase<NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION>, NativeRuntime.IFabricDataPackage, IPackageInfo
    {
        public string Name { get; set; }
        public string PathProperty { get; set; }
        public string Version { get; set; }
        public string ServiceManifestName { get; set; }
        public string ServiceManifestVersion { get; set; }

        public IntPtr get_Description()
        {
            return TestUtility.StructureToIntPtr(this.ToNative());
        }

        public IntPtr get_Path()
        {
            return Marshal.StringToCoTaskMemUni(this.PathProperty);
        }

        public override NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION
            {
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Version = Marshal.StringToCoTaskMemUni(this.Version),
                ServiceManifestName = Marshal.StringToCoTaskMemUni(this.ServiceManifestName),
                ServiceManifestVersion = Marshal.StringToCoTaskMemUni(this.ServiceManifestVersion),
            };
        }
    }

    class ConfigurationParameterInfo : NativeWrapperBase<NativeTypes.FABRIC_CONFIGURATION_PARAMETER>
    {
        public string Name { get; set; }
        public string Value { get; set; }

        public override NativeTypes.FABRIC_CONFIGURATION_PARAMETER ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CONFIGURATION_PARAMETER
            {
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Value = Marshal.StringToCoTaskMemUni(this.Value),
            };
        }
    }

    class ConfigurationSectionInfo : NativeWrapperBase<NativeTypes.FABRIC_CONFIGURATION_SECTION>
    {
        public string SectionName { get; set; }
        public NativeContainer<NativeTypes.FABRIC_CONFIGURATION_PARAMETER_LIST, ConfigurationParameterInfo> Parameters { get; private set; }

        public ConfigurationSectionInfo()
        {
            this.Parameters = new NativeContainer<NativeTypes.FABRIC_CONFIGURATION_PARAMETER_LIST, ConfigurationParameterInfo>(
                (count, buffer) => new NativeTypes.FABRIC_CONFIGURATION_PARAMETER_LIST { Count = count, Items = buffer });
        }

        public override NativeTypes.FABRIC_CONFIGURATION_SECTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CONFIGURATION_SECTION
            {
                Name = Marshal.StringToCoTaskMemUni(this.SectionName),
                Parameters = TestUtility.StructureToIntPtr(this.Parameters.ToNative()),
            };
        }
    }

    class ConfigurationSettingsInfo : NativeWrapperBase<NativeTypes.FABRIC_CONFIGURATION_SETTINGS>
    {
        public NativeContainer<NativeTypes.FABRIC_CONFIGURATION_SECTION_LIST, ConfigurationSectionInfo> Sections { get; private set; }

        public ConfigurationSettingsInfo()
        {
            this.Sections = new NativeContainer<NativeTypes.FABRIC_CONFIGURATION_SECTION_LIST, ConfigurationSectionInfo>(
                (count, buffer) => new NativeTypes.FABRIC_CONFIGURATION_SECTION_LIST { Count = count, Items = buffer });
        }

        public override NativeTypes.FABRIC_CONFIGURATION_SETTINGS ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CONFIGURATION_SETTINGS
            {
                Sections = TestUtility.StructureToIntPtr(this.Sections.ToNative()),
            };
        }
    }

    class ConfigurationPackageInfo : NativeWrapperBase<NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION>, NativeRuntime.IFabricConfigurationPackage, IPackageInfo
    {
        public string Name { get; set; }
        public string PathProperty { get; set; }
        public string Version { get; set; }
        public string ServiceManifestName { get; set; }
        public string ServiceManifestVersion { get; set; }
        public ConfigurationSettingsInfo SettingsProperty { get; set; }

        public ConfigurationPackageInfo()
        {
            this.SettingsProperty = new ConfigurationSettingsInfo();
        }

        public IntPtr get_Description()
        {
            return TestUtility.StructureToIntPtr(this.ToNative());
        }

        public IntPtr get_Path()
        {
            return Marshal.StringToCoTaskMemUni(this.PathProperty);
        }

        public IntPtr get_Settings()
        {
            return TestUtility.StructureToIntPtr(this.SettingsProperty.ToNative());
        }

        public IntPtr GetSection(IntPtr sectionName)
        {
            return TestUtility.StructureToIntPtr(this.SettingsProperty.Sections.First(s => s.SectionName == NativeTypes.FromNativeString(sectionName)).ToNative());
        }

        public IntPtr GetValue(IntPtr sectionName, IntPtr parameterName)
        {
            return Marshal.StringToCoTaskMemUni(this.SettingsProperty.Sections.First(s => s.SectionName == NativeTypes.FromNativeString(sectionName)).Parameters.First(p => p.Name == NativeTypes.FromNativeString(parameterName)).Value);
        }

        public override NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION
            {
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Version = Marshal.StringToCoTaskMemUni(this.Version),
                ServiceManifestName = Marshal.StringToCoTaskMemUni(this.ServiceManifestName),
                ServiceManifestVersion = Marshal.StringToCoTaskMemUni(this.ServiceManifestVersion),
            };
        }
    }

    #endregion

    #region ServiceTypes

    class DescriptionExtensionInfo : NativeWrapperBase<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION>
    {
        public string Name { get; set; }
        public string Value { get; set; }

        public override NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION
            {
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Value = Marshal.StringToCoTaskMemAuto(this.Value),
            };
        }
    }

    class ServiceLoadMetricInfo : NativeWrapperBase<NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>
    {
        public string Name { get; set; }
        public NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT Weight { get; set; }
        public uint PrimaryDefaultLoad { get; set; }
        public uint SecondaryDefaultLoad { get; set; }

        public override NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION
            {
                PrimaryDefaultLoad = this.PrimaryDefaultLoad,
                SecondaryDefaultLoad = this.SecondaryDefaultLoad,
                Name = Marshal.StringToCoTaskMemUni(this.Name),
                Weight = this.Weight
            };
        }
    }

    class ServiceTypeInfo : NativeWrapperBase<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION>
    {
        public string ServiceTypeName { get; set; }
        public string PlacementConstraints { get; set; }
        public NativeContainer<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST, DescriptionExtensionInfo> Extensions { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST, ServiceLoadMetricInfo> LoadMetrics { get; private set; }

        public ServiceTypeInfo()
        {
            this.Extensions = new NativeContainer<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST, DescriptionExtensionInfo>(
                (count, buffer) => new NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST { Count = count, Items = buffer });

            this.LoadMetrics = new NativeContainer<NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST, ServiceLoadMetricInfo>(
                (count, buffer) => new NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST { Count = count, Items = buffer });
        }

        protected virtual object ToNativeDerived(IntPtr serviceTypeName, IntPtr placementConstraints, IntPtr extensions, IntPtr loadMetrics)
        {
            return null;
        }

        public virtual NativeTypes.FABRIC_SERVICE_KIND Kind
        {
            get
            {
                return NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_INVALID;
            }
        }

        public override NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION ToNativeRaw()
        {
            if (this.Kind == NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_INVALID)
            {
                return new NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION
                {
                    Kind = this.Kind,
                    Value = IntPtr.Zero,
                };
            }
            else
            {
                return new NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION
                {
                    Kind = this.Kind,
                    Value = TestUtility.StructureToIntPtr(this.ToNativeDerived()),
                };
            }
        }

        public object ToNativeDerived()
        {
            return this.ToNativeDerived(
                        Marshal.StringToCoTaskMemUni(this.ServiceTypeName),
                        Marshal.StringToCoTaskMemUni(this.PlacementConstraints),
                        TestUtility.StructureToIntPtr(this.Extensions.ToNative()),
                        TestUtility.StructureToIntPtr(this.LoadMetrics.ToNative()));
        }
    }

    class StatelessServiceTypeInfo : ServiceTypeInfo
    {
        public override NativeTypes.FABRIC_SERVICE_KIND Kind
        {
            get
            {
                return NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATELESS;
            }
        }

        protected override object ToNativeDerived(IntPtr serviceTypeName, IntPtr placementConstraints, IntPtr extensions, IntPtr loadMetrics)
        {
            return new NativeTypes.FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION
            {
                ServiceTypeName = serviceTypeName,
                PlacementConstraints = placementConstraints,
                Extensions = extensions,
                LoadMetrics = loadMetrics,
            };
        }

    }

    class StatefulServiceTypeInfo : ServiceTypeInfo
    {
        public bool HasPersistedState { get; set; }

        public override NativeTypes.FABRIC_SERVICE_KIND Kind
        {
            get
            {
                return NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATEFUL;
            }
        }

        protected override object ToNativeDerived(IntPtr serviceTypeName, IntPtr placementConstraints, IntPtr extensions, IntPtr loadMetrics)
        {
            return new NativeTypes.FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION
            {
                ServiceTypeName = serviceTypeName,
                PlacementConstraints = placementConstraints,
                Extensions = extensions,
                HasPersistedState = NativeTypes.ToBOOLEAN(this.HasPersistedState),
                LoadMetrics = loadMetrics,
            };
        }
    }

    class ServiceGroupTypeMemberInfo : NativeWrapperBase<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION>
    {
        public string ServiceTypeName { get; set; }

        public override NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION
            {
                ServiceTypeName = Marshal.StringToCoTaskMemUni(this.ServiceTypeName)
            };
        }
    }

    class ServiceGroupTypeInfo : NativeWrapperBase<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION>
    {
        public ServiceTypeInfo Description { get; set; }
        public string PlacementConstraints { get; set; }
        public bool UseImplicitFactory { get; set; }

        public NativeContainer<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST, ServiceGroupTypeMemberInfo> Members { get; private set; }

        public ServiceGroupTypeInfo()
        {
            this.Members = new NativeContainer<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST, ServiceGroupTypeMemberInfo>(
                (count, buffer) => new NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST { Count = count, Items = buffer });
        }

        public override NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION ToNativeRaw()
        {
            return new NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION
            {
                Description = TestUtility.StructureToIntPtr(this.Description.ToNativeRaw()),
                Members = TestUtility.StructureToIntPtr(this.Members.ToNative()),
                UseImplicitFactory = this.UseImplicitFactory ? (sbyte)1 : (sbyte)0
            };
        }
    }


    #endregion

    /// <summary>
    /// The service manifest
    /// </summary>
    class ServiceManifestInfo : NativeRuntime.IFabricServiceManifestDescriptionResult
    {
        public IntPtr get_Description()
        {
            return this.ToNative();
        }

        public NativeContainer<NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST, EndPointInfo> Endpoints { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION_LIST, CodePackageInfo> CodePackages { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION_LIST, DataPackageInfo> DataPackages { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION_LIST, ConfigurationPackageInfo> ConfigurationPackages { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_LIST, ServiceTypeInfo> ServiceTypes { get; private set; }
        public NativeContainer<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST, ServiceGroupTypeInfo> ServiceGroupTypes { get; private set; }
        public string Name { get; set; }
        public string Version { get; set; }

        public ServiceManifestInfo()
        {
            this.Endpoints = new NativeContainer<NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST, EndPointInfo>(
                (count, buffer) => new NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST { Count = count, Items = buffer });

            this.CodePackages = new NativeContainer<NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION_LIST, CodePackageInfo>(
                (count, buffer) => new NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION_LIST { Count = count, Items = buffer });

            this.DataPackages = new NativeContainer<NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION_LIST, DataPackageInfo>(
                (count, buffer) => new NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION_LIST { Count = count, Items = buffer });

            this.ConfigurationPackages = new NativeContainer<NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION_LIST, ConfigurationPackageInfo>(
                (count, buffer) => new NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION_LIST { Count = count, Items = buffer });

            this.ServiceTypes = new NativeContainer<NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_LIST, ServiceTypeInfo>(
                (count, buffer) => new NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_LIST { Count = count, Items = buffer });

            this.ServiceGroupTypes = new NativeContainer<NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST, ServiceGroupTypeInfo>(
                (count, buffer) => new NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST { Count = count, Items = buffer });
        }

        public unsafe IntPtr ToNative()
        {
            NativeTypes.FABRIC_SERVICE_MANIFEST_DESCRIPTION serviceManifestDescription = new NativeTypes.FABRIC_SERVICE_MANIFEST_DESCRIPTION();
            serviceManifestDescription.Endpoints = TestUtility.StructureToIntPtr(this.Endpoints.ToNative());
            serviceManifestDescription.CodePackages = TestUtility.StructureToIntPtr(this.CodePackages.ToNative());
            serviceManifestDescription.DataPackages = TestUtility.StructureToIntPtr(this.DataPackages.ToNative());
            serviceManifestDescription.ConfigurationPackages = TestUtility.StructureToIntPtr(this.ConfigurationPackages.ToNative());
            serviceManifestDescription.ServiceTypes = TestUtility.StructureToIntPtr(this.ServiceTypes.ToNative());
            serviceManifestDescription.ServiceGroupTypes = TestUtility.StructureToIntPtr(this.ServiceGroupTypes.ToNative());
            serviceManifestDescription.Name = Marshal.StringToCoTaskMemUni(this.Name);
            serviceManifestDescription.Version = Marshal.StringToCoTaskMemUni(this.Version);

            return TestUtility.StructureToIntPtr(serviceManifestDescription);
        }
    }

    #endregion
}