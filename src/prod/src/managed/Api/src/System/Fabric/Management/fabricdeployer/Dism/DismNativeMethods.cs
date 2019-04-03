// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{    
    using System.Runtime.InteropServices;

    internal static class DismNativeMethods
    {
        public const int ERROR_SUCCESS = 0;
        private const string DismAssembly = "DismApi";

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismInitialize(DismLogLevel logLevel, string logFilePath, string scratchDirectory);

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismOpenSession(string imagePath, string windowsDirectory, string systemDrive, out DismSession session);

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismGetFeatureInfo(DismSession session, string featureName, string identifier, DismPackageIdentifier packageIdentifier, out IntPtr pFeatureInfo);

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismCloseSession(IntPtr pSession);

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismDelete(IntPtr pDismStructure);

        [DllImport(DismAssembly, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Error)]
        public static extern int DismShutdown();
    }
}

#endif