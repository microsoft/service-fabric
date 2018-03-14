set( LINUX_SOURCES
  ../ApiMonitoringWrapper.cpp
  ../AtomicOperation.cpp
  ../AtomicRedoOperation.cpp
  ../BackupInfo.cpp
  ../BackupVersion.cpp
  ../CompletionTask.cpp
  ../EnumerationCompletionResult.cpp
  ../Epoch.cpp
  ../LockContext.cpp
  ../IOperationDataStream.cpp
  ../OperationContext.cpp
  ../OperationDataStream.cpp
  ../RecoveryInformation.cpp
  ../StateProviderInfo.cpp
  ../Transaction.cpp
  ../TransactionBase.cpp
  ../TransactionState.cpp
  ../TransactionStateMachine.cpp
  ../TREventSource.cpp
  ../IOMonitor.cpp
  ../TRInternalSettings.cpp
  ../TRPerformanceCounters.cpp
  ../SLInternalSettings.cpp
  ../TryRemoveVersionResult.cpp
)

include_directories("..")

add_library(objects_txnreplicator_common OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_txnreplicator_common ../stdafx.h)

add_library(${lib_txnreplicator_common} STATIC $<TARGET_OBJECTS:objects_txnreplicator_common>)
target_link_libraries(${lib_txnreplicator_common}
    ${lib_data_utilities}
    ${Ktlfull}
    )

set_target_properties(${lib_txnreplicator_common} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

