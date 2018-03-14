set( LINUX_SOURCES
  ../BackupLogRecord.cpp
  ../BarrierLogRecord.cpp
  ../BeginCheckpointLogRecord.cpp
  ../BeginTransactionOperationLogRecord.cpp
  ../CheckpointState.cpp
  ../CompleteCheckpointLogRecord.cpp
  ../Constants.cpp
  ../CopyContext.cpp
  ../CopyStage.cpp
  ../CopyStageBuffers.cpp
  ../CopyModeResult.cpp
  ../CopyHeader.cpp
  ../CopyMetadata.cpp
  ../EndCheckpointLogRecord.cpp
  ../EndTransactionLogRecord.cpp
  ../FullCopyReason.cpp
  ../IndexingLogRecord.cpp
  ../InformationEvent.cpp
  ../InformationLogRecord.cpp
  ../InvalidLogRecords.cpp
  ../LogHeadRecord.cpp
  ../LogRecord.cpp
  ../LogRecords.cpp
  ../LogRecordType.cpp
  ../LogicalLogRecord.cpp
  ../OperationLogRecord.cpp
  ../PhysicalLogRecord.cpp
  ../PhysicalLogReader.cpp
  ../RecoveryPhysicalLogReader.cpp
  ../ProgressVector.cpp
  ../ProgressVectorEntry.cpp
  ../SharedProgressVectorEntry.cpp
  ../TransactionLogRecord.cpp
  ../TruncateHeadLogRecord.cpp
  ../TruncateTailLogRecord.cpp
  ../TruncationState.cpp
  ../UpdateEpochLogRecord.cpp
)

include_directories("..")

add_library(objects_LogRecordLib OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_LogRecordLib ../stdafx.h)

add_library(${lib_LogRecordLib} STATIC $<TARGET_OBJECTS:objects_LogRecordLib>)
target_link_libraries(${lib_LogRecordLib}
    ${lib_txnreplicator_common}
    ${lib_data_utilities}
    ${Ktlfull}
    )

set_target_properties(${lib_LogRecordLib} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

