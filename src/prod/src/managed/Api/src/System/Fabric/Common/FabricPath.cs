// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Interop;

    internal class FabricPath
    {
        internal static string GetUncPath(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvoke(() => FabricPath.GetUncPathHelper(path), "FabricPath.GetUncPath");
        }

        internal static string GetDirectoryName(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvoke(() => FabricPath.GetDirectoryNameHelper(path), "FabricPath.GetDirectoryName");
        }

        internal static string GetFullPath(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvoke(() => FabricPath.GetFullPathHelper(path), "FabricPath.GetFullPath");
        }

        private static string GetUncPathHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricGetUncPath(pin.AddBlittable(path)));
            }
        }

        private static string GetDirectoryNameHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricGetDirectoryName(pin.AddBlittable(path)));
            }
        }

        private static string GetFullPathHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricGetFullPath(pin.AddBlittable(path)));
            }
        }
    }
}