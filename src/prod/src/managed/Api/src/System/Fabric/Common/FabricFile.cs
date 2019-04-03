// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Interop;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using Microsoft.Win32.SafeHandles;
    using BOOLEAN = System.SByte;

    internal class FabricFile
    {
#if DotNetCoreClrLinux
        // Fake calls for Linux
        public static bool SetFileInformationByHandle(SafeHandle hFile, Kernel32Types.FILE_INFO_BY_HANDLE_CLASS FileInformationClass, ref Kernel32Types.FileInformation FileInformation, int dwBufferSize)
        {
            return true;
        }

        public static bool SetFileInformationByHandle(SafeHandle hFile, Kernel32Types.FILE_INFO_BY_HANDLE_CLASS FileInformationClass, ref Kernel32Types.FILE_BASIC_INFO lpFileInformation, int dwBufferSize)
        {
            return true;
        }
#else
        /// <summary>
        /// PInvokes to SetFileInformationByHandle: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365539(v=vs.85).aspx
        /// </summary>
        /// <param name="hFile">
        /// A handle to the file for which to change information.
        /// This handle must be opened with the appropriate permissions for the requested change.
        /// </param>
        /// <param name="FileInformationClass">
        /// A FILE_INFO_BY_HANDLE_CLASS enumeration value that specifies the type of information to be changed.
        /// </param>
        /// <param name="FileInformation">
        /// A pointer to the buffer that contains the information to change for the specified file information class. 
        /// The structure that this parameter points to corresponds to the class that is specified by FileInformationClass.</param>
        /// <param name="dwBufferSize">The size of FileInformation, in bytes.</param>
        /// <returns>
        /// Returns nonzero if successful or zero otherwise.
        /// To get extended error information, call GetLastError.
        /// </returns>
        /// <remarks>
        /// Only supported FileInformationClass is FileInformationClass.FileIoPriorityHintInfo.
        /// More can be added as needed.
        /// </remarks>
        /// <devnote>
        /// Note that here we PInvoke to Win32 directly instead of PInvoking the FabricCommon and then calling a Win32 API.
        /// This is because currently this API is not used in native and hence going through FabricCommon would be unncessary.
        /// If the need ever rises, we could plumb this through FabricCommon.
        /// </devnote>
        [DllImport("Kernel32.dll", SetLastError = true)]
        public static extern bool SetFileInformationByHandle(SafeHandle hFile, Kernel32Types.FILE_INFO_BY_HANDLE_CLASS FileInformationClass, ref Kernel32Types.FileInformation FileInformation, int dwBufferSize);

        [DllImport("Kernel32.dll", SetLastError = true)]
        public static extern bool SetFileInformationByHandle(SafeHandle hFile, Kernel32Types.FILE_INFO_BY_HANDLE_CLASS FileInformationClass, ref Kernel32Types.FILE_BASIC_INFO FileBasicInformation, int dwBufferSize);

        /// <summary>
        /// PInvokes to NtQueryInformationFile: https://msdn.microsoft.com/en-us/library/windows/hardware/ff567052(v=vs.85).aspx
        /// </summary>
        /// <param name="FileHandle">Handle to a file object.</param>
        /// <param name="IoStatusBlock">
        /// Pointer to an IO_STATUS_BLOCK structure that receives the final completion status and information about the operation. 
        /// The Information member receives the number of bytes that this routine actually writes to the FileInformation buffer.
        /// </param>
        /// <param name="pInfoBlock">
        /// Pointer to a caller-allocated buffer into which the routine writes the requested information about the file object. 
        /// The FileInformationClass parameter specifies the type of information that the caller requests.
        /// </param>
        /// <param name="length">The size, in bytes, of the buffer pointed to by FileInformation.</param>
        /// <param name="fileInformation">
        /// Specifies the type of information to be returned about the file, in the buffer that FileInformation points to. 
        /// Device and intermediate drivers can specify any of the following FILE_INFORMATION_CLASS values.
        /// </param>
        /// <returns></returns>
        /// <devnote>
        /// This method is marked internal to indicate that it is used for testing purposes only.
        /// SetFileInformationByHandle does not have a getter pair that allows querying FILE_IO_PRIORITY_HINT_INFO.
        /// Hence NtQueryInformationFile is used, which does expose a getter for the FILE_IO_PRIORITY_HINT_INFO, to test SetFileInformationByHandle.
        /// </devnote>
        [DllImport("ntdll.dll", SetLastError = true)]
        internal static extern IntPtr NtQueryInformationFile(
            SafeHandle FileHandle, 
            ref NTTypes.IO_STATUS_BLOCK IoStatusBlock, 
            IntPtr pInfoBlock, 
            uint length, 
            NTTypes.FILE_INFORMATION_CLASS fileInformation);
#endif

        internal static FileStream Create(string path)
        {
            return Open(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None);
        }

        internal static FileStream Open(string path, FileMode fileMode)
        {
            return Open(path, fileMode, FileAccess.ReadWrite, FileShare.None);
        }

        internal static FileStream Open(string path, FileMode fileMode, FileAccess fileAccess)
        {
            return Open(path, fileMode, fileAccess, FileShare.None);
        }

        internal static FileStream Open(string path, FileMode fileMode, FileAccess fileAccess, FileShare fileShare)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => OpenHelper(path, fileMode, fileAccess, fileShare), "FabricFile.Open");
        }

        internal static FileStream Open(string path, FileMode fileMode, FileAccess fileAccess, FileShare fileShare, int cacheSize, FileOptions fileOptions)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => OpenHelper(path, fileMode, fileAccess, fileShare, cacheSize, fileOptions), "FabricFile.Open");
        }

        internal static void Copy(string src, string des, bool overwrite)
        {
            Requires.Argument("src", src).NotNullOrEmpty();
            Requires.Argument("des", des).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => CopyHelper(src, des, overwrite), "FabricFile.Copy");
        }

        /// <summary>
        /// Move the <paramref name="src"/> file to the <paramref name="des"/> file.
        /// If the <paramref name="des"/> exists, a FabricException is throw.
        /// Once the move operation is successfully completed, <paramref name="src"/> file content would be moved to the <paramref name="des"/> file.
        /// [NOTE] : <paramref name="src"/> would not exist after the successful completion.
        /// </summary>
        /// <param name="src">File to move</param>
        /// <param name="des">New renamed file name</param>
        internal static void Move(string src, string des)
        {
            Requires.Argument("src", src).NotNullOrEmpty();
            Requires.Argument("des", des).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => MoveHelper(src, des), "FabricFile.Move");
        }

        internal static bool Exists(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => ExistsHelper(path), "FabricFile.Exists");
        }

        internal static void Delete(string path)
        {
            FabricFile.Delete(path, false);
        }

        internal static void SetLastWriteTime(string path, DateTime lastWriteTime)
        {
            Requires.Argument("path", path).NotNullOrEmpty();  

#if DotNetCoreClrLinux
            File.SetLastWriteTime(path, lastWriteTime);
#else
            Utility.WrapNativeSyncInvokeInMTA(() => SetLastWriteTimeHelper(path, lastWriteTime.ToFileTime()), "FabricFile.SetLastWriteTime");
#endif

            
        }

        /// <summary>
        /// Replace the <paramref name="sourceFileName"/> file to the <paramref name="destinationFileName"/> file.
        /// Order of operations :
        ///     1. Copy <paramref name="destinationFileName"/> to <paramref name="destinationBackupFileName"/>
        ///     2. Move <paramref name="sourceFileName"/> to <paramref name="destinationFileName"/>
        ///     3. Delete <paramref name="sourceFileName"/>
        /// <paramref name="destinationFileName"/> must exist, else a FileNotFoundException would be thrown.
        /// If the <paramref name="destinationBackupFileName"/> does not exist, it will be created. If it does exists, it would be overwritten.
        /// </summary>
        /// <param name="sourceFileName">Source file to replace</param>
        /// <param name="destinationFileName">New renamed destination file name</param>
        /// <param name="destinationBackupFileName">Backup of the original <paramref name="destinationFileName"/> file</param>
        /// <param name="ignoreMetadataErrors"></param>
        internal static void Replace(string sourceFileName, string destinationFileName, string destinationBackupFileName, bool ignoreMetadataErrors)
        {
            Requires.Argument("sourceFileName", sourceFileName).NotNullOrEmpty();
            Requires.Argument("destinationFileName", destinationFileName).NotNullOrEmpty();
            Requires.Argument("destinationBackupFileName", destinationBackupFileName).NotNullOrEmpty();

            Utility.WrapNativeSyncInvoke(() => ReplaceHelper(sourceFileName, destinationFileName, destinationBackupFileName, ignoreMetadataErrors), "FabricFile.Replace");
        }

        internal static bool CreateHardLink(string fileName, string existingFileName)
        {
            Requires.Argument("fileName", fileName).NotNullOrEmpty();
            Requires.Argument("existingFileName", existingFileName).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(() => CreateHardLinkHelper(fileName, existingFileName), "FabricFile.CreateHardLink");
        }

        internal static void Delete(string path, bool deleteReadonly)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => DeleteHelper(path, deleteReadonly), "FabricFile.Delete");
        }

        internal static Int64 GetSize(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => GetSizeHelper(path), "FabricFile.GetSize");
        }

        internal static DateTime GetLastWriteTime(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => GetLastWriteTimeHelper(path), "FabricFile.GetLastWriteTimeHelper");
        }

        internal static string GetVersionInfo(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            return Utility.WrapNativeSyncInvokeInMTA(() => GetVersionInfoHelper(path), "FabricFile.GetVersionInfoHelper");
        }

        internal static void RemoveReadOnlyAttribute(string path)
        {
            Requires.Argument("path", path).NotNullOrEmpty();
            Utility.WrapNativeSyncInvokeInMTA(() => RemoveReadOnlyAttributeHelper(path), "FabricFile.RemoveReadOnlyAttribute");
        }

        private static void CopyHelper(string src, string des, bool overwrite)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricFileCopy(
                    pin.AddBlittable(src),
                    pin.AddBlittable(des),
                    NativeTypes.ToBOOLEAN(overwrite));
            }
        }

        private static void MoveHelper(string src, string des)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricFileMove(
                    pin.AddBlittable(src),
                    pin.AddBlittable(des));
            }
        }

        private static void UpdateFilePermission(string filePath)
        {
#if DotNetCoreClrLinux
            //CoreCLR doesn't honour umask value for the new file/folder crated. As a workaround change the permission using chmod after file is created.
            //Setting default permission to 0644, i.e. read/write for Owner & Read only for group & others.
            const int defaultFilePermission = 0x1a4;
            if (FabricFile.Exists(filePath))
            {
                NativeHelper.chmod(filePath, defaultFilePermission);
            }
#endif
        }

        private static FileStream OpenHelper(string path, FileMode fileMode, FileAccess fileAccess, FileShare fileShare)
        {
            using (var pin = new PinCollection())
            {
#if DotNetCoreClrLinux
                FileStream fs = new FileStream(path, fileMode);
                UpdateFilePermission(path);
                return fs;
#else
                IntPtr handle;
                NativeCommon.FabricFileOpen(
                    pin.AddBlittable(path),
                    ToNative(fileMode),
                    ToNative(fileAccess),
                    ToNative(fileShare),
                    out handle);
                return new FileStream(new SafeFileHandle(handle, true), fileAccess);
#endif
            }
        }

        private static FileStream OpenHelper(string path, FileMode fileMode, FileAccess fileAccess, FileShare fileShare, int cacheSize, FileOptions fileOptions)
        {
            using (var pin = new PinCollection())
            {
#if DotNetCoreClrLinux
                FileStream fs = new FileStream(path, fileMode);
                UpdateFilePermission(path);
                return fs;
#else
                IntPtr handle;
                NativeCommon.FabricFileOpenEx(
                    pin.AddBlittable(path),
                    ToNative(fileMode),
                    ToNative(fileAccess),
                    ToNative(fileShare),
                    ToNative(fileOptions),
                    out handle);
                return new FileStream(new SafeFileHandle(handle, true), fileAccess, cacheSize, fileOptions.HasFlag(FileOptions.Asynchronous));
#endif
            }
        }

        private static void SetLastWriteTimeHelper(string path, long lastWriteTime)
        {
#if DotNetCoreClrLinux
            throw new NotSupportedException("This operation is not supported on Linux. Please use File.SetLastWriteTime API");
#else
            using (var pin = new PinCollection())
            {
                // Default values indicate "no change". Use defaults.
                var basicInfo = new Kernel32Types.FILE_BASIC_INFO
                {
                    CreationTime = -1,
                    LastAccessTime = -1,
                    LastWriteTime = lastWriteTime,
                    ChangeTime = -1,
                    FileAttributes = 0
                };

                // To update the file last written time, it needs to be opened up with Write access.
                using (FileStream file = FabricFile.Open(path, FileMode.Open, FileAccess.ReadWrite))
                {
                    var success = SetFileInformationByHandle(
                        file.SafeFileHandle,
                        Kernel32Types.FILE_INFO_BY_HANDLE_CLASS.FileBasicInfo,
                        ref basicInfo,
                        Marshal.SizeOf(typeof(Kernel32Types.FILE_BASIC_INFO)));

                    if (!success)
                    {
                        throw new Win32Exception(Marshal.GetLastWin32Error());
                    }
                }
            }
#endif
        }

        private static bool ExistsHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN isExisted;
                NativeCommon.FabricFileExists(pin.AddBlittable(path), out isExisted);
                return NativeTypes.FromBOOLEAN(isExisted);
            }
        }

        private static void RemoveReadOnlyAttributeHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricFileRemoveReadOnlyAttribute(
                    pin.AddBlittable(path));
            }
        }

        private static void ReplaceHelper(string sourceFileName, string destinationFileName, string destinationBackupFileName, bool ignoreMetadataErrors)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricFileReplace(
                    pin.AddBlittable(destinationFileName),
                    pin.AddBlittable(sourceFileName),
                    pin.AddBlittable(destinationBackupFileName),
                    NativeTypes.ToBOOLEAN(ignoreMetadataErrors));
            }
        }

        private static bool CreateHardLinkHelper(string fileName, string existingFileName)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN succeeded;
                NativeCommon.FabricFileCreateHardLink(
                    pin.AddBlittable(fileName),
                    pin.AddBlittable(existingFileName),
                    out succeeded);
                return NativeTypes.FromBOOLEAN(succeeded);
            }
        }

        private static Int64 GetSizeHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                Int64 size = 0;
                NativeCommon.FabricFileGetSize(
                    pin.AddBlittable(path), out size);
                return size;
            }
        }

        private static DateTime GetLastWriteTimeHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                NativeTypes.NativeFILETIME lastWriteTime;
                NativeCommon.FabricFileGetLastWriteTime(
                    pin.AddBlittable(path), out lastWriteTime);
                return NativeTypes.FromNativeFILETIME(lastWriteTime);
            }
        }

        private static string GetVersionInfoHelper(string path)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricFileGetVersionInfo(pin.AddBlittable(path)));
            }
        }

        private static void DeleteHelper(string path, bool deleteReadonly)
        {
            using (var pin = new PinCollection())
            {
                NativeCommon.FabricFileDelete(
                    pin.AddBlittable(path),
                    NativeTypes.ToBOOLEAN(deleteReadonly));
            }
        }

        private static NativeTypes.FABRIC_FILE_MODE ToNative(FileMode mode)
        {
            switch (mode)
            {
                case FileMode.Create:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_CREATE_ALWAYS;
                case FileMode.CreateNew:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_CREATE_NEW;
                case FileMode.Open:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_OPEN_EXISTING;
                case FileMode.OpenOrCreate:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_OPEN_ALWAYS;
                case FileMode.Truncate:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_TRUNCATE_EXISTING;
                default:
                    return NativeTypes.FABRIC_FILE_MODE.FABRIC_FILE_MODE_INVALID;
            }
        }

        private static NativeTypes.FABRIC_FILE_ACCESS ToNative(FileAccess access)
        {
            switch (access)
            {
                case FileAccess.Read:
                    return NativeTypes.FABRIC_FILE_ACCESS.FABRIC_FILE_ACCESS_READ;
                case FileAccess.Write:
                    return NativeTypes.FABRIC_FILE_ACCESS.FABRIC_FILE_ACCESS_WRITE;
                case FileAccess.ReadWrite:
                    return NativeTypes.FABRIC_FILE_ACCESS.FABRIC_FILE_ACCESS_READ_WRITE;
                default:
                    return NativeTypes.FABRIC_FILE_ACCESS.FABRIC_FILE_ACCESS_INVALID;
            }
        }

        private static NativeTypes.FABRIC_FILE_SHARE ToNative(FileShare share)
        {
            switch (share)
            {
                case FileShare.Delete:
                    return NativeTypes.FABRIC_FILE_SHARE.FABRIC_FILE_SHARE_DELETE;
                case FileShare.Read:
                    return NativeTypes.FABRIC_FILE_SHARE.FABRIC_FILE_SHARE_READ;
                case FileShare.Write:
                    return NativeTypes.FABRIC_FILE_SHARE.FABRIC_FILE_SHARE_WRITE;
                case FileShare.ReadWrite:
                    return NativeTypes.FABRIC_FILE_SHARE.FABRIC_FILE_SHARE_READ_WRITE;
                default:
                    return NativeTypes.FABRIC_FILE_SHARE.FABRIC_FILE_SHARE_INVALID;
            }
        }

        private static NativeTypes.FABRIC_FILE_ATTRIBUTES ToNative(FileOptions options)
        {
            NativeTypes.FABRIC_FILE_ATTRIBUTES attributes = NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_NORMAL;

            if (options.HasFlag(FileOptions.Asynchronous))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_OVERLAPPED;
            if (options.HasFlag(FileOptions.DeleteOnClose))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_DELETE_ON_CLOSE;
            if (options.HasFlag(FileOptions.Encrypted))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_ENCRYPTED;
            if (options.HasFlag(FileOptions.RandomAccess))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_RANDOM_ACCESS;
            if (options.HasFlag(FileOptions.SequentialScan))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_SEQUENTIAL_SCAN;
            if (options.HasFlag(FileOptions.WriteThrough))
                attributes |= NativeTypes.FABRIC_FILE_ATTRIBUTES.FABRIC_FILE_ATTRIBUTES_WRITE_THROUGH;

            return attributes;
        }
    }
}