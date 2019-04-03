// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Define all the TAG's here
#define LOGMANAGER_TAG 'MgoL'
#define FILELOGMANAGER_TAG 'FgoL'
#define KTLLOGMANAGER_TAG  'KgoL'
#define FAULTYFILELOGMANAGER_TAG 'GlfF'
#define MEMORYLOGMANAGER_TAG 'LmeM'
#define LOGGEDRECORDS_TAG 'SgoL'
#define LOGGINGREPLICATOR_TAG 'peRL'
#define LOGRECORDS_DISPATCHER_TAG 'DgoL'
#define PHYSICALLOGWRITER_TAG 'WyhP'
#define PHYSICALLOGWRITERCALLBACKMGR_TAG 'CyhP'
#define OPERATIONPROCESSOR_TAG 'rPpO'
#define REPLICATEDLOGMANAGER_TAG 'LpeR'
#define TXMAP_TAG 'paMT'
#define CHECKPOINTMANAGER_TAG 'KHCR'
#define TRANSACTIONMANAGER_TAG 'gMxT'
#define LOGRECORDSMAP_TAG 'pMRL'
#define RECOVERYMANAGER_TAG 'oceR'
#define COPYSTREAM_TAG 'SpoC'
#define LOGTRUNCATIONMANAGER_TAG 'MtgL'
#define V1REPLICATOR_TAG 'LPER'
#define TRUNCATETAILMANAGER_TAG 'MtrT'
#define SECONDARYDRAINMANAGER_TAG 'MrDS'

#define VERSION_MANAGER_TAG 'rgMV'
#define NOTIFICATION_KEY_TAG 'yeKN'
#define NOTIFICATION_KEY_COMPARER_TAG 'mcKN'

// Tags for backup related objects.
#define BACKUP_METADATA_FILE_PROPERTIES_TAG 'pfMB' // BackupMetadataFileProperties 
#define BACKUP_METADATA_FILE_TAG 'ifMB' // BackupMetadataFIle 
#define BACKUP_LOG_FILE_PROPERTIES_TAG 'pfLB' // BackupLogFilePropeRties 
#define BACKUP_LOG_FILE_TAG 'ifLB' // BackupLogFIle 
#define BACKUP_LOG_FILE_ASYNC_ENUMERATOR_TAG 'eaLB' // BackupLogfileAsyncEnumerator
#define INCREMENTAL_BACKUP_LOG_RECORDS_ASYNC_ENUMERATOR 'eaBI' // IncrementalBackuplogrecordsAsyncEnumerator
#define BACKUP_MANAGER_TAG 'rgMB' // BackupManaGeR
#define BACKUP_FOLDER_INFO_TAG 'niFB' // BackupFolderINfo

#include "../logrecordlib/LogRecordLib.Public.h"

#include "VersionManager.Public.h"
#include "LoggingReplicator.Internal.h"

#include "TracingHeaders.h"

#define LR_TRACE_EXCEPTIONINFORECORD(text, rec, status) \
    EventSource::Events->ExceptionInfo( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        rec.RecordType, \
        rec.Lsn, \
        rec.Psn, \
        status); \

#define LR_TRACE_EXCEPTIONINFO(text, status) \
    EventSource::Events->ExceptionInfo( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        Data::LogRecordLib::LogRecordType::Enum::Invalid, \
        -1, \
        -1, \
        status); \

#define LR_TRACE_EXCEPTION_STATUS(text, status) \
    EventSource::Events->Exception( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        Data::LogRecordLib::LogRecordType::Enum::Invalid, \
        -1, \
        -1, \
        status); \

#define LR_TRACE_EXCEPTION(text, status) \
    if (status == SF_STATUS_OBJECT_CLOSED) \
    { \
        LR_TRACE_EXCEPTIONINFO(text, status) \
    } \
    else \
    { \
        EventSource::Events->Exception( \
            TracePartitionId, \
            ReplicaId, \
            text, \
            Data::LogRecordLib::LogRecordType::Enum::Invalid, \
            -1, \
            -1, \
            status); \
    } \

#define LR_TRACE_EXCEPTIONRECORD(text, rec, status) \
    if (status == SF_STATUS_OBJECT_CLOSED) \
    { \
        LR_TRACE_EXCEPTIONINFORECORD(text, rec, status) \
    } \
    else \
    { \
        EventSource::Events->Exception( \
            TracePartitionId, \
            ReplicaId, \
            text, \
            rec.RecordType, \
            rec.Lsn, \
            rec.Psn, \
            status); \
    } \

#define LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(text, status) \
    EventSource::Events->ExceptionError( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        Data::LogRecordLib::LogRecordType::Enum::Invalid, \
        -1, \
        -1, \
        status); \

#define LR_TRACE_UNEXPECTEDEXCEPTION(text, status) \
    EventSource::Events->ExceptionError( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        Data::LogRecordLib::LogRecordType::Enum::Invalid, \
        -1, \
        -1, \
        status); \

#define LR_TRACE_UNEXPECTEDEXCEPTIONRECORD(text, rec, status) \
    EventSource::Events->ExceptionError( \
        TracePartitionId, \
        ReplicaId, \
        text, \
        rec.RecordType, \
        rec.Lsn, \
        rec.Psn, \
        status); \

#define FIRST_NTSTATUS_ERROR(status1, status2) NT_SUCCESS(status1) ? status2 : status1

#define BM_TRACE_AND_CO_RETURN_ON_FAILURE(text, id, status) \
    if (!NT_SUCCESS(status)) \
    { \
        EventSource::Events->IBM_BackupAsyncFinishError( \
            TracePartitionId, \
            ReplicaId, \
            id, \
            L"status", \
            status, \
            text); \
        co_return status; \
    } \

#define BM_TRACE_AND_CO_RETURN(text, id, status) \
    EventSource::Events->IBM_BackupAsyncFinishError( \
        TracePartitionId, \
        ReplicaId, \
        id, \
        L"status", \
        status, \
        text); \
    co_return status; \

#include "NotificationKey.h"
#include "NotificationKeyComparer.h"
#include "VersionManager.h"

// LoggingReplicator
#include "Pointers.h"
#include "RecoveredOrCopiedCheckpointState.h"
#include "IOperation.h"
#include "IOperationStream.h"
#include "RoleContextDrainState.h"

// This is a Win32 based Filelog
#ifndef PLATFORM_UNIX
#include "FileLog.h"
#endif

#include "FaultyFileLog.h"
#include "MemoryLog.h"
#include "MemoryLogReadStream.h"
#include "TransactionMap.h"
#include "LogRecordsMap.h"
#include "LoggedRecords.h"
#include "ILogManager.h"
#include "IFlushCallbackProcessor.h"
#include "ICompletedRecordsProcessor.h"
#include "PhysicalLogWriterCallbackManager.h"
#include "PhysicalLogWriter.h"
#include "LogManager.h"
#include "FileLogManager.h"
#include "KLogManager.h"
#include "MemoryLogManager.h"
#include "IReplicatedLogManager.h"
#include "ReplicatedLogManager.h"
#include "RecordProcessingMode.h"
#include "ILogTruncationManager.h"
#include "LogTruncationManager.h"
#include "IBackupManager.h"
#include "ICheckpointManager.h"
#include "CheckpointManager.h"
#include "IOperationProcessor.h"
#include "OperationProcessor.h"
#include "LogRecordsDispatcher.h"
#include "ParallelLogRecordsDispatcher.h"
#include "SerialLogRecordsDispatcher.h"
#include "TransactionManager.h"
#include "RecoveryManager.h"
#include "CopyStream.h"
#include "TruncateTailManager.h"
#include "SecondaryDrainManager.h"
#include "ReplicatorBackup.h"
#include "BackupMetadataFileProperties.h"
#include "BackupMetadataFile.h"
#include "BackupFolderInfo.h"
#include "BackupManager.h"
#include "LoggingReplicatorImpl.h"
#include "StateProvider.h"

// Backup Headers.
#include "BackupLogFileProperties.h"
#include "BackupLogFileAsyncEnumerator.h"
#include "IncrementalBackupLogRecordsAsyncEnumerator.h"
#include "BackupLogFile.h"
