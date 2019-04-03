// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Interop;
    using BOOLEAN = System.SByte;

    internal class FabricEnvironment
    {
#if DotNetCoreClrLinux
        internal static LinuxPackageManagerType GetLinuxPackageManagerType()
        {
            return Utility.WrapNativeSyncInvokeInMTA(() => FabricEnvironment.GetLinuxPackageManagerTypeHelper(), "FabricEnvironment.GetLinuxPackageManagerType");
        }
#endif

        public static string GetRoot()
        {
            return GetRoot(null);
        }

        public static string GetRoot(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetRootHelper(machineName), "FabricEnvironment.GetRoot");
        }

        public static string GetBinRoot()
        {
            return GetBinRoot(null);
        }

        public static string GetBinRoot(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetBinRootHelper(machineName), "FabricEnvironment.GetBinRoot");
        }

        public static string GetCodePath()
        {
            return GetCodePath(null);
        }

        public static string GetCodePath(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetCodePathHelper(machineName), "FabricEnvironment.GetCodePath");
        }

        public static string GetDataRoot()
        {
            return GetDataRoot(null);
        }

        public static string GetDataRoot(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetDataRootHelper(machineName), "FabricEnvironment.GetDataRoot");
        }

        public static string GetLogRoot()
        {
            return GetLogRoot(null);
        }

        public static string GetLogRoot(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetLogRootHelper(machineName), "FabricEnvironment.GetLogRoot");
        }

        public static void SetFabricRoot(string root)
        {
            SetFabricRoot(root, null);
        }

        public static void SetFabricRoot(string root, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetFabricRootHelper(root, machineName), "FabricEnvironment.SetFabricRoot");
        }

        public static void SetFabricBinRoot(string binRoot)
        {
            SetFabricBinRoot(binRoot, null);
        }

        public static void SetFabricBinRoot(string binRoot, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetFabricBinRootHelper(binRoot, machineName), "FabricEnvironment.SetBinRoot");
        }

        public static void SetFabricCodePath(string codePath)
        {
            SetFabricCodePath(codePath, null);
        }

        public static void SetFabricCodePath(string codePath, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetFabricCodePathHelper(codePath, machineName), "FabricEnvironment.SetFabricCodePath");
        }

        public static void SetDataRoot(string dataRoot)
        {
            SetDataRoot(dataRoot, null);
        }

        public static void SetDataRoot(string dataRoot, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetDataRootHelper(dataRoot, machineName), "FabricEnvironment.SetDataRoot");
        }

        public static void SetLogRoot(string logRoot)
        {
            SetLogRoot(logRoot, null);
        }

        public static void SetLogRoot(string logRoot, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetLogRootHelper(logRoot, machineName), "FabricEnvironment.SetLogRoot");
        }

        public static void SetEnableCircularTraceSession(bool enableCircularTraceSession)
        {
            SetEnableCircularTraceSession(enableCircularTraceSession, null);
        }

        public static void SetEnableCircularTraceSession(bool enableCircularTraceSession, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetEnableCircularTraceSessionHelper(enableCircularTraceSession, machineName), "FabricEnvironment.SetEnableCircularTraceSession");
        }

        public static void SetEnableUnsupportedPreviewFeatures(bool enableUnsupportedPreviewFeatures, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetEnableUnsupportedPreviewFeaturesHelper(enableUnsupportedPreviewFeatures, machineName), "FabricEnvironment.SetEnableUnsupportedPreviewFeatures");
        }

        public static void SetIsSFVolumeDiskServiceEnabled(bool isSFVolumeDiskServiceEnabled, string machineName)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetIsSFVolumeDiskServiceEnabledHelper(isSFVolumeDiskServiceEnabled, machineName), "FabricEnvironment.SetIsSFVolumeDiskServiceEnabled");
        }

        public static bool GetEnableCircularTraceSession()
        {
            return GetEnableCircularTraceSession(null);
        }

        public static bool GetEnableCircularTraceSession(string machineName)
        {
            return Utility.WrapNativeSyncInvoke(() => FabricEnvironment.GetEnableCircularTraceSessionHelper(machineName), "FabricEnvironment.GetEnableCircularTraceSession");
        }

#if DotNetCoreClrLinux
        public static void SetSfInstalledMoby(string fileContents)
        {
            Utility.WrapNativeSyncInvoke(() => FabricEnvironment.SetSfInstalledMobyHelper(fileContents), "FabricEnvironment.SetSfInstalledMoby");
        }

        private static LinuxPackageManagerType GetLinuxPackageManagerTypeHelper()
        {
            using (var pin = new PinCollection())
            {
                Int32 packageManagerType = 0;
                NativeCommon.GetLinuxPackageManagerType(out packageManagerType);

                return (LinuxPackageManagerType)packageManagerType;
            }
        }
#endif

        private static string GetRootHelper(string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return StringResult.FromNative(NativeCommon.FabricGetRoot());
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    return StringResult.FromNative(NativeCommon.FabricGetRoot2(pin.AddBlittable(machineName)));
                }
            }
        }

        private static string GetBinRootHelper(string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return StringResult.FromNative(NativeCommon.FabricGetBinRoot());
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    return StringResult.FromNative(NativeCommon.FabricGetBinRoot2(pin.AddBlittable(machineName)));
                }
            }
        }

        private static string GetCodePathHelper(string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return StringResult.FromNative(NativeCommon.FabricGetCodePath());
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    return StringResult.FromNative(NativeCommon.FabricGetCodePath2(pin.AddBlittable(machineName)));
                }
            }
        }

        private static string GetDataRootHelper(string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return StringResult.FromNative(NativeCommon.FabricGetDataRoot());
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    return StringResult.FromNative(NativeCommon.FabricGetDataRoot2(pin.AddBlittable(machineName)));
                }
            }
        }

        private static string GetLogRootHelper(string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return StringResult.FromNative(NativeCommon.FabricGetLogRoot());
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    return StringResult.FromNative(NativeCommon.FabricGetLogRoot2(pin.AddBlittable(machineName)));
                }
            }
        }

        private static void SetFabricRootHelper(string root, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetRoot(pin.AddBlittable(root));
                }
                else
                {
                    NativeCommon.FabricSetRoot2(pin.AddBlittable(root), pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetFabricBinRootHelper(string binRoot, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetBinRoot(pin.AddBlittable(binRoot));
                }
                else
                {
                    NativeCommon.FabricSetBinRoot2(pin.AddBlittable(binRoot), pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetFabricCodePathHelper(string codePath, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetCodePath(pin.AddBlittable(codePath));
                }
                else
                {
                    NativeCommon.FabricSetCodePath2(pin.AddBlittable(codePath), pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetDataRootHelper(string dataRoot, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetDataRoot(pin.AddBlittable(dataRoot));
                }
                else
                {
                    NativeCommon.FabricSetDataRoot2(pin.AddBlittable(dataRoot), pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetLogRootHelper(string logRoot, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetLogRoot(pin.AddBlittable(logRoot));
                }
                else
                {
                    NativeCommon.FabricSetLogRoot2(pin.AddBlittable(logRoot), pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetEnableCircularTraceSessionHelper(bool enableCircularTraceSession, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetEnableCircularTraceSession(enableCircularTraceSession);
                }
                else
                {
                    NativeCommon.FabricSetEnableCircularTraceSession2(enableCircularTraceSession, pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetEnableUnsupportedPreviewFeaturesHelper(bool enableUnsupportedPreviewFeatures, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetEnableUnsupportedPreviewFeatures(enableUnsupportedPreviewFeatures);
                }
                else
                {
                    NativeCommon.FabricSetEnableUnsupportedPreviewFeatures2(enableUnsupportedPreviewFeatures, pin.AddBlittable(machineName));
                }
            }
        }

        private static void SetIsSFVolumeDiskServiceEnabledHelper(bool isSFVolumeDiskServiceEnabled, string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    NativeCommon.FabricSetIsSFVolumeDiskServiceEnabled(isSFVolumeDiskServiceEnabled);
                }
                else
                {
                    NativeCommon.FabricSetIsSFVolumeDiskServiceEnabled2(isSFVolumeDiskServiceEnabled, pin.AddBlittable(machineName));
                }
            }
        }

#if DotNetCoreClrLinux
        private static void SetSfInstalledMobyHelper(string fileContents)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricSetSfInstalledMoby(pin.AddBlittable(fileContents));
            }
        }
#endif

        private static bool GetEnableCircularTraceSessionHelper(string machineName)
        {
            using (var pin = new PinCollection())
            {
                if (string.IsNullOrEmpty(machineName))
                {
                    return NativeCommon.FabricGetEnableCircularTraceSession();
                }
                else
                {
                    return NativeCommon.FabricGetEnableCircularTraceSession2(pin.AddBlittable(machineName));
                }
            }
        }
    }
}