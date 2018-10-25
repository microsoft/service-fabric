// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
#if !DotNetCoreClrLinux
    using System.Runtime.InteropServices;

    /// <summary>
    /// Type definitions for Windows Native APIs in NTDLL.dll
    /// </summary>
    internal static class NTTypes
    {
        /// <summary>
        /// Definition of FILE_INFORMATION_CLASS enumeration: https://msdn.microsoft.com/en-us/library/windows/hardware/ff728840(v=vs.85).aspx
        /// </summary>
        public enum FILE_INFORMATION_CLASS : int
        {
            FileDirectoryInformation =      1,
            FileFullDirectoryInformation,
            FileBothDirectoryInformation,
            FileBasicInformation,
            FileStandardInformation,
            FileInternalInformation,
            FileEaInformation,
            FileAccessInformation,
            FileNameInformation,
            FileRenameInformation,
            FileLinkInformation,
            FileNamesInformation,
            FileDispositionInformation,
            FilePositionInformation,
            FileFullEaInformation,
            FileModeInformation,
            FileAlignmentInformation,
            FileAllInformation,
            FileAllocationInformation,
            FileEndOfFileInformation,
            FileAlternateNameInformation,
            FileStreamInformation,
            FilePipeInformation,
            FilePipeLocalInformation,
            FilePipeRemoteInformation,
            FileMailslotQueryInformation,
            FileMailslotSetInformation,
            FileCompressionInformation,
            FileObjectIdInformation,
            FileCompletionInformation,
            FileMoveClusterInformation,
            FileQuotaInformation,
            FileReparsePointInformation,
            FileNetworkOpenInformation,
            FileAttributeTagInformation,
            FileTrackingInformation,
            FileIdBothDirectoryInformation,
            FileIdFullDirectoryInformation,
            FileValidDataLengthInformation,
            FileShortNameInformation,
            FileIoCompletionNotificationInformation,
            FileIoStatusBlockRangeInformation,
            FileIoPriorityHintInformation,
            FileSfioReserveInformation,
            FileSfioVolumeInformation,
            FileHardLinkInformation,
            FileProcessIdsUsingFileInformation,
            FileNormalizedNameInformation,
            FileNetworkPhysicalNameInformation,
            FileIdGlobalTxDirectoryInformation,
            FileIsRemoteDeviceInformation,
            FileUnusedInformation,
            FileNumaNodeInformation,
            FileStandardLinkInformation,
            FileRemoteProtocolInformation,
            FileRenameInformationBypassAccessCheck,
            FileLinkInformationBypassAccessCheck,
            FileVolumeNameInformation,
            FileIdInformation,
            FileIdExtdDirectoryInformation,
            FileReplaceCompletionInformation,
            FileHardLinkFullIdInformation,
            FileIdExtdBothDirectoryInformation,
            FileMaximumInformation
        }

        /// <summary>
        /// Definition of IO_PRIORITY_HINT enumeration: https://msdn.microsoft.com/en-us/library/windows/hardware/ff550594(v=vs.85).aspx
        /// </summary>
        public enum IO_PRIORITY_HINT : int
        {
            IoPriorityVeryLow = 0,
            IoPriorityLow = 1,
            IoPriorityNormal = 2,
            IoPriorityHigh = 3,
            IoPriorityCritical = 4,
            MaxIoPriorityTypes = 5
        }

        /// <summary>
        /// Definition of IO_STATUS_BLOCK structure: https://msdn.microsoft.com/en-us/library/windows/hardware/ff550671(v=vs.85).aspx
        /// </summary>
        public struct IO_STATUS_BLOCK
        {
            /// <summary>
            /// This is the completion status, either STATUS_SUCCESS if the requested operation was completed successfully or an informational, warning, or error STATUS_XXX value. For more information, see Using NTSTATUS values.
            /// </summary>
            internal uint Status;

            /// <summary>
            /// This is set to a request-dependent value. For example, on successful completion of a transfer request, this is set to the number of bytes transferred. If a transfer request is completed with another STATUS_XXX, this member is set to zero. 
            /// </summary>
            internal ulong Information;
        }

        /// <summary>
        /// Definition of IO_PRIORITY_HINT structure: https://msdn.microsoft.com/en-us/library/windows/hardware/ff545802(v=vs.85).aspx
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        public struct FILE_IO_PRIORITY_HINT_INFORMATION
        {
            public IO_PRIORITY_HINT PriorityHint;
        }
    }
#endif
}