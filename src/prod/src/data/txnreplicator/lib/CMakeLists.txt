set( LINUX_SOURCES
    ../ComOperationDataStream.cpp
    ../ComProxyDataLossHandler.cpp
    ../ComProxyOperation.cpp
    ../ComProxyOperationDataStream.cpp
    ../ComProxyOperationStream.cpp
    ../ComProxyStateProvider2Factory.cpp
    ../ComProxyStateReplicator.cpp
    ../ComStateProvider.cpp
    ../ComTransactionalReplicator.cpp
    ../ComTransactionalReplicatorFactory.cpp
    ../ITransactionalReplicatorFactory.cpp
    ../RuntimeFolders.cpp
    ../TransactionalReplicator.cpp
    ../TransactionalReplicatorSettings.Test.cpp
)

include_directories("..")
include_directories("../../../ktllogger/sys/ktlshim")
include_directories("../../../ktllogger/sys/inc")

add_library(objects_TransactionalReplicator OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_TransactionalReplicator ../stdafx.h)

add_library(${lib_TransactionalReplicator} STATIC $<TARGET_OBJECTS:objects_TransactionalReplicator>)
target_link_libraries(${lib_TransactionalReplicator}
    ${lib_statemanager}
    ${lib_LoggingReplicator}
    ${lib_TestHooks}
    ${lib_data_utilities}
    ${lib_KComAdapter}
    ${lib_KtlLoggerShimUnify}
    ${KtlLoggerUser}
    ${Ktlfull}
    )

set_target_properties(${lib_TransactionalReplicator} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

