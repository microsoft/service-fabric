// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Reflection;

    internal class BuildLayoutSpecification
    {
        private NativeImageModel.IFabricBuildLayoutSpecification2 nativeBuildLayout;

        private BuildLayoutSpecification(NativeImageModel.IFabricBuildLayoutSpecification2 nativeBuildLayout)
        {
            this.nativeBuildLayout = nativeBuildLayout;
        }

        public static BuildLayoutSpecification Create()
        {
            return Utility.WrapNativeSyncInvokeInMTA<BuildLayoutSpecification>(BuildLayoutSpecification.CreateHelper, "BuildLayoutSpecification.Create");
        }

        public void SetRoot(string value)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.SetRootHelper(value), "BuildLayoutSpecification.SetRoot");
        }

        public string GetRoot()
        {
            return Utility.WrapNativeSyncInvokeInMTA<string>(this.GetRootHelper, "BuildLayoutSpecification.GetRoot");
        }

        public string GetApplicationManifestFile()
        {
            return Utility.WrapNativeSyncInvokeInMTA<string>(this.GetApplicationManifestFileHelper, "BuildLayoutSpecification.GetApplicationManifestFile");
        }

        public string GetServiceManifestFile(string serviceManifestName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServiceManifestFileHelper(serviceManifestName), "BuildLayoutSpecification.GetServiceManifestFile");
        }

        public string GetServiceManifestChecksumFile(string serviceManifestName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServiceManifestChecksumFileHelper(serviceManifestName), "BuildLayoutSpecification.GetServiceManifestChecksumFile");
        }

        public string GetCodePackageFolder(
            string serviceManifestName,
            string codePackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageFolderHelper(serviceManifestName, codePackageName), "BuildLayoutSpecification.GetCodePackagePackage");
        }

        public string GetCodePackageChecksumFile(
            string serviceManifestName,
            string codePackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageChecksumFileHelper(serviceManifestName, codePackageName), "BuildLayoutSpecification.GetCodePackageChecksumFile");
        }

        public string GetConfigPackageFolder(
            string serviceManifestName,
            string configPackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetConfigPackageFolderHelper(serviceManifestName, configPackageName), "BuildLayoutSpecification.GetConfigPackageFolder");
        }

        public string GetConfigPackageChecksumFile(
            string serviceManifestName,
            string configPackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetConfigPackageChecksumFileHelper(serviceManifestName, configPackageName), "BuildLayoutSpecification.GetConfigPackageChecksumFile");
        }

        public string GetDataPackageFolder(
            string serviceManifestName,
            string dataPackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetDataPackageFolderHelper(serviceManifestName, dataPackageName), "BuildLayoutSpecification.GetDataPackage");
        }

        public string GetDataPackageChecksumFile(
            string serviceManifestName,
            string dataPackageName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetDataPackageChecksumFileHelper(serviceManifestName, dataPackageName), "BuildLayoutSpecification.GetDataPackageChecksumFile");
        }

        public string GetSettingsFile(string configPackageFolder)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetSettingsFileHelper(configPackageFolder), "BuildLayoutSpecification.GetSettingsFile");
        }

        public string GetSubPackageArchiveFile(
            string packageFolder)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetSubPackageArchiveFileHelper(packageFolder), "BuildLayoutSpecification.GetSubPackageArchiveFile");
        }

        public string GetChecksumFile(
            string fileOrDirectoryName)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetChecksumFileHelper(fileOrDirectoryName), "BuildLayoutSpecification.GetChecksumFile");
        }

        #region Interop Code

        private static BuildLayoutSpecification CreateHelper()
        {
            Guid iid = typeof(NativeImageModel.IFabricBuildLayoutSpecification2).GetTypeInfo().GUID;
            NativeImageModel.IFabricBuildLayoutSpecification2 newNativeBuildLayout = null;

            newNativeBuildLayout = (NativeImageModel.IFabricBuildLayoutSpecification2)NativeImageModel.FabricCreateBuildLayoutSpecification(ref iid);

            var runtime = new BuildLayoutSpecification(newNativeBuildLayout);
            return runtime;
        }

        private void SetRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeBuildLayout.SetRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetRootHelper()
        {
            return StringResult.FromNative(this.nativeBuildLayout.GetRoot());
        }

        private string GetApplicationManifestFileHelper()
        {
            return StringResult.FromNative(this.nativeBuildLayout.GetApplicationManifestFile());
        }

        private string GetServiceManifestFileHelper(string serviceManifestName)
        {
            using (var pin = new PinBlittable(serviceManifestName))
            {
                return StringResult.FromNative(
                    this.nativeBuildLayout.GetServiceManifestFile(pin.AddrOfPinnedObject()));
            }
        }

        private string GetServiceManifestChecksumFileHelper(string serviceManifestName)
        {
            using (var pin = new PinBlittable(serviceManifestName))
            {
                return StringResult.FromNative(
                    this.nativeBuildLayout.GetServiceManifestChecksumFile(pin.AddrOfPinnedObject()));
            }
        }

        private string GetCodePackageFolderHelper(
            string serviceManifestName,
            string codePackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetCodePackageFolder(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(codePackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodePackageChecksumFileHelper(
            string serviceManifestName,
            string codePackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetCodePackageChecksumFile(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(codePackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetConfigPackageFolderHelper(
            string serviceManifestName,
            string configPackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetConfigPackageFolder(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(configPackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetConfigPackageChecksumFileHelper(
            string serviceManifestName,
            string configPackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetConfigPackageChecksumFile(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(configPackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetDataPackageFolderHelper(
            string serviceManifestName,
            string dataPackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetDataPackageFolder(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(dataPackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetDataPackageChecksumFileHelper(
            string serviceManifestName,
            string dataPackageName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetDataPackageChecksumFile(
                    pin.AddBlittable(serviceManifestName),
                    pin.AddBlittable(dataPackageName));

                return StringResult.FromNative(result);
            }
        }

        private string GetSettingsFileHelper(string configPackageFolder)
        {
            using (var pin = new PinBlittable(configPackageFolder))
            {
                return StringResult.FromNative(
                    this.nativeBuildLayout.GetSettingsFile(pin.AddrOfPinnedObject()));
            }
        }

        private string GetSubPackageArchiveFileHelper(
            string packageFolder)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetSubPackageArchiveFile(
                    pin.AddBlittable(packageFolder));

                return StringResult.FromNative(result);
            }
        }

        private string GetChecksumFileHelper(
            string fileOrDirectoryName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeBuildLayout.GetChecksumFile(
                    pin.AddBlittable(fileOrDirectoryName));

                return StringResult.FromNative(result);
            }
        }

        #endregion
    }
}