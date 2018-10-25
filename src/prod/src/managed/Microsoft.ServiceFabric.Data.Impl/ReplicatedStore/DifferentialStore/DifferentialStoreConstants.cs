// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Constants for DifferentialStore.
    /// </summary>
    internal static class DifferentialStoreConstants
    {
        public const long InvalidLSN = -1;
        public const long ZeroLSN = 0;

        public const string AddCopyData_Received = "received";
        public const string RemoveAll_Starting = "starting";
        public const string RemoveAll_Completed = "completed";
        public const string Open_Starting = "starting";
        public const string Open_Completed = "completed";
        public const string Close_Starting = "starting";
        public const string Close_Completed = "completed";
        public const string MergeKeyCheckpointFilesAsync_Starting = "starting";
        public const string MergeKeyCheckpointFilesAsync_Completed = "completed";
        public const string Abort_Starting = "starting";
        public const string Abort_Completed = "completed";
        public const string ChangeRole_Starting = "starting";
        public const string ChangeRole_Completed = "completed";
        public const string Checkpoint_Prepare_Starting = "prepare checkpoint starting. CheckpointLSN: ";
        public const string Checkpoint_Perform_Starting = "perform checkpoint starting. LastPrepareCheckpointLSN: {0} CheckpointLSN: {1}";
        public const string Checkpoint_Complete_Starting = "complete checkpoint starting";
        public const string Open_Checkpoint_Create = "creating first checkpoint";
        public const string Open_Checkpoint_DeleteStaleNext = "deleting stale next checkpoint";
        public const string Recover_Checkpoint_Completed = "completed";
        public const string OnRecoveryCompleted_Starting = "starting";
        public const string OnRecoveryCompleted_Completed = "completed";
        public const string RemoveState_Starting = "starting";
        public const string RemoveState_Completed = "completed";
        public const string OnDataLoss_Starting = "starting";
        public const string OnDataLoss_Completed = "completed";
        public const string GetCurrentState_Starting = "starting";
        public const string BeginSettingCurrent_Starting = "starting";
        public const string BeginSettingCurrent_Completed = "completed";
        public const string EndSettingCurrent_Starting = "starting";
        public const string EndSettingCurrent_Completed = "completed";
        public const string OnRecoveryApply_Starting = "starting";
        public const string OnRecoveryApply_Completed = "completed";
        public const string OnSecondaryApply_Starting = "starting";
        public const string OnSecondaryApply_Completed = "completed";
        public const string BackupCheckpoint_Starting = "starting";
        public const string BackupCheckpoint_Completed = "completed";
        public const string RestoreCheckpoint_Starting = "starting";
        public const string RestoreCheckpoint_Completed = "completed";
        public const string RestoreCheckpoint_FailedWith = "failed with ";
        public const string RecoverCopyState_Completed = "completed";
        public const string Cleanup_Starting = "starting";
        public const string Cleanup_NextInProgressMasterTable_Closing = "closing next in-progress master table";
        public const string Cleanup_Completed = "completed";
        public const string HydrateError_ReadingFailed = "reading failed";
        public const string HydrateError_Deserialization = "deserialization";
        public const string OnApplyRemove_Deserialization = "deserialization";
        public const string OnApplyUpdate_Deserialization = "deserialization";
        public const string OnApplyAdd_Deserialization = "deserialization";
        public const string OnUndoFalseProgress_Deserialization = "deserialization";
        public const string Readable = "readable";
        public const string Writable = "writable";
        public const string Lock_Free = "lock-free";
        public const string KeyDoesnotExist = "keydoesnotexist";
        public const string VersionDoesnotMatch = "versiondoesnotmatch";
        public const string OnSecondaryApply = "secondary";
        public const string OnRecoveryApply = "recovery";
        public const string PrimaryNotReadable = "primary not readable";
        public const string Consolidation_MergeSelected = "Consolidation merge selected";
        public const string Recovery_MergeSelected = "Recovery merge selected";
        public const string AfterRecoveryMerge = "After recovery merge";
        public const string FileMetadata_Write = "FileMetadata write";

        public const string TraceType = "TStore";

        internal static class PerformanceCounters
        {
            public const string Name = "Service Fabric TStore";
            public const string Description = "Counters for Service Fabric TStore";
            public const string Symbol = "ServiceFabricTStore";
            public const string ItemCountName = "Item Count";
            public const string ItemCountDescription = "Number of items in the store.";
            public const string ItemCountSymbol = "ItemCount";
            public const string DiskSizeName = "Disk Size";
            public const string DiskSizeDescription = "Total disk size, in bytes, of checkpoint files for the store.";
            public const string DiskSizeSymbol = "DiskSize";
            public const string CheckpointFileWriteBytesPerSecName = "Checkpoint File Write Bytes/sec";
            public const string CheckpointFileWriteBytesPerSecDescription = "Number of bytes written per second for the last checkpoint file";
            public const string CheckpointFileWriteBytesPerSecSymbol = "CheckpointFileWriteBytesPerSec";
            public const string CopyDiskTransferBytesPerSecName = "Copy Disk Transfer Bytes/sec";
            public const string CopyDiskTransferBytesPerSecDescription = "Number of disk bytes read (on primary) or written (on secondary) per second on store copy";
            public const string CopyDiskTransferBytesPerSecSymbol = "CopyDiskTransferBytesPerSec";
        }
    }
}