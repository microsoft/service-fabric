// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Reflection;

    internal class RunLayoutSpecification
    {
        private NativeImageModel.IFabricRunLayoutSpecification nativeRunLayout;

        private RunLayoutSpecification(NativeImageModel.IFabricRunLayoutSpecification nativeRunLayout)
        {
            this.nativeRunLayout = nativeRunLayout;
        }

        public static RunLayoutSpecification Create()
        {
            return Utility.WrapNativeSyncInvokeInMTA<RunLayoutSpecification>(RunLayoutSpecification.CreateHelper, "RunLayoutSpecification.Create");
        }

        public void SetRoot(string value)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.SetRootHelper(value), "StoreLayoutSpecification.SetRoot");
        }

        public string GetApplicationLogFolder(
            string applicationId)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetApplicationLogFolderHelper(applicationId), "RunLayoutSpecification.GetApplicationLogFolder");
        }

        public string GetApplicationPackageFile(
            string applicationId,
            string applicationRolloutVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetApplicationPackageFileHelper(applicationId, applicationRolloutVersion), "RunLayoutSpecification.GetApplicationPackageFile");
        }

        public string GetServicePackageFile(
            string applicationId,
            string servicePackageName,
            string servicePackageRolloutVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetServicePackageFileHelper(applicationId, servicePackageName, servicePackageRolloutVersion), "RunLayoutSpecification.GetServicePackageFile");
        }

        public string GetDataPackageFolder(
            string applicationId, 
            string servicePackageName, 
            string dataPackageName, 
            string dataPackageVersion,
            bool isSharedPackage)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetDataPackageFolderHelper(applicationId, servicePackageName, dataPackageName, dataPackageVersion, isSharedPackage), "RunLayoutSpecification.GetDataPackageFolder");
        }

        public string GetCodePackageFolder(
            string applicationId,
            string servicePackageName,
            string codePackageName,
            string codePackageVersion,
            bool isSharedPackage)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetCodePackageFolderHelper(applicationId, servicePackageName, codePackageName, codePackageVersion, isSharedPackage), "RunLayoutSpecification.GetCodePackageFolder");
        }

        #region Interop code

        private static RunLayoutSpecification CreateHelper()
        {
            Guid iid = typeof(NativeImageModel.IFabricRunLayoutSpecification).GetTypeInfo().GUID;
            NativeImageModel.IFabricRunLayoutSpecification newNativeStoreLayout = null;

            newNativeStoreLayout = NativeImageModel.FabricCreateRunLayoutSpecification(ref iid);

            return new RunLayoutSpecification(newNativeStoreLayout);
        }

        private void SetRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeRunLayout.SetRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetApplicationLogFolderHelper(
            string applicationId)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeRunLayout.GetApplicationLogFolder(
                    pin.AddBlittable(applicationId));

                return StringResult.FromNative(result);
            }
        }

        private string GetApplicationPackageFileHelper(
            string applicationId,
            string applicationRolloutVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeRunLayout.GetApplicationPackageFile(
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(applicationRolloutVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetServicePackageFileHelper(
            string applicationId,
            string servicePackageName,
            string servicePackageRolloutVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeRunLayout.GetServicePackageFile(
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(servicePackageName),
                    pin.AddBlittable(servicePackageRolloutVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetDataPackageFolderHelper(
            string applicationId,
            string servicePackageName,
            string dataPackageName,
            string dataPackageVersion,
            bool isSharedPackage)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeRunLayout.GetDataPackageFolder(
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(servicePackageName),
                    pin.AddBlittable(dataPackageName),
                    pin.AddBlittable(dataPackageVersion),
                    NativeTypes.ToBOOLEAN(isSharedPackage));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodePackageFolderHelper(
            string applicationId,
            string servicePackageName,
            string codePackageName,
            string codePackageVersion,
            bool isSharedPackage)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeRunLayout.GetCodePackageFolder(
                    pin.AddBlittable(applicationId),
                    pin.AddBlittable(servicePackageName),
                    pin.AddBlittable(codePackageName),
                    pin.AddBlittable(codePackageVersion),
                    NativeTypes.ToBOOLEAN(isSharedPackage));

                return StringResult.FromNative(result);
            }
        }

        #endregion
    }
}