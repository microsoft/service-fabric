// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Reflection;

    internal class FabricDeploymentSpecification
    {
        private NativeImageModel.IFabricDeploymentSpecification nativeFabricDeployment;

        private FabricDeploymentSpecification(NativeImageModel.IFabricDeploymentSpecification nativeFabricDeployment)
        {
            this.nativeFabricDeployment = nativeFabricDeployment;
        }

        public static FabricDeploymentSpecification Create()
        {
            return Utility.WrapNativeSyncInvoke<FabricDeploymentSpecification>(FabricDeploymentSpecification.CreateHelper, "FabricDeploymentSpecification.Create");
        }

        public void SetDataRoot(string value)
        {
            Utility.WrapNativeSyncInvoke(() => this.SetDataRootHelper(value), "FabricDeploymentSpecification.SetDataRoot");
        }

        public string GetDataRoot()
        {
            return Utility.WrapNativeSyncInvoke<string>(this.GetDataRootHelper, "FabricDeploymentSpecification.GetDataRoot");
        }

        public void SetLogRoot(string value)
        {
            Utility.WrapNativeSyncInvoke(() => this.SetLogRootHelper(value), "FabricDeploymentSpecification.SetLogRoot");
        }

        public string GetLogRoot()
        {
            return Utility.WrapNativeSyncInvoke<string>(this.GetLogRootHelper, "FabricDeploymentSpecification.GetLogRoot");
        }

        public string GetLogFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetLogFolderHelper(), "FabricDeploymentSpecification.GetLogFolder");
        }

        public string GetTracesFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetTracesFolderHelper(), "FabricDeploymentSpecification.GetTracesFolder");
        }

        public string GetCrashDumpsFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetCrashDumpsFolderHelper(), "FabricDeploymentSpecification.GetCrashDumpsFolder");
        }

        public string GetApplicationCrashDumpsFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetApplicationCrashDumpsFolderHelper(), "FabricDeploymentSpecification.GetApplicationCrashDumpsFolder");
        }

        public string GetAppInstanceDataFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetAppInstanceDataFolderHelper(), "FabricDeploymentSpecification.GetAppInstanceDataFolder");
        }

        public string GetPerformanceCountersBinaryFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetPerformanceCountersBinaryFolderHelper(), "FabricDeploymentSpecification.GetPerformanceCountersBinaryFolder");
        }

        public string GetTargetInformationFile()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetTargetInformationFileHelper(), "FabricDeploymentSpecification.GetTargetInformationFile");
        }

        public string GetNodeFolder(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetNodeFolderHelper(nodeName), "FabricDeploymentSpecification.GetNodeFolder");
        }

        public string GetFabricFolder(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetFabricFolderHelper(nodeName), "FabricDeploymentSpecification.GetFabricFolder");
        }

        public string GetCurrentClusterManifestFile(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetCurrentClusterManifestFileHelper(nodeName), "FabricDeploymentSpecification.GetCurrentClusterManifestFile");
        }

        public string GetVersionedClusterManifestFile(string nodeName, string version)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetVersionedClusterManifestFileHelper(nodeName, version), "FabricDeploymentSpecification.GetVersionedClusterManifestFile");
        }

        public string GetInstallerScriptFile(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetInstallerScriptFileHelper(nodeName), "FabricDeploymentSpecification.GetInstallerScriptFile");
        }

        public string GetInstallerLogFile(string nodeName, string codeVersion)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetInstallerLogFileHelper(nodeName, codeVersion), "FabricDeploymentSpecification.GetInstallerLogFile");
        }

        public string GetInfrastructureManfiestFile(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetInfrastructureManfiestFileHelper(nodeName), "FabricDeploymentSpecification.GetInfrastructureManfiestFile");
        }

        public string GetConfigurationDeploymentFolder(string nodeName, string configVersion)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetConfigurationDeploymentFolderHelper(nodeName, configVersion), "FabricDeploymentSpecification.GetConfigurationDeploymentFolder");
        }

        public string GetDataDeploymentFolder(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetDataDeploymentFolderHelper(nodeName), "FabricDeploymentSpecification.GetDataDeploymentFolder");
        }

        public string GetCodeDeploymentFolder(string nodeName, string service)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetCodeDeploymentFolderHelper(nodeName, service), "FabricDeploymentSpecification.GetCodeDeploymentFolder");
        }

        public string GetInstalledBinaryFolder(string installationFolder, string service)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetInstalledBinaryFolderHelper(installationFolder, service), "FabricDeploymentSpecification.GetInstalledBinaryFolder");
        }

        public string GetWorkFolder(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetWorkFolderHelper(nodeName), "FabricDeploymentSpecification.GetWorkFolder");
        }

        public string GetCurrentFabricPackageFile(string nodeName)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetCurrentFabricPackageFileHelper(nodeName), "FabricDeploymentSpecification.GetCurrentFabricPackageFile");
        }

        public string GetVersionedFabricPackageFile(string nodeName, string version)
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetVersionedFabricPackageFileHelper(nodeName, version), "FabricDeploymentSpecification.GetVersionedFabricPackageFile");
        }

        public string GetQueryTraceFolder()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetQueryTraceFolderHelper(), "FabricDeploymentSpecification.GetQueryTraceFolder");
        }

        public void SetEnableCircularTraceSession(bool value)
        {
            Utility.WrapNativeSyncInvoke(
                () => this.SetEnableCircularTraceSessionHelper(value),
                "FabricDeploymentSpecification.SetEnableCircularTraceSession");
        }

        public bool GetEnableCircularTraceSession()
        {
            return Utility.WrapNativeSyncInvoke(
                () => this.GetEnableCircularTraceSessionHelper(),
                "FabricDeploymentSpecification.GetEnableCircularTraceSession");
        }
        
        #region Interop code

        private static FabricDeploymentSpecification CreateHelper()
        {
            Guid iid = typeof(NativeImageModel.IFabricDeploymentSpecification).GetTypeInfo().GUID;
            NativeImageModel.IFabricDeploymentSpecification newNativeFabricDeployment = null;

            newNativeFabricDeployment = NativeImageModel.FabricCreateFabricDeploymentSpecification(ref iid);

            return new FabricDeploymentSpecification(newNativeFabricDeployment);
        }

        private void SetDataRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeFabricDeployment.SetDataRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetDataRootHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetDataRoot());
        }

        private void SetLogRootHelper(string value)
        {
            using (var pin = new PinBlittable(value))
            {
                this.nativeFabricDeployment.SetLogRoot(pin.AddrOfPinnedObject());
            }
        }

        private string GetLogRootHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetLogRoot());
        }

        private string GetLogFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetLogFolder());
        }

        private string GetTracesFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetTracesFolder());
        }

        private string GetCrashDumpsFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetCrashDumpsFolder());
        }

        private string GetApplicationCrashDumpsFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetApplicationCrashDumpsFolder());
        }

        private string GetAppInstanceDataFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetAppInstanceDataFolder());
        }

        private string GetPerformanceCountersBinaryFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetPerformanceCountersBinaryFolder());
        }

        private string GetTargetInformationFileHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetTargetInformationFile());
        }

        private string GetNodeFolderHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetNodeFolder(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetFabricFolderHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetFabricFolder(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetCurrentClusterManifestFileHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetCurrentClusterManifestFile(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetVersionedClusterManifestFileHelper(string nodeName, string version)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetVersionedClusterManifestFile(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(version));

                return StringResult.FromNative(result);
            }
        }

        private string GetInstallerScriptFileHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetInstallerScriptFile(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetInstallerLogFileHelper(string nodeName, string codeVersion)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetInstallerLogFile(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(codeVersion));

                return StringResult.FromNative(result);
            }
        }

        private string GetInfrastructureManfiestFileHelper(string nodeName) 
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetInfrastructureManfiestFile(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetConfigurationDeploymentFolderHelper(string nodeName, string configVersion) 
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetConfigurationDeploymentFolder(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(configVersion));
                return StringResult.FromNative(result);
            }
        }

        private string GetDataDeploymentFolderHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetDataDeploymentFolder(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetCodeDeploymentFolderHelper(string nodeName, string service)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetCodeDeploymentFolder(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(service));

                return StringResult.FromNative(result);
            }
        }

        private string GetInstalledBinaryFolderHelper(string installationFolder, string service)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetInstalledBinaryFolder(
                    pin.AddBlittable(installationFolder),
                    pin.AddBlittable(service));

                return StringResult.FromNative(result);
            }
        }

        private string GetWorkFolderHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetWorkFolder(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetCurrentFabricPackageFileHelper(string nodeName)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetCurrentFabricPackageFile(
                    pin.AddBlittable(nodeName));

                return StringResult.FromNative(result);
            }
        }

        private string GetVersionedFabricPackageFileHelper(string nodeName, string version)
        {
            using (var pin = new PinCollection())
            {
                var result = this.nativeFabricDeployment.GetVersionedFabricPackageFile(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(version));

                return StringResult.FromNative(result);
            }
        }

        private string GetQueryTraceFolderHelper()
        {
            return StringResult.FromNative(this.nativeFabricDeployment.GetQueryTraceFolder());
        }

        private bool GetEnableCircularTraceSessionHelper()
        {
            return this.nativeFabricDeployment.GetEnableCircularTraceSession();
        }

        private void SetEnableCircularTraceSessionHelper(bool value)
        {
            this.nativeFabricDeployment.SetEnableCircularTraceSession(value);
        }
        #endregion
    }
}