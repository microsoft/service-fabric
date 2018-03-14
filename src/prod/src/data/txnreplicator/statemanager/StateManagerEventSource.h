// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
#define DECLARE_SM_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define SM_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::SM, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        // Structured trace reserved areas.
        //  1. TraceType: [0, 3]
        //  2. MetadataManager [4, 25]
        //  3. NamedOperationDataStream [26, 30]
        //  4. CheckpointManager [31, 60]
        //  5. CheckpointFile [61, 70]
        //  6. StateManager [71, 145]
        //  7. SM_Dispatch [146, 150]
        //  8. ApiDispatcher [151, 160]
        //  9. ISPM [161, 180]
        class StateManagerEventSource
        {
        public:
            // MetadataManager
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_LockForRead, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64, Common::TimeSpan);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_LockForWrite, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64, Common::TimeSpan);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_MoveToDeleted, Common::Guid, LONG64, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_RemoveAbort, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_RemoveCommit, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_RemoveLock, Common::Guid, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(MetadataManager_Error, Common::Guid, LONG64, std::wstring, LONG64);

            // NamedOperationDataStream
            DECLARE_SM_STRUCTURED_TRACE(NamedOperationDataStreamError, Common::Guid, LONG64, std::wstring, LONG64);

            // CheckpointManager
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerPerformCheckpoint, Common::Guid, LONG64, ULONGLONG, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointNotRename, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointFinished, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerRecover, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerRecoverSummary, Common::Guid, LONG64, ULONGLONG, ULONGLONG, ULONGLONG);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerNullState, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerNotNullState, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerGetStateSummary, Common::Guid, LONG64, int64, int64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerRestoreCheckpointStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerRestoreCheckpointEnd, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointManagerError, Common::Guid, LONG64, std::wstring, LONG64);

            // CheckpointFile
            DECLARE_SM_STRUCTURED_TRACE(CheckpointFileError, Common::Guid, LONG64, std::wstring, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CheckpointFileWriteComplete, Common::Guid, LONG64, int);
            DECLARE_SM_STRUCTURED_TRACE(
                CheckpointFile_GetCurrentStateStart,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_REPLICA_ID,
                SerializationMode::Trace,
                ULONG32,                            // Copy Protocol Version
                ULONG32);                           // SizeInBytes
            DECLARE_SM_STRUCTURED_TRACE(
                CheckpointFile_GetCurrentState, 
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_REPLICA_ID,
                SerializationMode::Trace,
                int,                                // OperationData index
                ULONG32,                            // SizeInBytes
                std::wstring);                      // State providers and their mode.

            // StateManager
            DECLARE_SM_STRUCTURED_TRACE(OpenAsync, Common::Guid, LONG64, bool, bool);
            DECLARE_SM_STRUCTURED_TRACE(ChangeRoleAsync, Common::Guid, LONG64, int, int);
            DECLARE_SM_STRUCTURED_TRACE(CloseAsync, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(GetOrAddUpdateId, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(AddUpdateId, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(PrepareCheckpointStart, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(PrepareCheckpointCompleted, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(PerformCheckpointStart, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(PerformCheckpointCompleted, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CompleteCheckpointStart, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CompleteCheckpointCompleted, Common::Guid, LONG64, LONG64);

            DECLARE_SM_STRUCTURED_TRACE(
            ApplyAsyncStart, 
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER, 
                int,                        // ApplyContext
                LONG64,                     // Transaction Id
                FABRIC_SEQUENCE_NUMBER);    // CommitSequenceNumber

            DECLARE_SM_STRUCTURED_TRACE(
            ApplyAsyncFinish, 
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER, 
                int,                        // ApplyContext
                LONG64,                     // Transaction Id
                FABRIC_SEQUENCE_NUMBER,     // CommitSequenceNumber
                FABRIC_STATE_PROVIDER_ID,   // StateProviderId
                Common::WStringLiteral,     // StateProviderName
                Common::WStringLiteral);    // OperationType: Insert/Delete

            DECLARE_SM_STRUCTURED_TRACE(
            ApplyAsyncNoop, 
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER,
                int,                        // ApplyContext
                LONG64,                     // Transaction Id
                FABRIC_SEQUENCE_NUMBER,     // CommitSequenceNumber
                FABRIC_STATE_PROVIDER_ID,   // StateProviderId
                Common::WStringLiteral,     // OperationType: Insert/Delete
                bool);                      // IsStateProviderDeleted

            DECLARE_SM_STRUCTURED_TRACE(UnlockOnLocalState, Common::Guid, LONG64, LONG64);

            DECLARE_SM_STRUCTURED_TRACE(
            ApplyOnStaleStateProvider, 
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER, 
                LONG64,                     // Transaction Id
                FABRIC_STATE_PROVIDER_ID);  // StateProviderId

            DECLARE_SM_STRUCTURED_TRACE(ServiceOpen, Common::Guid, LONG64, bool, bool);
            DECLARE_SM_STRUCTURED_TRACE(ServiceOpenCompleted, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(ServiceCloseStarted, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(ServiceCloseCompleted, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(OpenSP, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(ChangeRoleSP, Common::Guid, LONG64, int);
            DECLARE_SM_STRUCTURED_TRACE(ChangeRoleToNone, Common::Guid, LONG64, int);
            DECLARE_SM_STRUCTURED_TRACE(RecoverSPStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RecoverSP, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveSP, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RecoverSMCheckpointEmpty, Common::Guid, LONG64, int64);
            DECLARE_SM_STRUCTURED_TRACE(RecoverSMCheckpoint, Common::Guid, LONG64, int64, int64);
            DECLARE_SM_STRUCTURED_TRACE(BeginSetState, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(SetCurrentState, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(CopyState, Common::Guid, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(GetChildren, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(AddSingle, Common::Guid, LONG64, LONG64, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(AddSingleErr, Common::Guid, LONG64, LONG64, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveSingleErr, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveSingle, Common::Guid, LONG64, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveSingleNotExist, Common::Guid, LONG64, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveSingleInvariant, Common::Guid, LONG64, LONG64, Common::WStringLiteral, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(AddOperation, Common::Guid, LONG64, ULONG32, LONG64, ULONG32);
            DECLARE_SM_STRUCTURED_TRACE(NotRegistered, Common::Guid, LONG64, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(NotRegisteredDeleting, Common::Guid, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(PerformCheckpointSP, Common::Guid, LONG64, ULONGLONG);
            DECLARE_SM_STRUCTURED_TRACE(DeleteSPWorkDirectory, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(Cleanup, Common::Guid, LONG64, int);
            DECLARE_SM_STRUCTURED_TRACE(CleanupDelete, Common::Guid, LONG64, LONG64, int);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSP, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSPMark, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSPDelete, Common::Guid, LONG64, Common::WStringLiteral);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSPDeleteErr, Common::Guid, LONG64, Common::WStringLiteral, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSPTooLong, Common::Guid, LONG64, int64);
            DECLARE_SM_STRUCTURED_TRACE(RemoveUnreferencedSPCompleted, Common::Guid, LONG64, Common::WStringLiteral, int64);
            DECLARE_SM_STRUCTURED_TRACE(GetCurrentState, Common::Guid, FABRIC_REPLICA_ID, FABRIC_REPLICA_ID);
            DECLARE_SM_STRUCTURED_TRACE(EndSettingCurrentState, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(Ctor, Common::Guid, LONG64, Common::WStringLiteral, int);
            DECLARE_SM_STRUCTURED_TRACE(Dtor, Common::Guid, LONG64, Common::WStringLiteral, int);
            DECLARE_SM_STRUCTURED_TRACE(BackupStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(BackupEnd, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RestoreStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RestoreEnd, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RestoreStateProviderStart, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(RestoreStateProviderEnd, Common::Guid, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(Error, Common::Guid, LONG64, std::wstring, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(Warning, Common::Guid, LONG64, std::wstring);
            DECLARE_SM_STRUCTURED_TRACE(ReplicationError, Common::Guid, LONG64, LONG64, LONG64, LONG64);
            DECLARE_SM_STRUCTURED_TRACE(ReadStatusNotGranted, Common::Guid, LONG64, Common::WStringLiteral, int, bool, int);

            // Dispatching the state providers from SM'
            DECLARE_SM_STRUCTURED_TRACE(
            Dispatch_CloseAsync,
                Common::Guid, FABRIC_REPLICA_ID,
                LONG64,                     // Number of active state providers
                LONG64,                     // Number of deleted state providers
                LONG64,                     // Time in ms
                LONG64);                    // status

            DECLARE_SM_STRUCTURED_TRACE(
            Dispatch_Abort,
                Common::Guid, FABRIC_REPLICA_ID,
                LONG64,                     // Number of active state providers
                LONG64,                     // Number of deleted state providers
                LONG64);                    // Time in ms

            //  8. ApiDispatcher [151, 160]
            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                Common::WStringLiteral,     // Function name
                LONG64);                    // Error code (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_Factory_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                Common::WStringLiteral,     // State Provider Name
                Common::WStringLiteral,     // Type Name
                LONG64);                    // Error Code. (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_Initialize_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                Common::WStringLiteral,     // Work Folder
                ULONG32,                    // Number of children  
                LONG64);                    // Error Code. (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_ChangeRoleAsync_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                LONG32,                     // Role
                LONG64);                    // Error Code. (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_SetCurrentStateAsync_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                LONG64,                     // State Record Number
                ULONG32,                    // OperationData Array Segment count
                LONG64);                    // Error Code. (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_PrepareCheckpoint_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                FABRIC_SEQUENCE_NUMBER,     // Checkpoint LSN
                LONG64);                    // Error Code. (0 is not valid)

            DECLARE_SM_STRUCTURED_TRACE(
            ISP2_PrepareForRemoveAsync_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                LONG64,                     // Transaction Id
                LONG64);                    // Error Code. (0 is not valid)

            // ISPM API
            DECLARE_SM_STRUCTURED_TRACE(
            ISPM_ApiError,
                Common::Guid, FABRIC_REPLICA_ID, FABRIC_STATE_PROVIDER_ID,
                Common::StringLiteral,      // API Name
                Common::WStringLiteral,     // State provider name
                LONG64,                     // Transaction Id
                LONG64);                    // Error Code. (0 is not valid)

            StateManagerEventSource() :
                // Structured trace reserved areas.
                //  1. TraceType:                   [0  , 3]
                //  2. MetadataManager              [4  , 25]
                //  3. NamedOperationDataStream     [26 , 30]
                //  4. CheckpointManager            [31 , 60]
                //  5. CheckpointFile               [61 , 70]
                //  6. StateManager                 [71 , 145]
                //  7. SM_Dispatch                  [146, 150]
                //  8. ApiDispatcher                [151, 160]
                //  9. ISPM                         [161, 180]
                // Note: TranceId starts from 4, this is because 0-3 has already been defined as trace type: Info, Warning, Error, Noise.
                // To move the partition id to the type section, format like DSM.MetadataManagerLockForRead@{PartitionId}, the first parameter PartitionId should mark as "id".
                // MetadataManager
                SM_STRUCTURED_TRACE(MetadataManager_LockForRead, 4, Noise, "{1}: Component: MetadataManager LockForReadAsync: key: {2} SPID: {3} TxnID: {4} Timeout: {5} ", "id", "ReplicaId", "Key", "SPId", "TxnId", "Timeout"),
                SM_STRUCTURED_TRACE(MetadataManager_LockForWrite, 5, Info, "{1}: Component: MetadataManager LockForWriteAsync: key: {2} SPID: {3} TxnID: {4} Timeout: {5}", "id", "ReplicaId", "Key", "SPId", "TxnId", "Timeout"),
                SM_STRUCTURED_TRACE(MetadataManager_MoveToDeleted, 6, Info, "{1}: Component: MetadataManager Added state provider to delete list. State Provider Id: {2}. State Provider Name: {3}", "id", "ReplicaId", "SPId", "Key"),
                SM_STRUCTURED_TRACE(MetadataManager_RemoveAbort, 7, Info, "{1}: Component: MetadataManager Unlock: State provider is not transient delete. Key: {2} SPID: {3} TxnID: {4}", "id", "ReplicaId", "Key", "SPId", "TxnId"),
                SM_STRUCTURED_TRACE(MetadataManager_RemoveCommit, 8, Info, "{1}: Component: MetadataManager Unlock: State provider in deleted list. Key: {2} SPID: {3} TxnID: {4}", "id", "ReplicaId", "Key", "SPId", "TxnId"),
                SM_STRUCTURED_TRACE(MetadataManager_RemoveLock, 9, Info, "{1}: Component: MetadataManager RemoveLock: key: {2} TxnID: {3}", "id", "ReplicaId", "Key", "TxnId"),
                SM_STRUCTURED_TRACE(MetadataManager_Error, 10, Error, "{1}: {2} Component: MetadataManager ErrorCode: {3}", "id", "ReplicaId", "message", "errorCode"),

                // NamedOperationDataStream
                SM_STRUCTURED_TRACE(NamedOperationDataStreamError, 26, Error, "{1}: {2} ErrorCode: {3}", "id", "ReplicaId", "message", "errorCode"),

                // CheckpointManager
                SM_STRUCTURED_TRACE(CheckpointManagerPerformCheckpoint, 31, Info, "{1}: PerformCheckpoint: State Provider count: {2}. New checkpoint size {3} bytes.", "id", "ReplicaId", "CollectionCount", "FileSize"),
                SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointStart, 32, Info, "{1}: CompleteCheckpointAsync: called", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointNotRename, 33, Info, "{1}: CompleteCheckpointAsync: Rename not required", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CheckpointManagerCompleteCheckpointFinished, 34, Info, "{1}: CompleteCheckpointAsync: completed", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CheckpointManagerRecover, 35, Info, "{1}: RecoverCheckpointAsync: Checkpoint File: {2}", "id", "ReplicaId", "Path"),
                SM_STRUCTURED_TRACE(CheckpointManagerRecoverSummary, 36, Info, "{1}: Recovered checkpoint count: {2} active: {3} deleted: {4}", "id", "ReplicaId", "Count", "ActiveCount", "DeletedCount"),
                SM_STRUCTURED_TRACE(CheckpointManagerNullState, 37, Info, "{1}: State Provider {2} returned null copy operation data stream", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(CheckpointManagerNotNullState, 38, Info, "{1}: State Provider {2} returned not null copy stream", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(CheckpointManagerGetStateSummary, 39, Info, "{1}: GetCurrentState completed. Total: {2} ms GetOrderedMetadataArray: {3} ms", "id", "ReplicaId", "TotalTime", "Time"),
                SM_STRUCTURED_TRACE(CheckpointManagerRestoreCheckpointStart, 40, Info, "{1}: Api.RestoreCheckpointAsync starts.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CheckpointManagerRestoreCheckpointEnd, 41, Info, "{1}: Api.RestoreCheckpointAsync ends.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CheckpointManagerError, 42, Error, "{1}: {2} ErrorCode: {3}", "id", "ReplicaId", "message", "errorCode"),

                // CheckpointFile 
                SM_STRUCTURED_TRACE(CheckpointFileError, 61, Error, "{1}: {2}, ErrorCode: {3}", "id", "ReplicaId", "message", "errorCode"),
                SM_STRUCTURED_TRACE(CheckpointFileWriteComplete, 62, Info, "{1}: CheckpointFile WriteAsync complete, SerailizationMode: {2}.", "id", "ReplicaId", "SerailizationMode"),
                SM_STRUCTURED_TRACE(CheckpointFile_GetCurrentStateStart, 63, Info,
                    "{1}: CheckpointFile::GetCopyData. TargetReplicaId: {2} SerailizationMode: {3} CopyProtocol Version: {4} Size: {5} bytes",
                    "id", "replicaID", "targetReplicaId", "serializationMode", "version", "sizeInBytes"),
                SM_STRUCTURED_TRACE(CheckpointFile_GetCurrentState, 64, Info, 
                    "{1}: CheckpointFile::GetCopyData. TargetReplicaId: {2} SerailizationMode: {3} Index: {4} Size: {5} bytes\r\n{6}", 
                    "id", "replicaID", "targetReplicaId", "serializationMode", "index", "sizeInBytes", "message"),

                // StateManager
                SM_STRUCTURED_TRACE(OpenAsync, 71, Info, "{1}: Api.OpenAsync started: CompleteCheckpoint: {2} CleanupRestore: {3}.", "id", "ReplicaId", "CompleteCheckpoint", "Cleanup"),
                SM_STRUCTURED_TRACE(ChangeRoleAsync, 72, Info, "{1}: Api.ChangeRoleAsync started: CurrentRole: {2} NewRole: {3}.", "id", "ReplicaId", "Role", "NewRole"),
                SM_STRUCTURED_TRACE(CloseAsync, 73, Info, "{1}: APi.CloseAsync started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(GetOrAddUpdateId, 74, Info, "{1}: Change state provider id to {2}", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(AddUpdateId, 75, Info, "{1}: Change state provider id to {2}", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(PrepareCheckpointStart, 76, Info, "{1}: Api.PrepareCheckpointAsync.Start: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(PrepareCheckpointCompleted, 77, Info, "{1}: Api.PrepareCheckpointAsync.Completed: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(PerformCheckpointStart, 78, Info, "{1}: Api.PerformCheckpointAsync.Start: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(PerformCheckpointCompleted, 79, Info, "{1}: Api.PerformCheckpointAsync.Completed: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(CompleteCheckpointStart, 80, Info, "{1}: Api.CompleteCheckpointAsync.Start: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(CompleteCheckpointCompleted, 81, Info, "{1}: Api.CompleteCheckpointAsync.Completed: CheckpointLSN {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(ApplyAsyncStart, 82, Noise,
                    "{1}: ApplyAsync: ApplyLSN: {2} ApplyContext: {3:x} TransactionId: {4} CommitLSN: {5}",
                    "id", "ReplicaId", "ApplyLSN", "ApplyContext", "TxnId", "CommitLSN"),
                SM_STRUCTURED_TRACE(ApplyAsyncFinish, 83, Info, 
                    "{1}: ApplyAsync: ApplyLSN: {2} ApplyContext: {3:x} TransactionId: {4} CommitLSN: {5} SPId: {6} SPName: {7} ApplyType: {8}", 
                    "id", "ReplicaId", "ApplyLSN", "ApplyContext", "TxnId", "CommitLSN", "SPId", "SPName", "ApplyType"),
                SM_STRUCTURED_TRACE(ApplyAsyncNoop, 84, Info,
                    "{1}: ApplyAsync: ApplyLSN: {2} ApplyContext: {3:x} TransactionId: {4} CommitLSN: {5} SPId: {6} ApplyType: {7} IsSPDeleted: {8}",
                    "id", "ReplicaId", "ApplyLSN", "ApplyContext", "TxnId", "CommitLSN", "SPId", "ApplyType", "IsDeleted"),

                // StateManager trace gap (84-91) is available 
                SM_STRUCTURED_TRACE(UnlockOnLocalState, 91, Info, "{1}: Unlock (SM): TransactionId: {2}", "id", "ReplicaId", "TxnId"),
                SM_STRUCTURED_TRACE(ApplyOnStaleStateProvider, 92, Info, 
                    "{1}: ApplyAsync: ApplyLSN: {2} TxnId: {3} SPId: {4}", 
                    "id", "ReplicaId", "LSN", "TxnId", "SPId"),
                SM_STRUCTURED_TRACE(ServiceOpen, 93, Info, "{1}: OnServiceOpenAsync: started. CompleteCheckpoint: {2} CleanupRestore: {3}", "id", "ReplicaId", "CompleteCheckpoint", "Cleanup"),
                SM_STRUCTURED_TRACE(ServiceOpenCompleted, 94, Info, "{1}: OnServiceOpenAsync: completed.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(ServiceCloseStarted, 95, Info, "{1}: OnServiceCloseAsync: started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(ServiceCloseCompleted, 96, Info, "{1}: OnServiceCloseAsync: completed.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(OpenSP, 97, Info, "{1}: Dispatcher.OpenAsync started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(ChangeRoleSP, 98, Info, "{1}: Dispatcher.ChangeRoleAsync started. Role: {2}", "id", "ReplicaId", "NewRole"),
                SM_STRUCTURED_TRACE(ChangeRoleToNone, 99, Info, "{1}: Dispatcher.ChangeRoleAsync on deleted state providers started. Role: {2}", "id", "ReplicaId", "NewRole"),
                SM_STRUCTURED_TRACE(RecoverSPStart, 100, Info, "{1}: Dispatcher.RecoverCheckpointAsync started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RecoverSP, 101, Info, "{1}: Dispatcher.RecoverCheckpointAsync on state provider {2}.", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(RemoveSP, 102, Info, "{1}: Dispatcher.RemoveStateAsync started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RecoverSMCheckpointEmpty, 103, Info, "{1}: RecoverCheckpointAsync completed. Total: {2} ms", "id", "ReplicaId", "Timer"),
                SM_STRUCTURED_TRACE(RecoverSMCheckpoint, 104, Info, "{1}: RecoverCheckpointAsync completed in {2} ms, RemoveUnreferencedStateProviders completed in {3} ms", "id", "ReplicaId", "Timer1", "Timer2"),
                SM_STRUCTURED_TRACE(BeginSetState, 105, Info, "{1}: BeginSetCurrentState started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(CopyState, 106, Info, "{1}: CopyToLocalStateAsync: Copied State Provider Id: {2} to deleted list.", "id", "ReplicaId", "SPId"),
                SM_STRUCTURED_TRACE(GetChildren, 107, Info, "{1}: GetChildren started.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(AddSingle, 108, Info, "{1}: AddSingleStateProviderKeyExists: SPid: {2} TxnId: {3} TypeName: {4}", "id", "ReplicaId", "SPId", "TxnId", "TypeName"),
                SM_STRUCTURED_TRACE(AddSingleErr, 109, Error, "{1}: AddSingleStateProviderKeyExists failed cause state provider already exists.: SPid: {2} TxnId: {3} TypeName: {4}, ErrorCode: {5}", "id", "ReplicaId", "SPId", "TxnId", "TypeName", "errorCode"),
                SM_STRUCTURED_TRACE(RemoveSingleErr, 110, Error, "{1}: RemoveSingleAsync: SPName: {2} cannot be found. TxnId: {3}, ErrorCode: {4}", "id", "ReplicaId", "Name", "TxnId", "errorCode"),
                SM_STRUCTURED_TRACE(RemoveSingle, 111, Info, "{1}: RemoveSingleAsync: SPid: {2} SPName: {3} TxnId: {4}", "id", "ReplicaId", "SPId", "Name", "TxnId"),
                SM_STRUCTURED_TRACE(RemoveSingleNotExist, 112, Error, "{1}: RemoveSingleAsync: SPName: {2} cannot be found. TxnId: {3}, ErrorCode: {4}", "id", "ReplicaId", "Name", "TxnId", "errorCode"),
                SM_STRUCTURED_TRACE(RemoveSingleInvariant, 113, Error, "{1}: RemoveSingleAsync: SPId: {2} SPName: {3} is being deleted. TxnId: {4}, ErrorCode: {5}", "id", "ReplicaId", "SPId", "Name", "TxnId", "errorCode"),
                SM_STRUCTURED_TRACE(AddOperation, 114, Warning, "{1}: Retried AddOperation {2} times. TransactionId: {3} Backoff: {4}", "id", "ReplicaId", "Times", "TxnId", "Backoff"),
                SM_STRUCTURED_TRACE(NotRegistered, 115, Error, "{1}: StateProvider {2} is not registered, ErrorCode: {3}", "id", "ReplicaId", "SPid", "errorCode"),
                SM_STRUCTURED_TRACE(NotRegisteredDeleting, 116, Error, "{1}: StateProvider {2} is not registered, ErrorCode: {3}", "id", "ReplicaId", "Name", "errorCode"),
                SM_STRUCTURED_TRACE(PerformCheckpointSP, 117, Info, "{1}: SM.PerformCheckpoint: Completed perform checkpoint on {2} state providers", "id", "ReplicaId", "Count"),
                SM_STRUCTURED_TRACE(DeleteSPWorkDirectory, 118, Warning, "{1}: RemoveUnreferencedStateProviders failed. Directory: {2}", "id", "ReplicaId", "Dir"),
                SM_STRUCTURED_TRACE(Cleanup, 119, Info, "{1}: Cleanup metadata LSN: {2}", "id", "ReplicaId", "LSN"),
                SM_STRUCTURED_TRACE(CleanupDelete, 120, Info, "{1}: Cleanup metadata SPId: {2} LSN: {3}", "id", "ReplicaId", "SPId", "LSN"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSP, 121, Info, "{1}: RemoveUnreferencedStateProviders started. Work directory: {2}", "id", "ReplicaId", "Dir"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSPMark, 122, Info, "{1}: RemoveUnreferencedStateProviders. Marked SP directory for deletion: {2}", "id", "ReplicaId", "Dir"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSPDelete, 123, Info, "{1}: RemoveUnreferencedStateProviders. Deleting SP directory for deletion: {2}", "id", "ReplicaId", "Dir"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSPDeleteErr, 124, Warning, "{1}: RemoveUnreferencedStateProviders failed. Directory: {2}, ErrorCode: {3}", "id", "ReplicaId", "Dir", "errorCode"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSPTooLong, 125, Warning, "{1}: RemoveUnreferencedStateProviders took too long: {2} ms", "id", "ReplicaId", "Time"),
                SM_STRUCTURED_TRACE(RemoveUnreferencedSPCompleted, 126, Info, "{1}: RemoveUnreferencedStateProviders completed. Work directory: {2} Time: {3} ms", "id", "ReplicaId", "Dir", "Time"),
                SM_STRUCTURED_TRACE(GetCurrentState, 127, Info, "{1}: Api.GetCurrentState started. Target: {2}", "id", "ReplicaId", "targetReplicaId"),
                SM_STRUCTURED_TRACE(EndSettingCurrentState, 128, Info, "{1}: EndSettingCurrentState.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(Ctor, 129, Info, "{1}: Replica Folder: {2}, SerializationMode: {3}", "id", "ReplicaId", "ReplicaFolderPath", "SerializationMode"),
                SM_STRUCTURED_TRACE(Dtor, 130, Info, "{1}: Replica Folder: {2}, SerializationMode: {3}", "id", "ReplicaId", "ReplicaFolderPath", "SerializationMode"),
                SM_STRUCTURED_TRACE(BackupStart, 131, Info, "{1}: Api.BackupAsync starts.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(BackupEnd, 132, Info, "{1}: Api.BackupAsync ends.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RestoreStart, 133, Info, "{1}: Api.RestoreAsync starts.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RestoreEnd, 134, Info, "{1}: Api.RestoreAsync ends.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RestoreStateProviderStart, 135, Info, "{1}: Api.RestoreStateProviderAsync starts.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(RestoreStateProviderEnd, 136, Info, "{1}: Api.RestoreStateProviderAsync ends.", "id", "ReplicaId"),
                SM_STRUCTURED_TRACE(Error, 137, Error, "{1}: {2} ErrorCode: {3}", "id", "ReplicaId", "message", "errorCode"),
                SM_STRUCTURED_TRACE(ReplicationError, 138, Warning,
                    "{1}: SPId: {2} TxnId: {3} ErrorCode: {4}",
                    "id", "ReplicaId", "SPId", "TxnId", "errorCode"),
                SM_STRUCTURED_TRACE(ReadStatusNotGranted, 139, Info,
                    "{1}: SPName: {2} ReadStatus: {3} IsReadable: {4} Role: {5}",
                    "id", "ReplicaId", "SPName", "ReadStatus", "IsReadable", "Role"),
                SM_STRUCTURED_TRACE(Warning, 140, Warning, 
                    "{1}: {2}", "id", "ReplicaId", "message"),
                SM_STRUCTURED_TRACE(SetCurrentState, 141, Info, 
                    "{1}: Api.SetCurrentStateAsync started. StateRecordNumber: {2}, SPId: {3}", "id", "ReplicaId", "StateNum", "SPId"),

                // SM_Dispatch
                SM_STRUCTURED_TRACE(
                    Dispatch_CloseAsync, 146, Info,
                    "{1}: Active: {2} Deleted: {3} Time (ms): {4} Status: {5}",
                    "id", "ReplicaId", "aCnt", "dCnt", "time", "status"),
                SM_STRUCTURED_TRACE(
                    Dispatch_Abort, 147, Info,
                    "{1}: Active: {2} Deleted: {3} Time (ms): {4}",
                    "id", "ReplicaId", "aCnt", "dCnt", "time"),

                //  8. ApiDispatcher                [151, 160]
                SM_STRUCTURED_TRACE(
                    ISP2_ApiError, 151, Warning,
                    "{1}: SPId: {2} Api: Function: {3} Status: {4:x}",
                    "id", "ReplicaId", "SPId", "Function", "Status"),
                SM_STRUCTURED_TRACE(
                    ISP2_Factory_ApiError, 152, Warning,
                    "{1}: SPId: {2} Api: ISP2::Factory Name: {3} TypeName: {4} Status: {5:x}",
                    "id", "ReplicaId", "SPId", "Name", "TypeName", "Status"),
                SM_STRUCTURED_TRACE(
                    ISP2_Initialize_ApiError, 153, Warning,
                    "{1}: SPId: {2} Api: ISP2::Initialize WorkFolder: {3} Number of Children: {4} Status: {5:x}",
                    "id", "ReplicaId", "SPId", "WorkFolder", "numberOfChildren", "Status"),
                SM_STRUCTURED_TRACE(
                    ISP2_ChangeRoleAsync_ApiError, 154, Warning,
                    "{1}: SPId: {2} Api: ISP2::ChangeRoleAsync: {3} Status: {4:x}",
                    "id", "ReplicaId", "SPId", "role", "Status"),
                SM_STRUCTURED_TRACE(
                    ISP2_SetCurrentStateAsync_ApiError, 155, Warning,
                    "{1}: SPId: {2} Api: ISP2::SetCurrentStateAsync StateRecordNumber: {3} SegmentCount: {4} Status: {5:x}",
                    "id", "ReplicaId", "SPId", "srn", "scount", "Status"),

                SM_STRUCTURED_TRACE(
                    ISP2_PrepareCheckpoint_ApiError, 156, Warning,
                    "{1}: SPId: {2} Api: ISP2::PrepareCheckpoint CheckpointLSN: {3} Status: {4:x}",
                    "id", "ReplicaId", "SPId", "checkpointLSN", "Status"),

                SM_STRUCTURED_TRACE(
                    ISP2_PrepareForRemoveAsync_ApiError, 157, Warning,
                    "{1}: SPId: {2} Api: ISP2::PrepareForRemoveAsync TxnId: {3} Status: {4:x}",
                    "id", "ReplicaId", "SPId", "txnId", "Status"),

                //  9. ISPM                         [161, 180]
                SM_STRUCTURED_TRACE(
                    ISPM_ApiError, 160, Info,
                    "{1}: SPId: {2} API: {3} SPName: {4} TxnId: {5} ErrorCode: {6:x}",
                    "id", "ReplicaId", "SPId", "ApiName", "SPName", "TxnId", "errorCode")
            {
            }

            static Common::Global<StateManagerEventSource> Events;
        };
    }
}
