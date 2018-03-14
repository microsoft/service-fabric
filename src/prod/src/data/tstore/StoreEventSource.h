// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace TStore
    {
        #define DECLARE_STORE_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
        #define STORE_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RStore, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        class StoreEventSource
        {
        public:
            // File Metadata
            DECLARE_STORE_STRUCTURED_TRACE(FileMetadataDeleteKeyValue, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(FileMetadataCloseException, Common::Guid, Common::WStringLiteral, LONG64);

            // Metadata Manager
            DECLARE_STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplace, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceValidating, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceMoving, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceReplacing, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceDeleting, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);

            // Checkpoint Files
            DECLARE_STORE_STRUCTURED_TRACE(KeyCheckpointFileOpenAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(ValueCheckpointFileOpenAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);

            // Consolidation Manager
            DECLARE_STORE_STRUCTURED_TRACE(ConsolidationManagerSlowConsolidation, Common::Guid, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(ConsolidationManagerMergeAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(ConsolidationManagerMergeFile, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            
            // Recovery Store Component
            DECLARE_STORE_STRUCTURED_TRACE(RecoveryStoreComponentMergeKeyCheckpointFilesAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64);

            // Copy Manager
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerAddCopyDataAsync, Common::Guid, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationData, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationProtocol, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationMsg, Common::Guid, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessMetadataTableCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessStartKeyFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessWriteKeyFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessEndKeyFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessStartValueFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessWriteValueFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessEndValueFileCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerProcessCompleteCopyOperation, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(CopyManagerException, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::StringLiteral, LONG64);

            // Store Copy Stream
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStarting, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageVersion, Common::Guid, Common::WStringLiteral, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageMetadataTable, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkOpen, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkStart, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkWrite, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkEnd, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCompleted, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCopyStreamException, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::StringLiteral, LONG64);

            // Store
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnServiceOpenAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnServiceCloseAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOpenAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCloseAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreAbortAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreChangeRoleAsyncApi, Common::Guid, Common::WStringLiteral, int);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnCleanupAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnCleanupAsyncApiPrimeLockNotAcquired, Common::Guid, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreTryGetValueAsyncNotFound, Common::Guid, Common::WStringLiteral, LONG64, ULONG64, ULONG32, LONG64, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreAddAsync, Common::Guid, Common::WStringLiteral, LONG64, ULONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreAddAsyncError, Common::Guid, Common::WStringLiteral, ULONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsync, Common::Guid, Common::WStringLiteral, LONG64, LONG64, ULONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsyncKeyDoesNotExist, Common::Guid, Common::WStringLiteral, ULONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsyncVersionMismatch, Common::Guid, Common::WStringLiteral, ULONG64, LONG64, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditonalRemoveAsync, Common::Guid, Common::WStringLiteral, LONG64, LONG64, ULONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditionalRemoveAsyncKeyDoesNotExist, Common::Guid, Common::WStringLiteral, ULONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConditionalRemoveAsyncVersionMismatch, Common::Guid, Common::WStringLiteral, ULONG64, LONG64, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreApplyOnPrimaryAsync, Common::Guid, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreApplyOnSecondaryAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64, LONG64, ULONG64, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreApplyOnRecoveryAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64, LONG64, ULONG64, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreUndoFalseProgressOnSecondaryAsync, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreUndoFalseProgressOnSecondaryAsyncError, Common::Guid, Common::WStringLiteral, LONG64, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyAdd, Common::Guid, Common::WStringLiteral, LONG64, LONG64, ULONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyAddError, Common::Guid, Common::WStringLiteral, LONG64, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyUpdate, Common::Guid, Common::WStringLiteral, LONG64, LONG64, ULONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyUpdateError, Common::Guid, Common::WStringLiteral, LONG64, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyRemove, Common::Guid, Common::WStringLiteral, LONG64, LONG64, ULONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreOnApplyRemoveError, Common::Guid, Common::WStringLiteral, LONG64, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreAcquireKeyModificationLockAsync, Common::Guid, Common::WStringLiteral, ULONG32, ULONG64, LONG64, int);
            DECLARE_STORE_STRUCTURED_TRACE(StoreAcquireKeyReadLockAsync, Common::Guid, Common::WStringLiteral, ULONG32, ULONG64, LONG64, int, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreUnlock, Common::Guid, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StorePrepareCheckpointStartApi, Common::Guid, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StorePrepareCheckpointCompletedApi, Common::Guid, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StorePerformCheckpointAsyncStartApi, Common::Guid, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCheckpointAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StorePerformCheckpointAsyncCompletedApi, Common::Guid, Common::WStringLiteral, ULONG32, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCompleteCheckpointAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCompleteCheckpointAsyncCompletedApi, Common::Guid, Common::WStringLiteral, LONG64, LONG64, LONG64, LONG64, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRecoverCheckpointAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StorePreloadValues, Common::Guid, Common::WStringLiteral, ULONG32, bool, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRecoverCheckpointAsyncFirstCheckpoint, Common::Guid, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesStart, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesToDelete, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesDeleting, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesComplete, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreGetCurrentStateApi, Common::Guid, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreBeginSettingCurrentStateApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreEndSettingCurrentStateAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreEndSettingCurrentStateAsyncAbortedApi, Common::Guid, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRemoveStateAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRecoverCopyStateAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncError, Common::Guid, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncFile, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncApi, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncError, Common::Guid, Common::WStringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncFile, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreCreateKeyEnumeratorAsync, Common::Guid, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRebuildNotificationStarting, Common::Guid, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreRebuildNotificationCompleted, Common::Guid, Common::WStringLiteral, INT64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreSweep, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreException, Common::Guid, Common::WStringLiteral, Common::WStringLiteral, Common::StringLiteral, LONG64);
            DECLARE_STORE_STRUCTURED_TRACE(StoreThrowIfNotWritable, Common::Guid, Common::WStringLiteral, LONG64, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreThrowIfNotReadable, Common::Guid, Common::WStringLiteral, LONG64, ULONG32, ULONG32);
            DECLARE_STORE_STRUCTURED_TRACE(StoreConstructor, Common::Guid, Common::WStringLiteral, Common::WStringLiteral);
            DECLARE_STORE_STRUCTURED_TRACE(StoreDestructor, Common::Guid, Common::WStringLiteral);


            StoreEventSource() :
                // Note: TraceId starts from 4, this is because 0-3 has already been defined as trace type: Info, Warning, Error, Noise.

                // File Metadata
                STORE_STRUCTURED_TRACE(FileMetadataDeleteKeyValue, 4, Info, "{1}: Deleting key file: {2} and value file: {3}", "id", "TraceTag", "KeyFilename", "ValueFilename"),
                STORE_STRUCTURED_TRACE(FileMetadataCloseException, 5, Warning, "{1}: Failed on closing filemetadata with status {2}", "id", "TraceTag", "status"),

                // Metadata Manager
                STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplace, 6, Info, "{1}: phase={2}", "id", "TraceTag", "Phase"),
                STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceValidating, 7, Info, "{1}: validating tmpFilePath {2}", "id", "TraceTag", "Filepath"),
                STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceMoving, 8, Info, "{1}: move {2} to {3}", "id", "TraceTag", "Phase", "Src", "Dest"),
                STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceReplacing, 9, Info, "{1}: replace {2} to {3}", "id", "TraceTag", "Dest", "Src"),
                STORE_STRUCTURED_TRACE(MetadataManagerSafeFileReplaceDeleting, 10, Info, "{1}: deleting {2}", "id", "TraceTag", "File"),

                // Checkpoint Files
                STORE_STRUCTURED_TRACE(KeyCheckpointFileOpenAsync, 11, Info, "{1}: filename={2} state={3}", "id", "TraceTag", "Filename", "State"),
                STORE_STRUCTURED_TRACE(ValueCheckpointFileOpenAsync, 12, Info, "{1}: filename={2} state={3}", "id", "TraceTag", "Filename", "State"),

                // Consolidation Manager
                STORE_STRUCTURED_TRACE(ConsolidationManagerMergeAsync, 13, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(ConsolidationManagerMergeFile, 14, Info, "{1}: Merge filename={2}", "id", "TraceTag", "Filename"),
                STORE_STRUCTURED_TRACE(ConsolidationManagerSlowConsolidation, 15, Warning, "{1}: high delta count {2} and the default count is {3} indicating slow consolidation", "id", "TraceTag", "Index", "DefaultNumber"),

                // Recovery Store Component
                STORE_STRUCTURED_TRACE(RecoveryStoreComponentMergeKeyCheckpointFilesAsync, 16, Noise, "{1}: Merge keyfiles. phase={2} count={3}", "id", "TraceTag", "Phase", "Count"),

                // Copy Manager
                STORE_STRUCTURED_TRACE(CopyManagerAddCopyDataAsync, 17, Info, "{1}: Received bytes={2}", "id", "TraceTag", "ReceivedBytes"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationData, 18, Info, "{1}: directory={2} bytes={3}", "id", "TraceTag", "Directory", "ReceivedBytes"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationProtocol, 19, Info, "{1}: directory={2} version={3}", "id", "TraceTag", "Direcotry", "Version"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessVersionCopyOperationMsg, 20, Info, "{1}: Unknown copy protocol version={2}", "id", "TraceTag", "Version"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessMetadataTableCopyOperation, 21, Info, "{1}: directory={2} bytes={3}", "id", "TraceTag", "Directory", "ReceivedBytes"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessStartKeyFileCopyOperation, 22, Info, "{1}: directory={2} filename={3} bytes={4} fileid={5}", "id", "TraceTag", "Directory", "Filename", "ReceivedBytes", "FileId"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessWriteKeyFileCopyOperation, 23, Info, "{1}: directory={2} filename={3} bytes={4}", "id", "TraceTag", "Directory", "Filename", "ReceivedBytes"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessEndKeyFileCopyOperation, 24, Info, "{1}: directory={2} filename={3} filesize={4}", "id", "TraceTag", "Directory", "Filename", "FileSize"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessStartValueFileCopyOperation, 25, Info, "{1}: directory={2} filename={3} bytes={4} fileid={5}", "id", "TraceTag", "Directory", "Filename", "ReceivedBytes", "FileId"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessWriteValueFileCopyOperation, 26, Info, "{1}: directory={2} filename={3} bytes={4}", "id", "TraceTag", "Directory", "Filename", "ReceivedBytes"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessEndValueFileCopyOperation, 27, Info, "{1}: directory={2} filename={3} filesize={4}", "id", "TraceTag", "Directory", "Filename", "FileSize"),
                STORE_STRUCTURED_TRACE(CopyManagerProcessCompleteCopyOperation, 28, Info, "{1}: directory={2}", "id", "TraceTag", "Directory"),
                STORE_STRUCTURED_TRACE(CopyManagerException, 29, Warning, "{1}: UnexpectedException: message={2} error code={4}\nStack: {3}", "id", "TraceTag", "Message", "StackTrace", "ErrorCode"),

                // Store Copy Stream
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStarting, 30, Info, "{1}: starting directory={2}", "id", "TraceTag", "Directory"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageVersion, 31, Info, "{1}: version={2} bytes={3}", "id", "TraceTag", "Version", "Bytes"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageMetadataTable, 32, Info, "{1}: directory={2} bytes={3}", "id", "TraceTag", "Directory", "Bytes"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkOpen, 33, Info, "{1}: Opening file={2}", "id", "TraceTag", "File"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkStart, 34, Info, "{1}: Starting file={2} operation={3} bytes={4} fileid={5}", "id", "TraceTag", "File", "Operation", "Bytes", "FileId"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkWrite, 35, Info, "{1}: Writing file={2} operation={3} bytes={4}", "id", "TraceTag", "File", "Operation", "Bytes"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCheckpointChunkEnd, 36, Info, "{1}: Ending file={2} operation={3}", "id", "TraceTag", "File", "Operation"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamCopyStageCompleted, 37, Info, "{1}: directory={2}", "id", "TraceTag", "Directory"),
                STORE_STRUCTURED_TRACE(StoreCopyStreamException, 38, Warning, "{1}: UnexpectedException: Message: {2} Error Code: {4}\nStack: {3}", "id", "TraceTag", "Message", "StackTrace", "ErrorCode"),

                // Store
                STORE_STRUCTURED_TRACE(StoreConstructor, 100, Info, "{1}: ctor name={2}", "id", "TraceTag", "Name"),
                STORE_STRUCTURED_TRACE(StoreDestructor, 101, Info, "{1}: dtor", "id", "TraceTag"),
                STORE_STRUCTURED_TRACE(StoreOnServiceOpenAsync, 102, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreOnServiceCloseAsync, 103, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreOpenAsyncApi, 104, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreCloseAsyncApi, 105, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreAbortAsyncApi, 106, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreChangeRoleAsyncApi, 107, Info, "{1}: completed role={2}", "id", "TraceTag", "Role"),
                STORE_STRUCTURED_TRACE(StoreOnCleanupAsyncApi, 108, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreTryGetValueAsyncNotFound, 109, Noise, "{1}: lock-free txn={2} key={3} isolation={4} lsn={5} reason={6}", "id", "TraceTag", "Transaction", "Key", "IsolationLevel", "LSN", "Reason"),
                STORE_STRUCTURED_TRACE(StoreAddAsync, 110, Info, "{1}: replicated txn={2} key={3}", "id", "TraceTag", "Transaction", "Key"),
                STORE_STRUCTURED_TRACE(StoreAddAsyncError, 111, Warning, "{1}: key={2} cannot be added in txn={3}", "id", "TraceTag", "Key", "Transaction"),
                STORE_STRUCTURED_TRACE(StoreConditonalRemoveAsync, 112, Info, "{1}: replicated lsn={2} txn={3} key={4}", "id", "TraceTag", "LSN", "Transaction", "Key"),
                STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsync, 113, Info, "{1}: replicated lsn={2} txn={3} key={4}", "id", "TraceTag", "LSN", "Transaction", "Key"),
                STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsyncKeyDoesNotExist, 114, Info, "{1}: key={2} cannot be updated in txn={3}. Key does not exist", "id", "TraceTag", "Key", "Transaction"),
                STORE_STRUCTURED_TRACE(StoreConditionalUpdateAsyncVersionMismatch, 115, Warning, "{1}: key={2} cannot be updated in txn={3}. Version mismatch. Current: {4} != Conditional: {5}", "id", "TraceTag", "Key", "Transaction", "CurrentLSN", "ConditionalLSN"),
                STORE_STRUCTURED_TRACE(StoreConditionalRemoveAsyncKeyDoesNotExist, 116, Info, "{1}: key={2} cannot be removed in txn={3}. Key does not exist", "id", "TraceTag", "Key", "Transaction"),
                STORE_STRUCTURED_TRACE(StoreConditionalRemoveAsyncVersionMismatch, 117, Warning, "{1}: key={2} cannot be removed in txn={3}. Version mismatch. Current: {4} != Conditional: {5}", "id", "TraceTag", "Key", "Transaction", "CurrentLSN", "ConditionalLSN"),
                STORE_STRUCTURED_TRACE(StoreApplyOnPrimaryAsync, 119, Info, "{1}: lsn={2} txn={3}", "id", "TraceTag", "LSN", "Transaction"),
                STORE_STRUCTURED_TRACE(StoreApplyOnSecondaryAsync, 120, Info, "{1}: {2} lsn={3} txn={4} key={5} type={6}", "id", "TraceTag", "Message", "LSN", "Transaction", "Key", "ModificationType"),
                STORE_STRUCTURED_TRACE(StoreApplyOnRecoveryAsync, 121, Info, "{1}: {2} lsn={3} txn={4} key={5} type={6}", "id", "TraceTag", "Message", "LSN", "Transaction", "Key", "ModificationType"),
                STORE_STRUCTURED_TRACE(StoreUndoFalseProgressOnSecondaryAsync, 122, Info, "{1}: {2} lsn={3}, txn={4}", "id", "TraceTag", "Message", "LSN", "Transaction"),
                STORE_STRUCTURED_TRACE(StoreUndoFalseProgressOnSecondaryAsyncError, 123, Info, "{1}: txn={2} reason={3}", "id", "TraceTag", "Transaction", "Reason"),
                STORE_STRUCTURED_TRACE(StoreOnApplyAdd, 124, Info, "{1}: OnApplyAdd lsn={2} txn={3} key={4} count={5}", "id", "TraceTag", "LSN", "Transaction", "Key", "Count"),
                STORE_STRUCTURED_TRACE(StoreOnApplyAddError, 125, Error, "{1}: txn={2} reason={3}", "id", "TraceTag", "Transaction", "Reason"),
                STORE_STRUCTURED_TRACE(StoreOnApplyUpdate, 126, Info, "{1}: OnApplyUpdate lsn={2} txn={3} key={4}", "id", "TraceTag", "LSN", "Transaction", "Key"),
                STORE_STRUCTURED_TRACE(StoreOnApplyUpdateError, 127, Error, "{1}: txn={2} reason={3}", "id", "TraceTag", "Transaction", "Reason"),
                STORE_STRUCTURED_TRACE(StoreOnApplyRemove, 128, Info, "{1}: OnApplyRemove lsn={2} txn={3} key={4} count={5}", "id", "TraceTag", "LSN", "Transaction", "Key", "Count"),
                STORE_STRUCTURED_TRACE(StoreOnApplyRemoveError, 129, Error, "{1}: txn={2} reason={3}", "id", "TraceTag", "Transaction", "Reason"),
                STORE_STRUCTURED_TRACE(StoreAcquireKeyModificationLockAsync, 130, Noise, "{1}: AcquireKeyModificationLockAsync acquire lock={2} key={3} txn={4} timeout={5}", "id", "TraceTag", "LockMode", "Key", "Transaction", "Timeout"),
                STORE_STRUCTURED_TRACE(StoreAcquireKeyReadLockAsync, 131, Noise, "{1}: AcquireKeyReadLockAsync acquire lock={2} key={3} txn={4} timeout={5} isolation={6}", "id", "TraceTag", "LockMode", "Key", "Transaction", "Timeout", "Isolation"),
                STORE_STRUCTURED_TRACE(StoreUnlock, 132, Noise, "{1}: Unlock starting txn={2}", "id", "TraceTag", "Transaction"),
                STORE_STRUCTURED_TRACE(StorePrepareCheckpointStartApi, 133, Info, "{1}: prepare checkpoint starting. checkpoint lsn={2}", "id", "TraceTag", "LSN"),
                STORE_STRUCTURED_TRACE(StorePrepareCheckpointCompletedApi, 134, Info, "{1}: count={2}", "id", "TraceTag", "Count"),
                STORE_STRUCTURED_TRACE(StorePerformCheckpointAsyncStartApi, 135, Info, "{1}: perfom checkpoint starting. last prepare checkpoint lsn={2}; checkpoint lsn={3}", "id", "TraceTag", "LastPrepareLSN", "CheckpointLSN"),
                STORE_STRUCTURED_TRACE(StoreCheckpointAsyncApi, 136, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StorePerformCheckpointAsyncCompletedApi, 137, Info, "{1}: last fileid={2} duration={3} ms", "id", "TraceTag", "FileId", "Duration"),
                STORE_STRUCTURED_TRACE(StoreCompleteCheckpointAsyncApi, 138, Info, "{1}: complete checkpoint {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreCompleteCheckpointAsyncCompletedApi, 139, Info, "{1}: total={2} ms replace={3} ms swap={4} ms computeToBeDeleted={5} ms delete={5} ms; checkpointed={6} bytes", "id", "TraceTag", "TotalTime", "ReplaceTime", "SwapTime", "ComputeDeletedTime", "DeleteTime", "Size"),
                STORE_STRUCTURED_TRACE(StoreRecoverCheckpointAsyncApi, 140, Info, "{1}: {2} count={3} recovered={4} bytes lsn={5}", "id", "TraceTag", "Message", "Count", "Size", "LSN"),
                STORE_STRUCTURED_TRACE(StoreRecoverCheckpointAsyncFirstCheckpoint, 141, Info, "{1}: creating first checkpoint", "id", "TraceTag"),
                STORE_STRUCTURED_TRACE(StorePreloadValues, 142, Info, "{1}: loading values as part of recovery. parallelism={2} starting={3} duration={4} ms", "id", "TraceTag", "Parallelism", "IsStarting", "DurationInMs"),
                STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesStart, 143, Info, "{1}: start. directory={2}", "id", "TraceTag", "Directory"),
                STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesToDelete, 144, Info, "{1}: found file to delete={2}", "id", "TraceTag", "TableFile"),
                STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesDeleting, 145, Info, "{1}: deleting file={2}", "id", "TraceTag", "TableFile"),
                STORE_STRUCTURED_TRACE(StoreMetadataTableTrimFilesComplete, 146, Info, "{1}: complete. directory={2}", "id", "TraceTag", "Directory"),
                STORE_STRUCTURED_TRACE(StoreGetCurrentStateApi, 147, Info, "{1}: starting", "id", "TraceTag"),
                STORE_STRUCTURED_TRACE(StoreBeginSettingCurrentStateApi, 148, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreEndSettingCurrentStateAsyncApi, 149, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreEndSettingCurrentStateAsyncAbortedApi, 150, Warning, "{1}: Copy Aborted", "id", "TraceTag"),
                STORE_STRUCTURED_TRACE(StoreRemoveStateAsyncApi, 151, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreRecoverCopyStateAsyncApi, 152, Info, "{1}: phase={2} count={3}", "id", "TraceTag", "Phase", "Count"),
                STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncApi, 153, Info, "{1}: folder={2} {3}", "id", "TraceTag", "Folder", "Message"),
                STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncError, 154, Error, "{1}: failed with error status {2}", "id", "TraceTag", "Status"),
                STORE_STRUCTURED_TRACE(StoreBackupCheckpointAsyncFile, 155, Info, "{1}: folder={2} backing up file {3} to {4}", "id", "TraceTag", "Folder", "Source", "Destination"),
                STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncApi, 156, Info, "{1}: folder={2} {3}", "id", "TraceTag", "Folder", "Message"),
                STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncFile, 157, Info, "{1}: folder={2} restoring file {3} to {4}", "id", "TraceTag", "Folder", "Source", "Destination"),
                STORE_STRUCTURED_TRACE(StoreRestoreCheckpointAsyncError, 158, Error, "{1}: failed with error status {2}", "id", "TraceTag", "Status"),
                STORE_STRUCTURED_TRACE(StoreCreateKeyEnumeratorAsync, 159, Info, "{1}: Txn: {1} VisibilitySequenceNumber: {2}", "id", "TraceTag", "Transaction", "VisibilitySequenceNumber"),
                STORE_STRUCTURED_TRACE(StoreRebuildNotificationStarting, 160, Info, "{1}: starting", "id", "TraceTag"),
                STORE_STRUCTURED_TRACE(StoreRebuildNotificationCompleted, 161, Info, "{1}: completed; duration: {2} ms", "id", "TraceTag", "Duration"),
                STORE_STRUCTURED_TRACE(StoreSweep, 162, Info, "{1}: {2}", "id", "TraceTag", "Message"),
                STORE_STRUCTURED_TRACE(StoreException, 163, Warning, "{1}: UnexpectedException: Message: {2} Code:{4}\nStack: {3}", "id", "TraceTag", "Message", "StackTrace", "ErrorCode"),
                STORE_STRUCTURED_TRACE(StoreThrowIfNotWritable, 164, Warning, "{1}: txn={2} status={3} role={4}", "id", "TraceTag", "Transaction", "Status", "Role"),
                STORE_STRUCTURED_TRACE(StoreThrowIfNotReadable, 165, Warning, "{1}: txn={2} status={3} role={4}", "id", "TraceTag", "Transaction", "Status", "Role"),
                STORE_STRUCTURED_TRACE(StoreOnCleanupAsyncApiPrimeLockNotAcquired, 166, Warning, "{1}: timed out trying to acquire prime lock", "id", "TraceTag")
            {
            }
            static Common::Global<StoreEventSource> Events;
        };
    }
}
