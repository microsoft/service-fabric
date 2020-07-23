// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Fabric.Description;

    internal sealed class ServiceManifest
    {
        private readonly KeyedItemCollection<string, ServiceTypeDescription> serviceTypes = new KeyedItemCollection<string, ServiceTypeDescription>(d => d.ServiceTypeName);
        private readonly KeyedItemCollection<string, ServiceGroupTypeDescription> serviceGroupTypes = new KeyedItemCollection<string, ServiceGroupTypeDescription>(d => d.ServiceTypeDescription.ServiceTypeName);
        private readonly KeyedItemCollection<string, CodePackage> codePackages = new KeyedItemCollection<string, CodePackage>(d => d.Description.Name);
        private readonly KeyedItemCollection<string, DataPackage> dataPackages = new KeyedItemCollection<string, DataPackage>(d => d.Description.Name);
        private readonly KeyedItemCollection<string, ConfigurationPackage> configPackages = new KeyedItemCollection<string, ConfigurationPackage>(d => d.Description.Name);
        private readonly KeyedItemCollection<string, EndpointResourceDescription> endpoints = new KeyedItemCollection<string, EndpointResourceDescription>(d => d.Name);

        public ServiceManifest()
        {
        }

        public string Name { get; set; }

        public string Description { get; set; }

        public string Version { get; set; }

        public KeyedCollection<string, ServiceTypeDescription> ServiceTypeDescriptions
        {
            get
            {
                return this.serviceTypes;
            }
        }

        public KeyedCollection<string, ServiceGroupTypeDescription> ServiceGroupTypeDescriptions
        {
            get
            {
                return this.serviceGroupTypes;
            }
        }

        public KeyedCollection<string, CodePackage> CodePackages
        {
            get
            {
                return this.codePackages;
            }
        }

        public KeyedCollection<string, DataPackage> DataPackages
        {
            get
            {
                return this.dataPackages;
            }
        }

        public KeyedCollection<string, ConfigurationPackage> ConfigurationPackages
        {
            get
            {
                return this.configPackages;
            }
        }

        public KeyedCollection<string, EndpointResourceDescription> Endpoints
        {
            get
            {
                return this.endpoints;
            }
        }

        internal static ServiceManifest CreateFromCodePackageActivationContext(CodePackageActivationContext context)
        {
            ServiceManifest manifest = new ServiceManifest();

            foreach (ServiceTypeDescription description in context.GetServiceTypes())
            {
                manifest.ServiceTypeDescriptions.Add(description);
            }

            foreach (ServiceGroupTypeDescription description in context.GetServiceGroupTypes())
            {
                manifest.ServiceGroupTypeDescriptions.Add(description);
            }

            ServiceManifest.ParseCodePackages(context, manifest);
            ServiceManifest.ParseConfigurationPackages(context, manifest);
            ServiceManifest.ParseDataPackages(context, manifest);
            ServiceManifest.ParseEndPoints(context, manifest);

            manifest.Name = manifest.CodePackages[context.CodePackageName].Description.ServiceManifestName;
            manifest.Version = manifest.CodePackages[context.CodePackageName].Description.ServiceManifestVersion;

            return manifest;
        }

        private static void ParseCodePackages(CodePackageActivationContext context, ServiceManifest manifest)
        {
            IList<string> names = context.GetCodePackageNames();
            foreach (string name in names)
            {
                manifest.CodePackages.Add(context.GetCodePackageObject(name));
            }
        }

        private static void ParseConfigurationPackages(CodePackageActivationContext context, ServiceManifest manifest)
        {
            IList<string> names = context.GetConfigurationPackageNames();
            foreach (string name in names)
            {
                manifest.ConfigurationPackages.Add(context.GetConfigurationPackageObject(name));
            }
        }

        private static void ParseDataPackages(CodePackageActivationContext context, ServiceManifest manifest)
        {
            IList<string> names = context.GetDataPackageNames();
            foreach (string name in names)
            {
                manifest.DataPackages.Add(context.GetDataPackageObject(name));
            }
        }

        private static void ParseEndPoints(CodePackageActivationContext context, ServiceManifest manifest)
        {
            foreach (EndpointResourceDescription description in context.GetEndpoints())
            {
                manifest.Endpoints.Add(description);
            }
        }

        internal static unsafe ServiceManifest CreateFromNative(NativeRuntime.IFabricCodePackageActivationContext nativeContext)
        {
            ServiceManifest manifest = new ServiceManifest();

            var nativeServiceTypes = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_LIST*)nativeContext.get_ServiceTypes();
            if (nativeServiceTypes != null)
            {
                for (int i = 0; i < nativeServiceTypes->Count; i++)
                {
                    manifest.ServiceTypeDescriptions.Add(ServiceTypeDescription.CreateFromNative(nativeServiceTypes->Items + i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION))));
                }
            }

            var nativeServiceGroupTypes = (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST*)nativeContext.get_ServiceGroupTypes();
            if (nativeServiceGroupTypes != null)
            {
                for (int i = 0; i < nativeServiceGroupTypes->Count; i++)
                {
                    manifest.ServiceGroupTypeDescriptions.Add(ServiceGroupTypeDescription.CreateFromNative(nativeServiceGroupTypes->Items + i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION))));
                }
            }

            ServiceManifest.ParseCodePackages(nativeContext, manifest);
            ServiceManifest.ParseConfigurationPackages(nativeContext, manifest);
            ServiceManifest.ParseDataPackages(nativeContext, manifest);
            ServiceManifest.ParseEndPoints(nativeContext, manifest);

            ReleaseAssert.AssertIfNot(manifest.CodePackages.Count > 0, "There should be at least one code package");
            manifest.Name = manifest.CodePackages[0].Description.ServiceManifestName;
            manifest.Version = manifest.CodePackages[0].Description.ServiceManifestVersion;

            GC.KeepAlive(nativeContext);

            return manifest;
        }

        private static unsafe void ParseCodePackages(NativeRuntime.IFabricCodePackageActivationContext nativeContext, ServiceManifest manifest)
        {
            IList<string> names = new List<string>();
            NativeCommon.IFabricStringListResult nativeResult = nativeContext.GetCodePackageNames();
            uint count;
            IntPtr nativeNames = nativeResult.GetStrings(out count);
            for (int i = 0; i < count; i++)
            {
                NativeRuntime.IFabricCodePackage codePackageResult = nativeContext.GetCodePackage(Marshal.ReadIntPtr((IntPtr)(nativeNames + i)));
                NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION nativeDescription = *(NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION*)(codePackageResult.get_Description());
                NativeRuntime.IFabricCodePackage nativePackage = null;
                string packageName = NativeTypes.FromNativeString(nativeDescription.Name);

                using (var pin = new PinBlittable(packageName))
                {
                    nativePackage = nativeContext.GetCodePackage(pin.AddrOfPinnedObject());
                }

                manifest.CodePackages.Add(CodePackage.CreateFromNative(nativePackage));

                GC.KeepAlive(codePackageResult);
            }

            GC.KeepAlive(nativeResult);
        }

        private static unsafe void ParseConfigurationPackages(NativeRuntime.IFabricCodePackageActivationContext nativeContext, ServiceManifest manifest)
        {
            IList<string> names = new List<string>();
            NativeCommon.IFabricStringListResult nativeResult = nativeContext.GetConfigurationPackageNames();
            uint count;
            IntPtr nativeNames = nativeResult.GetStrings(out count);
            for (int i = 0; i < count; i++)
            {
                NativeRuntime.IFabricConfigurationPackage configPackageResult = nativeContext.GetConfigurationPackage(Marshal.ReadIntPtr((IntPtr)(nativeNames + i)));
                NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION nativeDescription = *(((NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION*)configPackageResult.get_Description()));
                NativeRuntime.IFabricConfigurationPackage nativePackage = null;
                string packageName = NativeTypes.FromNativeString(nativeDescription.Name);

                using (var pin = new PinBlittable(packageName))
                {
                    nativePackage = nativeContext.GetConfigurationPackage(pin.AddrOfPinnedObject());
                }

                manifest.ConfigurationPackages.Add(ConfigurationPackage.CreateFromNative(nativePackage));
                GC.KeepAlive(configPackageResult);
            }

            GC.KeepAlive(nativeResult);
        }

        private static unsafe void ParseDataPackages(NativeRuntime.IFabricCodePackageActivationContext nativeContext, ServiceManifest manifest)
        {
            IList<string> names = new List<string>();
            NativeCommon.IFabricStringListResult nativeResult = nativeContext.GetDataPackageNames();
            uint count;
            IntPtr nativeNames = nativeResult.GetStrings(out count);
            for (int i = 0; i < count; i++)
            {
                NativeRuntime.IFabricDataPackage dataPackageResult = nativeContext.GetDataPackage(Marshal.ReadIntPtr((IntPtr)(nativeNames + i)));
                NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION nativeDescription = *(((NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION*)dataPackageResult.get_Description()));
                NativeRuntime.IFabricDataPackage nativePackage = null;
                string packageName = NativeTypes.FromNativeString(nativeDescription.Name);

                using (var pin = new PinBlittable(packageName))
                {
                    nativePackage = nativeContext.GetDataPackage(pin.AddrOfPinnedObject());
                }

                manifest.DataPackages.Add(DataPackage.CreateFromNative(nativePackage));
            }
        }

        private static unsafe void ParseEndPoints(NativeRuntime.IFabricCodePackageActivationContext nativeContext, ServiceManifest manifest)
        {
            var list = (NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST*)nativeContext.get_ServiceEndpointResources();
            if (list != null)
            {
                for (int i = 0; i < list->Count; i++)
                {
                    IntPtr nativeDescription = list->Items + i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION));

                    manifest.Endpoints.Add(EndpointResourceDescription.CreateFromNative(nativeDescription));
                }
            }
        }
    }
}