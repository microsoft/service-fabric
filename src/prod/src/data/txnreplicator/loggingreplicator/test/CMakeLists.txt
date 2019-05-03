include_directories("..")
include_directories("../../../../ktllogger/sys/inc")
include_directories("../../../../ktllogger/sys/ktlshim")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_loggingreplicator_test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../ApiFaultUtility.cpp
  ../BackupFolderInfo.Test.cpp
  ../BackupLogFile.Test.cpp
  ../BackupLogFileProperties.Test.cpp
  ../BackupManager.Test.cpp
  ../BackupMetadataFile.Test.cpp
  ../BackupMetadataFileProperties.Test.cpp
  ../CompletionTask.Test.cpp
  ../CopyContext.Test.cpp
  ../CopyHeader.Test.cpp
  ../CopyMetadata.Test.cpp
  ../Integration.Test.cpp
  ../KLogManager.Test.cpp
  ../LogicalLogRecord.Test.cpp
  ../LogManager.Test.cpp
  ../LogRecordsDispatcher.Test.cpp
  ../LogTruncationManager.Test.cpp
  ../MemoryLog.Test.cpp
  ../OperationProcessor.Test.cpp
  ../PeriodicCheckpointTruncation.Test.cpp
  ../PhysicalLogRecord.Test.cpp
  ../PhysicalLogWriter.Test.cpp
  ../ProgressVector.Test.cpp
  ../TestBackupCallbackHandler.cpp
  ../TestBackupRestoreProvider.cpp
  ../TestCheckpointManager.cpp
  ../TestCopyStreamConverter.cpp
  ../TestLoggingReplicatorToVersionManager.cpp
  ../TestLogManager.cpp
  ../TestLogRecords.cpp
  ../TestLogRecordUtility.cpp
  ../TestLogTruncationManager.cpp
  ../TestOperation.cpp
  ../TestOperationProcessor.cpp
  ../TestPhysicalLogReader.cpp
  ../TestReplica.cpp
  ../TestReplicatedLogManager.cpp
  ../TestStateProviderManager.cpp
  ../TestStateReplicator.cpp
  ../TestStateStream.cpp
  ../TestTransaction.cpp
  ../TestTransactionGenerator.cpp
  ../TestTransactionManager.cpp
  ../TestVersionProvider.cpp
  ../TransactionMap.Test.cpp
  ../VersionManagerTestBase.cpp
  ../VersionManager.Test.cpp
  ../TestTransactionChangeHandler.cpp
  ../TestTransactionReplicator.cpp
)

add_precompiled_header(${exe_loggingreplicator_test} ../stdafx.h)

set_target_properties(${exe_loggingreplicator_test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
        LINK_FLAGS "-Wl,--allow-multiple-definition") 

target_link_libraries(${exe_loggingreplicator_test}
  ${lib_LoggingReplicator}
  ${lib_data_testcommon}
  ${lib_txnreplicator_common}
  ${lib_data_utilities}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_KComAdapter}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
  ssl
  crypto
  minizip
  z
  m
  rt
  pthread
  c
  dl
  xml2
  uuid
)

install(
    FILES ./loggingreplicator.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_loggingreplicator_test}.cfg
)
