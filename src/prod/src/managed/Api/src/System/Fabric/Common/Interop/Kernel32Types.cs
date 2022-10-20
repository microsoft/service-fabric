// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System.Runtime.InteropServices;

    /// <summary>
    /// Type definitions for Win32 Kernel32 APIs.
    /// </summary>
    internal static class Kernel32Types
    {
        /// <summary>
        /// Definition of FILE_INFO_BY_HANDLE_CLASS enumeration.
        /// </summary>
        public enum FILE_INFO_BY_HANDLE_CLASS : int
        {
            FileBasicInfo =                     0,
            FileStandardInfo =                  1,
            FileNameInfo =                      2,
            FileRenameInfo =                    3,
            FileDispositionInfo =               4,
            FileAllocationInfo =                5,
            FileEndOfFileInfo =                 6,
            FileStreamInfo =                    7,
            FileCompressionInfo =               8,
            FileAttributeTagInfo =              9,
            FileIdBothDirectoryInfo =           10, // 0xA
            FileIdBothDirectoryRestartInfo =    11, // 0xB
            FileIoPriorityHintInfo =            12, // 0xC
            FileRemoteProtocolInfo =            13, // 0xD
            FileFullDirectoryInfo =             14, // 0xE
            FileFullDirectoryRestartInfo =      15, // 0xF
            FileStorageInfo =                   16, // 0x10
            FileAlignmentInfo =                 17, // 0x11
            FileIdInfo =                        18, // 0x12
            FileIdExtdDirectoryInfo =           19, // 0x13
            FileIdExtdDirectoryRestartInfo =    20, // 0x14
            MaximumFileInformationClass
        }

        /// <summary>
        /// Definition of Win32 PRIORITY_HINT enumeration: https://msdn.microsoft.com/en-us/library/windows/desktop/bb870631(v=vs.85).aspx
        /// </summary>
        public enum PRIORITY_HINT : int
        {
            // The lowest possible priority hint level. The system uses this value for background I/O operations.
            IoPriorityHintVeryLow =         0,

            // A low-priority hint level.
            IoPriorityHintLow =             1,

            // A normal-priority hint level. This value is the default setting for an I/O operation.
            IoPriorityHintNormal =          2,

            // This value is used for validation. Supported values are less than this value.
            MaximumIoPriorityHintType =     3
        }

        /// <summary>
        /// Definition of Win32 FILE_IO_PRIORITY_HINT_INFO struct: https://msdn.microsoft.com/en-us/library/windows/desktop/bb870610(v=vs.85).aspx
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 8, Size = 8)]
        public struct FILE_IO_PRIORITY_HINT_INFO
        {
            public PRIORITY_HINT PriorityHint;
        }

        /// <summary>
        /// Struct that holds possible FileInformation types.
        /// The structure that this parameter points to corresponds to the class that is specified by FileInformationClass.
        /// </summary>
        /// <devnote>
        /// This is the place to add other FileInformation structs when we need to support them.
        /// E.g. FILE_BASIC_INFO, FILE_RENAME_INFO, FILE_DISPOSITION_INFO, etc.
        /// </devnote>
        [StructLayout(LayoutKind.Explicit)]
        public struct FileInformation
        {
            [FieldOffset(0)]
            public FILE_IO_PRIORITY_HINT_INFO FILE_IO_PRIORITY_HINT_INFO;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct FILE_BASIC_INFO
        {
            [FieldOffset(0)]
            internal long CreationTime;

            [FieldOffset(8)]
            internal long LastAccessTime;

            [FieldOffset(16)]
            internal long LastWriteTime;

            [FieldOffset(24)]
            internal long ChangeTime;

            [FieldOffset(32)]
            internal uint FileAttributes;
        }
    }
}