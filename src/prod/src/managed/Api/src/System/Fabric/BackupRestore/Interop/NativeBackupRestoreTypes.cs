// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Interop
{
    using System;
    using System.Fabric.Interop;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.InteropServices;
    
    //// ----------------------------------------------------------------------------
    //// typedefs

    using BOOLEAN = System.SByte;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_BACKUP_OPERATION_ID = System.Guid;
    using FABRIC_RESTORE_OPERATION_ID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1310:FieldNamesMustNotContainUnderscore", Justification = "Matches names in IDL.")]
    internal static class NativeBackupRestoreTypes
    {
        #region Backup Restore Enums & Types

        [Guid("6fd2680b-63db-424a-988e-576c73977be2")]
        internal enum FABRIC_BACKUP_POLICY_TYPE : int
        {
            FABRIC_BACKUP_POLICY_TYPE_INVALID           = 0x0000,
            FABRIC_BACKUP_POLICY_TYPE_FREQUENCY_BASED   = 0x0001,
            FABRIC_BACKUP_POLICY_TYPE_SCHEDULE_BASED    = 0x0002
        }

        [Guid("9fd97a6c-63bf-4691-b34c-734dbe73aa5c")]
        internal enum FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE : int
        {
            FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_INVALID  = 0x0000,
            FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_DAILY    = 0x0001,
            FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_WEEKLY   = 0X0002,
        }

        [Guid("98a5de3d-b10c-4787-ab0b-0810045caa8b")]
        internal enum FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE : int
        {
            FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_INVALID = 0x0000,
            FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_MINUTES = 0x0001,
            FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_HOURS   = 0X0002,
        }

        [Guid("0c51aec0-922d-4f07-961f-26e7c0ab686c")]
        internal enum FABRIC_BACKUP_STORE_TYPE : int
        {
            FABRIC_BACKUP_STORE_TYPE_INVALID          = 0x0000,
            FABRIC_BACKUP_STORE_TYPE_FILE_SHARE       = 0x0001,
            FABRIC_BACKUP_STORE_TYPE_AZURE_STORE      = 0x0002,
            FABRIC_BACKUP_STORE_TYPE_DSMS_AZURE_STORE = 0x0003,
        }

        [Guid("9460d8aa-e70d-480e-a118-453016a0867e")]
        internal enum FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE : int
        {
            FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_NONE         = 0x0000,
            FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_DOMAIN_USER  = 0x0001,
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_PARTITION_INFO
        {
            public IntPtr ServiceName;
            public FABRIC_PARTITION_ID PartitionId;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_STORE_INFORMATION
        {
            public FABRIC_BACKUP_STORE_TYPE StoreType;
            public IntPtr StoreAccessInformation;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION
        {
            public IntPtr FileSharePath;
            public FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE AccessType;
            public IntPtr PrimaryUserName;
            public IntPtr PrimaryPassword;
            public IntPtr SecondaryUserName;
            public IntPtr SecondaryPassword;
            public BOOLEAN IsPasswordEncrypted;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION
        {
            public IntPtr ConnectionString;
            public IntPtr ContainerName;
            public IntPtr FolderPath;
            public BOOLEAN IsConnectionStringEncrypted;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION
        {
            public IntPtr StorageCredentialsSourceLocation;
            public IntPtr ContainerName;
            public IntPtr FolderPath;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_POLICY
        {
            public IntPtr Name;
            public Guid PolicyId;
            public FABRIC_BACKUP_POLICY_TYPE PolicyType;
            public IntPtr PolicyDescription;
            public byte MaxIncrementalBackups;
            public IntPtr StoreInformation;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST
        {
            public UInt32 Count;
            public IntPtr RunTimes;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_SCHEDULE_BASED_BACKUP_POLICY
        {
            public FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE RunScheduleType;
            public IntPtr RunTimesList;
            public byte RunDays;
            public IntPtr Reserverd;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_FREQUENCY_BASED_BACKUP_POLICY
        {
            public FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE RunFrequencyType;
            public UInt16 Value;
            public IntPtr Reserverd;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_CONFIGURATION
        {
            public UInt32 OperationTimeoutMilliseconds;
            public IntPtr StoreInformation;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_OPERATION_RESULT
        {
            public IntPtr ServiceName;
            public FABRIC_PARTITION_ID PartitionId;
            public FABRIC_BACKUP_OPERATION_ID OperationId;
            public Int32 ErrorCode;
            public IntPtr Message;
            public Guid BackupId;
            public IntPtr BackupLocation;
            public NativeTypes.FABRIC_EPOCH EpochOfLastBackupRecord;
            public FABRIC_SEQUENCE_NUMBER LsnOfLastBackupRecord;
            public NativeTypes.NativeFILETIME TimeStampUtc;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_RESTORE_OPERATION_RESULT
        {
            public IntPtr ServiceName;
            public FABRIC_PARTITION_ID PartitionId;
            public FABRIC_RESTORE_OPERATION_ID OperationId;
            public Int32 ErrorCode;
            public IntPtr Message;
            public NativeTypes.NativeFILETIME TimeStampUtc;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_RESTORE_POINT_DETAILS
        {
            public FABRIC_RESTORE_OPERATION_ID OperationId;
            public IntPtr BackupLocations;
            public IntPtr StoreInformation;
            public BOOLEAN UserInitiatedOperation;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_UPLOAD_INFO
        {
            public IntPtr BackupMetadataFilePath;
            public IntPtr LocalBackupPath;
            public IntPtr DestinationFolderName;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_BACKUP_DOWNLOAD_INFO
        {
            public IntPtr BackupLocations;
            public IntPtr DestinationRootPath;
            public IntPtr Reserved;
        }
        #endregion
    }
}