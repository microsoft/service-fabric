set( LINUX_SOURCES
  ../BackupFolderInfo.cpp
  ../BackupLogFile.cpp
  ../BackupLogFileAsyncEnumerator.cpp
  ../BackupLogFileProperties.cpp
  ../BackupManager.cpp
  ../BackupMetadataFile.cpp
  ../BackupMetadataFileProperties.cpp
  ../CheckpointManager.cpp
  ../CopyStream.cpp
  ../DrainingStream.cpp
  ../EventSource.cpp
  ../FaultyFileLog.cpp
  ../FileLogManager.cpp
  ../FlushedRecordInfo.cpp
  ../IncrementalBackupLogRecordsAsyncEnumerator.cpp
  ../KLogManager.cpp
  ../IStateProvider.cpp
  ../LoggedRecords.cpp
  ../LoggingReplicatorFactory.cpp
  ../LoggingReplicatorImpl.cpp
  ../LogManager.cpp
  ../LogReaderRange.cpp
  ../LogRecordsDispatcher.cpp
  ../LogRecordsMap.cpp
  ../LogTruncationManager.cpp
  ../MemoryLog.cpp
  ../MemoryLogManager.cpp
  ../MemoryLogReadStream.cpp
  ../NotificationKey.cpp
  ../NotificationKeyComparer.cpp
  ../OperationProcessor.cpp
  ../PeriodicCheckpointTruncationState.cpp
  ../ParallelLogRecordsDispatcher.cpp
  ../PhysicalLogWriter.cpp
  ../PhysicalLogWriterCallbackManager.cpp
  ../RecoveredOrCopiedCheckpointState.cpp
  ../RecoveryManager.cpp
  ../ReplicatedLogManager.cpp
  ../ReplicatorBackup.cpp
  ../RoleContextDrainState.cpp
  ../SecondaryDrainManager.cpp
  ../SerialLogRecordsDispatcher.cpp
  ../StateProvider.cpp
  ../TransactionManager.cpp
  ../TransactionMap.cpp
  ../TruncateTailManager.cpp
  ../VersionManager.cpp
  ../VersionManagerFactory.cpp
)

include_directories("..")
include_directories("../../../../ktllogger/sys/inc")
include_directories("../../../../ktllogger/sys/ktlshim")

add_library(objects_LoggingReplicator OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_LoggingReplicator ../stdafx.h)

add_library(${lib_LoggingReplicator} STATIC $<TARGET_OBJECTS:objects_LoggingReplicator>)
target_link_libraries(${lib_LoggingReplicator}
    ${lib_LogRecordLib}
    ${lib_txnreplicator_common}
    ${lib_data_utilities}
    ${lib_LogicalLog}
    ${lib_TestHooks}
    ${KtlLoggerUser}
    ${Ktlfull}
    )

set_target_properties(${lib_LoggingReplicator} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

