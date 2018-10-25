// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Reflection;

    internal class StoreLayoutSpecification
    {
        private NativeImageModel.IFabricStoreLayoutSpecification2 nativeStoreLayout;

        private StoreLayoutSpecification(NativeImageModel.IFabricStoreLayoutSpecification2 nativeStoreLayout)
        {
            this.nativeStoreLayout = nativeStoreLayout;
        }

        public static StoreLayoutSpecification Create()
        {
            return Utility.WrapNativeSyncInvokeInMTA<StoreLayoutSpecification>(StoreLayoutSpecification.CreateHelper, "StoreLayoutSpecification.Create");
        }

        public void SetRoot(string value)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.SetRootHelper(value), "StoreLayoutSpecification.SetRoot");
        }

        public string GetRoot()
        {
            return Utility.WrapNativeSyncInvokeInMTA<string>(this.GetRootHelper, "StoreLayoutSpecification.GetRoot");
        }

        public string GetApplicationManifestFile(
            string applicationTypeName,
            string applicationTypeVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetApplicationManifestFileHelper(applicationTypeName, applicationTypeVersion), "StoreLayoutSpecification.GetApplicationManifestFile");
        }

        public string GetApplicationInstanceFile(
            string applicationTypeName,
            string applicationId,
            string applicationInstanceVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetApplicationInstanceFileHelper(applicationTypeName, applicationId, applicationInstanceVersion), "StoreLayoutSpecification.GetAPplicationInstanceFile");
        }

        public string GetApplicationPackageFile(
            string applicationTypeName,
            string applicationId,
            string applicationRolloutVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetApplicationPackageFileHelper(applicationTypeName, applicationId, applicationRolloutVersion), "StoreLayoutSpecification.GetApplicationPackageFile");
        }

        public string GetServicePackageFile(
            string applicationTypeName,
            string applicationId,
            string servicePackageName,
            string servicePackageRolloutVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServicePackageFileHelper(applicationTypeName, applicationId, servicePackageName, servicePackageRolloutVersion), "StoreLayoutSpecification.GetServicePackageFileHelper");
        }

        public string GetServiceManifestFile(
            string applicationTypeName,
            string serviceManifestName,
            string serviceManifestVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServiceManifestFileHelper(applicationTypeName, serviceManifestName, serviceManifestVersion), "StoreLayoutSpecification.GetServiceManifestFile");
        }

        public string GetServiceManifestChecksumFile(
            string applicationTypeName,
            string serviceManifestName,
            string serviceManifestVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServiceManifestChecksumFileHelper(applicationTypeName, serviceManifestName, serviceManifestVersion), "StoreLayoutSpecification.GetServiceManifestChecksumFile");
        }

        public string GetCodePackageFolder(string applicationTypeName, string servicePackageName, string codePackageName, string codePackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageFolderHelper(applicationTypeName, servicePackageName, codePackageName, codePackageVersion), "StoreLayoutSpecification.GetCodePackageFolder");
        }

        public string GetCodePackageChecksumFile(string applicationTypeName, string servicePackageName, string codePackageName, string codePackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageChecksumFileHelper(applicationTypeName, servicePackageName, codePackageName, codePackageVersion), "StoreLayoutSpecification.GetCodePackageChecksumFile");
        }

        public string GetConfigPackageFolder(string applicationTypeName, string servicePackageName, string configPackageName, string configPackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetConfigPackageFolderHelper(applicationTypeName, servicePackageName, configPackageName, configPackageVersion), "StoreLayoutSpecification.GetConfigPackageFolder");
        }

        public string GetConfigPackageChecksumFile(string applicationTypeName, string servicePackageName, string configPackageName, string configPackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetConfigPackageChecksumFileHelper(applicationTypeName, servicePackageName, configPackageName, configPackageVersion), "StoreLayoutSpecification.GetConfigPackageChecksumFile");
        }

        public string GetDataPackageFolder(string applicationTypeName, string servicePackageName, string dataPackageName, string dataPackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetDataPackageFolderHelper(applicationTypeName, servicePackageName, dataPackageName, dataPackageVersion), "StoreLayoutSpecification.GetDataPackageFolder");
        }

        public string GetDataPackageChecksumFile(string applicationTypeName, string servicePackageName, string dataPackageName, string dataPackageVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetDataPackageChecksumFileHelper(applicationTypeName, servicePackageName, dataPackageName, dataPackageVersion), "StoreLayoutSpecification.GetDataPackageChecksumFile");
        }

        public string GetSettingsFile(string configPackageFolder)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetSettingsFileHelper(configPackageFolder), "StoreLayoutSpecification.GetSettingsFile");
        }

        public string GetSubPackageArchiveFile(string packageFolder)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetSubPackageArchiveFileHelper(packageFolder), "StoreLayoutSpecification.GetSubPackageArchiveFile");
        }

        #region Interop code

        private static StoreLayoutSpecification CreateHelper()
        {
            Guid iid = typeof(NativeImageModel.IFabricStoreLayoutSpecification2).GetTypeInfo().GUID;
            NativeImageModel.IFabricStoreLayoutSpecification2 newNativeStoreLayout = null;

            newNativeStoreLayout = (NativeImageModel.IFabricStoreLayoutSpecification2)NativeImageModel.FabricCreateStoreLayoutSpecification(ref iid);

            return new StoreLayoutSpecification(newNativeStoreLayout);
        }

        private void SetRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeStoreLayout.SetRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetRootHelper()
        {
            return StringResult.FromNative(this.nativeStoreLayout.GetRoot());
        }

        private string GetApplicationManifestFileHelper(
            string applicationTypeName,
            string applicationTypeVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetApplicationManifestFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(applicationTypeVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetApplicationInstanceFileHelper(
            string applicationTypeName,
            string applicationId,
            string applicationInstanceVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetApplicationInstanceFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(applicationInstanceVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetApplicationPackageFileHelper(
            string applicationTypeName,
            string applicationId,
            string applicationRolloutVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetApplicationPackageFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(applicationRolloutVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetServicePackageFileHelper(
            string applicationTypeName,
            string applicationId,
            string servicePackageName,
            string servicePackageRolloutVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetServicePackageFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(servicePackageName),
                    pin.AddBlittable(servicePackageRolloutVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetServiceManifestFileHelper(
            string applicationTypeName,
            string serviceManifestName,
            string serviceManifestVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetServiceManifestFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(serviceManifestVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetServiceManifestChecksumFileHelper(
            string applicationTypeName,
            string serviceManifestName,
            string serviceManifestVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetServiceManifestChecksumFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(serviceManifestVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodePackageFolderHelper(
            string applicationTypeName,
            string serviceManifestName,
            string codePackageName,
            string codePackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetCodePackageFolder(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(codePackageName),
                    pin.AddBlittable(codePackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodePackageChecksumFileHelper(
            string applicationTypeName,
            string serviceManifestName,
            string codePackageName,
            string codePackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetCodePackageChecksumFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(codePackageName),
                    pin.AddBlittable(codePackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetConfigPackageFolderHelper(
            string applicationTypeName,
            string serviceManifestName,
            string configPackageName,
            string configPackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetConfigPackageFolder(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(configPackageName),
                    pin.AddBlittable(configPackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetConfigPackageChecksumFileHelper(
            string applicationTypeName,
            string serviceManifestName,
            string configPackageName,
            string configPackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetConfigPackageChecksumFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(configPackageName),
                    pin.AddBlittable(configPackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetDataPackageFolderHelper(
            string applicationTypeName,
            string serviceManifestName,
            string dataPackageName,
            string dataPackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetDataPackageFolder(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(dataPackageName),
                    pin.AddBlittable(dataPackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetDataPackageChecksumFileHelper(
            string applicationTypeName,
            string serviceManifestName,
            string dataPackageName,
            string dataPackageVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetDataPackageChecksumFile(
                    pin.AddBlittable(applicationTypeName),
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(dataPackageName),
                    pin.AddBlittable(dataPackageVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetSettingsFileHelper(string configPackageFolder)
        {
            using (var pin = new PinBlittable(configPackageFolder))
            {
                return StringResult.FromNative(
                    this.nativeStoreLayout.GetSettingsFile(pin.AddrOfPinnedObject()));
            }
        }

        private string GetSubPackageArchiveFileHelper(
            string packageFolder)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeStoreLayout.GetSubPackageArchiveFile(
                    pin.AddBlittable(packageFolder));

                return StringResult.FromNative(result);
            }
        }

        #endregion
    }
}