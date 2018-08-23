// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Reflection;

    internal class WinFabStoreLayoutSpecification
    {
        private NativeImageModel.IFabricWinFabStoreLayoutSpecification nativeWinFabStoreLayout;

        private WinFabStoreLayoutSpecification(NativeImageModel.IFabricWinFabStoreLayoutSpecification nativeStoreLayout)
        {
            this.nativeWinFabStoreLayout = nativeStoreLayout;
        }

        public static WinFabStoreLayoutSpecification Create()
        {
            return Utility.WrapNativeSyncInvokeInMTA<WinFabStoreLayoutSpecification>(WinFabStoreLayoutSpecification.CreateHelper, "WinFabStoreLayoutSpecification.Create");
        }

        public void SetRoot(string value)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.SetRootHelper(value), "WinFabStoreLayoutSpecification.SetRoot");
        }

        public string GetRoot()
        {
            return Utility.WrapNativeSyncInvokeInMTA<string>(this.GetRootHelper, "WinFabStoreLayoutSpecification.GetRoot");
        }

        public string GetPatchFile(string version)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetPatchFileHelper(version), "WinFabStoreLayoutSpecification.GetPatchFile");
        }

        public string GetCabPatchFile(string version)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCabPatchFileHelper(version), "WinFabStoreLayoutSpecification.GetCabPatchFile");
        }

        public string GetCodePackageFolder(string version)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageFolderHelper(version), "WinFabStoreLayoutSpecification.GetCodePackageFolder");
        }

        public string GetClusterManifestFile(string clusterManifestVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetClusterManifestFileHelper(clusterManifestVersion), "WinFabStoreLayoutSpecification.GetClusterManifestFileHelper");
        }       

        #region Interop code

        private static WinFabStoreLayoutSpecification CreateHelper()
        {
            Guid iid = typeof(NativeImageModel.IFabricWinFabStoreLayoutSpecification).GetTypeInfo().GUID;
            NativeImageModel.IFabricWinFabStoreLayoutSpecification newNativeWinFabStoreLayout = null;

            newNativeWinFabStoreLayout = NativeImageModel.FabricCreateWinFabStoreLayoutSpecification(ref iid);

            return new WinFabStoreLayoutSpecification(newNativeWinFabStoreLayout);
        }

        private void SetRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeWinFabStoreLayout.SetRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetRootHelper()
        {
            return StringResult.FromNative(this.nativeWinFabStoreLayout.GetRoot());
        }

        private string GetPatchFileHelper(string version)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeWinFabStoreLayout.GetPatchFile(
                    pin.AddBlittable(version));

                return StringResult.FromNative(result);
            }
        }


        private string GetCabPatchFileHelper(string version)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeWinFabStoreLayout.GetCabPatchFile(
                    pin.AddBlittable(version));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodePackageFolderHelper(string version)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeWinFabStoreLayout.GetCodePackageFolder(
                    pin.AddBlittable(version));

                return StringResult.FromNative(result);
            }
        }

        private string GetClusterManifestFileHelper(string clusterManifestVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeWinFabStoreLayout.GetClusterManifestFile(
                    pin.AddBlittable(clusterManifestVersion));

                return StringResult.FromNative(result);
            }
        }
        
        #endregion
    }
}