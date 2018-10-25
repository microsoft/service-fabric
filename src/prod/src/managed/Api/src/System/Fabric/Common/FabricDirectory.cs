// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.IO;
    using System.Fabric.Interop;
    using BOOLEAN = System.SByte;

    internal class FabricDirectory
    {
        internal static void CreateDirectory(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => CreateHelper(path), "FabricDirectory.CreateDirectory");
        }

        internal static string[] GetDirectories(string path)
        {
            return GetDirectories(path, "*", SearchOption.TopDirectoryOnly);
        }

        internal static string[] GetDirectories(string path, string pattern)
        {
            return GetDirectories(path, pattern, SearchOption.TopDirectoryOnly);
        }

        internal static string[] GetDirectories(string path, string pattern, SearchOption option)
        {
            return GetDirectories(path, pattern, true, option);
        }

        internal static string[] GetDirectories(string path, string pattern, bool getFullPath, SearchOption option)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => GetDirectoriesHelper(path, pattern, getFullPath, option), "FabricDirectory.GetDirectories");
        }

        internal static string[] GetFiles(string path)
        {
            return GetFiles(path, "*");
        }

        internal static string[] GetFiles(string path, string pattern)
        {
            return GetFiles(path, pattern, true, SearchOption.TopDirectoryOnly);
        }

        internal static string[] GetFiles(string path, string pattern, SearchOption option)
        {
            return GetFiles(path, pattern, true, option);
        }

        internal static string[] GetFiles(string path, string pattern, bool getFullPath, SearchOption option)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => GetFilesHelper(path, pattern, getFullPath, option), "FabricDirectory.GetFiles");
        }

        internal static void Copy(string src, string des, bool overwrite)
        {
            Requires.Argument("src", src).NotNullOrEmpty();
            Requires.Argument("des", des).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => CopyHelper(src, des, overwrite), "FabricDirectory.Copy");
        }

        internal static void Rename(string src, string des, bool overwrite)
        {
            Requires.Argument("src", src).NotNullOrEmpty();
            Requires.Argument("des", des).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => RenameHelper(src, des, overwrite), "FabricDirectory.Rename");
        }

        internal static bool Exists(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => ExistsHelper(path), "FabricDirectory.Exists");
        }

        internal static void Delete(string path)
        {
            Delete(path, false, false);
        }

        internal static void Delete(string path, bool recursive)
        {
            Delete(path, recursive, false);
        }

        internal static void Delete(string path, bool recursive, bool deleteReadOnlyFiles)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => DeleteHelper(path, recursive, deleteReadOnlyFiles), "FabricDirectory.Delete");
        }

        internal static bool IsSymbolicLink(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => IsSymbolicLinkHelper(path), "FabricDirectory.IsSymbolicLink");
        }

        private static void CreateHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricDirectoryCreate(pin.AddBlittable(path));
            }
        }

        private static string[] GetDirectoriesHelper(string path, string pattern, bool getFullPath, SearchOption option)
        {
            using (var pin = new PinCollection())
            {
                var collectionResult = StringCollectionResult.FromNative(
                    NativeCommon.FabricDirectoryGetDirectories(
                        pin.AddBlittable(path),
                        pin.AddBlittable(pattern),
                        NativeTypes.ToBOOLEAN(getFullPath),
                        NativeTypes.ToBOOLEAN(option == SearchOption.TopDirectoryOnly)), 
                    true);
                string[] arrayResult = new string[collectionResult.Count];
                collectionResult.CopyTo(arrayResult, 0);    
                return arrayResult;
            }
        }

        private static string[] GetFilesHelper(string path, string pattern, bool getFullPath, SearchOption option)
        {
            using (var pin = new PinCollection())
            {
                var collectionResult = StringCollectionResult.FromNative(
                    NativeCommon.FabricDirectoryGetFiles(
                        pin.AddBlittable(path),
                        pin.AddBlittable(pattern),
                        NativeTypes.ToBOOLEAN(getFullPath),
                        NativeTypes.ToBOOLEAN(option == SearchOption.TopDirectoryOnly)),
                    !getFullPath); // allow duplicates
                string[] arrayResult = new string[collectionResult.Count];
                collectionResult.CopyTo(arrayResult, 0);
                return arrayResult;
            }
        }

        private static void CopyHelper(string src, string des, bool overwrite)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricDirectoryCopy(
                    pin.AddBlittable(src),
                    pin.AddBlittable(des),
                    NativeTypes.ToBOOLEAN(overwrite));
            }
        }

        private static void RenameHelper(string src, string des, bool overwrite)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricDirectoryRename(
                    pin.AddBlittable(src),
                    pin.AddBlittable(des),
                    NativeTypes.ToBOOLEAN(overwrite));
            }
        }

        private static bool ExistsHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN isExisted;
                NativeCommon.FabricDirectoryExists(pin.AddBlittable(path), out isExisted);
                return NativeTypes.FromBOOLEAN(isExisted);
            }
        }

        private static void DeleteHelper(string path, bool recursive, bool deleteReadOnlyFiles)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricDirectoryDelete(
                    pin.AddBlittable(path),
                    NativeTypes.ToBOOLEAN(recursive),
                    NativeTypes.ToBOOLEAN(deleteReadOnlyFiles));
            }
        }

        private static bool IsSymbolicLinkHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN result;
                NativeCommon.FabricDirectoryIsSymbolicLink(pin.AddBlittable(path), out result);
                return NativeTypes.FromBOOLEAN(result);
            }
        }
    }
}