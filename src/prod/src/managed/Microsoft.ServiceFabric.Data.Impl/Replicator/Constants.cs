// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    internal static class Constants
    {
        /// <summary>
        /// Backup context prefix.
        /// </summary>
        public const string BackupContextPrefix = "backup ";

        public const string DefaultEarliestLogReader = "None";

        public const string DefaultSharedLogContainerGuid = "{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}";

        public const string DefaultSharedLogName = "replicatorshared.log";

        public const string DefaultSharedLogSubdirectory = "ReplicatorLog";

        public const string LocalBackupFolderName = "trbackup";

        public const string Fabric_HostApplicationDirectory = "Fabric_Folder_Application_OnHost";

        /// <summary>
        /// Number of bytes in a KBytes.
        /// </summary>
        public const int NumberOfBytesInKBytes = 1024;

        public const int MaxReplicationWriteMemoryStreamsBufferPoolItemCount = 1024;

        public const int ReplicationWriteMemoryStreamsBufferPoolCleanupMilliseconds = 60 * 1000;

        public const string ReplicatorBackupFolderName = "lr";

        public const string ReplicatorBackupLogName = "backup.log";

        public const string ReplicatorBackupMetadataFileName = "backup.metadata";

        public const string ReplicatorIncrementalBackupMetadataFileName = "incremental.metadata";

        // Backup and restore
        public const string RestoreFolderName = "restore";

        public const string RestoreTokenName = "restore.token";

        /// <summary>
        /// Sizeof(Guid)
        /// </summary>
        public const int SizeOfGuidBytes = 16;

        /// <summary>
        /// Alignment for x64 architectures.
        /// </summary>
        public const int X64Alignment = 8;

        public const string StateManagerBackupFolderName = "sm";

        /// <summary>
        /// Default maximum length of ProgressVector string in Kb
        /// </summary>
        public const int ProgressVectorMaxStringSizeInKb = 60;

        /// <summary>
        /// Prefix when tracing progress vectors
        /// </summary>
        public const string ProgressVectorTracePrefix = "                            ";

        /// <summary>
        /// Constant bytes used for padding.
        /// </summary>
        public static readonly byte[] BadFood = new byte[] {0x0B, 0xAD, 0xF0, 0x0D};

        /// <summary>
        /// Size of the Bad Food.
        /// </summary>
        public static readonly int BadFoodLength = BadFood.Length;
    }
}