// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.ImageModel
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    using BOOLEAN = System.SByte;

    internal static class NativeImageModel
    {
        [ComImport]
        [Guid("A5344964-3542-41BD-A271-F2F21AC541A8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricBuildLayoutSpecification
        {
            void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationManifestFile();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestFile(
                [In] IntPtr serviceManifestName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestChecksumFile(
                [In] IntPtr serviceManifestName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigPackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigPackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataPackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataPackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetSettingsFile(
                [In] IntPtr configPackageFolder);
        }

        [ComImport]
        [Guid("93F7CA0F-8BE0-4601-92A9-AD864B4A798D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricBuildLayoutSpecification2 : IFabricBuildLayoutSpecification
        {
            new void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetApplicationManifestFile();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestFile(
                [In] IntPtr serviceManifestName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestChecksumFile(
                [In] IntPtr serviceManifestName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetCodePackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetConfigPackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetConfigPackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetDataPackageFolder(
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetDataPackageChecksumFile(
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetSettingsFile(
                [In] IntPtr configPackageFolder);

            // IFabricBuildLayoutSpecification2

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetSubPackageArchiveFile(
                [In] IntPtr packageFolder);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetChecksumFile(
                [In] IntPtr packageFolder);
        }

        [ComImport]
        [Guid("53636FDE-84CA-4657-9927-1D22DC37297D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricRunLayoutSpecification
        {
            void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationFolder(
                [In] IntPtr applicationId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationWorkFolder(
                [In] IntPtr applicationId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationLogFolder(
                [In] IntPtr applicationId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationTempFolder(
                [In] IntPtr applicationId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationPackageFile(
                [In] IntPtr applicationId,
                [In] IntPtr applicationRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServicePackageFile(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr servicePackageRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCurrentServicePackageFile(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetEndpointDescriptionsFile(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestFile(
                [In] IntPtr applicationId,
                [In] IntPtr serviceManifestName,
                [In] IntPtr serviceManifestVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr codePackageName,
                [In] IntPtr codePackageVersion,
                [In] BOOLEAN isSharedPackage);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigPackageFolder(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr configPackageName,
                [In] IntPtr configPackageVersion,
                [In] BOOLEAN isSharedPackage);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataPackageFolder(
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr dataPackageName,
                [In] IntPtr dataPackageVersion,
                [In] BOOLEAN isSharedPackage);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetSettingsFile(
                [In] IntPtr configPackageFolder);
        }

        [ComImport]
        [Guid("A4E94CF4-A47B-4767-853F-CBD5914641D6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricStoreLayoutSpecification
        {
            void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationManifestFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationInstanceFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr applicationInstanceVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationPackageFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr applicationRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServicePackageFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr servicePackageRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr serviceManifestVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr serviceManifestVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName,
                [In] IntPtr codePackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName,
                [In] IntPtr codePackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigPackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName,
                [In] IntPtr configPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigPackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName,
                [In] IntPtr configPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataPackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName,
                [In] IntPtr dataPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataPackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName,
                [In] IntPtr dataPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetSettingsFile(
                [In] IntPtr configPackageFolder);
        }

        [ComImport]
        [Guid("E4A20405-2CEF-4DA0-BD6C-B466A5845D48")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricStoreLayoutSpecification2 : IFabricStoreLayoutSpecification
        {
            new void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetApplicationManifestFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetApplicationInstanceFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr applicationInstanceVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetApplicationPackageFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr applicationRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServicePackageFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationId,
                [In] IntPtr servicePackageName,
                [In] IntPtr servicePackageRolloutVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr serviceManifestVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr serviceManifestVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName,
                [In] IntPtr codePackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetCodePackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr codePackageName,
                [In] IntPtr codePackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetConfigPackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName,
                [In] IntPtr configPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetConfigPackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr configPackageName,
                [In] IntPtr configPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetDataPackageFolder(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName,
                [In] IntPtr dataPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetDataPackageChecksumFile(
                [In] IntPtr applicationTypeName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr dataPackageName,
                [In] IntPtr dataPackageVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetSettingsFile(
                [In] IntPtr configPackageFolder);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetSubPackageArchiveFile(
                [In] IntPtr packageFolder);
        }

        [ComImport]
        [Guid("D07D4CB7-0E31-4930-8AC1-DC1A1062241D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricWinFabStoreLayoutSpecification
        {
            void SetRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetPatchFile(
                [In] IntPtr version);

			[return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCabPatchFile(
                [In] IntPtr version);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodePackageFolder(
                [In] IntPtr version);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetClusterManifestFile(
                [In] IntPtr clusterManifestVersion);
        }

        [ComImport]
        [Guid("5C36B827-0297-4355-B591-7EB7211548AE")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricDeploymentSpecification
        {
            void SetDataRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataRoot();

            void SetLogRoot([In] IntPtr value);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetLogRoot();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetLogFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetTracesFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCrashDumpsFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetApplicationCrashDumpsFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetAppInstanceDataFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetPerformanceCountersBinaryFolder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetTargetInformationFile();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetNodeFolder(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetFabricFolder(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCurrentClusterManifestFile(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetVersionedClusterManifestFile(
                [In] IntPtr nodeName,
                [In] IntPtr version);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetInstallerScriptFile(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetInstallerLogFile(
                [In] IntPtr nodeName,
                [In] IntPtr codeVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetInfrastructureManfiestFile(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetConfigurationDeploymentFolder(
                [In] IntPtr nodeName,
                [In] IntPtr configVersion);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDataDeploymentFolder(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCodeDeploymentFolder(
                [In] IntPtr nodeName,
                [In] IntPtr service);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetInstalledBinaryFolder(
                [In] IntPtr installationFolder, 
                [In] IntPtr service);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetWorkFolder([In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetCurrentFabricPackageFile(
                [In] IntPtr nodeName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetVersionedFabricPackageFile(
                [In] IntPtr nodeName,
                [In] IntPtr version);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetQueryTraceFolder();

            [return: MarshalAs(UnmanagedType.I1)]
            bool GetEnableCircularTraceSession();

            void SetEnableCircularTraceSession([In][MarshalAs(UnmanagedType.I1)] bool value);
        }

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricBuildLayoutSpecification FabricCreateBuildLayoutSpecification(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricStoreLayoutSpecification FabricCreateStoreLayoutSpecification(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricRunLayoutSpecification FabricCreateRunLayoutSpecification(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricWinFabStoreLayoutSpecification FabricCreateWinFabStoreLayoutSpecification(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricDeploymentSpecification FabricCreateFabricDeploymentSpecification(
            ref Guid riid);
    }
}