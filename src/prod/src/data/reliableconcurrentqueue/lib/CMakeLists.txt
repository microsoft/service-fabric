set (lib_ReliableConcurrentQueue "ReliableConcurrentQueue" CACHE STRING "ReliableConcurrentQueue library")

set( LINUX_SOURCES
  ../stdafx.cpp
  ../RCQStateProviderFactory.cpp
)

include_directories("..")

add_library(objects_ReliableConcurrentQueue OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliableConcurrentQueue ../stdafx.h FORCEINCLUDE)

add_library(${lib_ReliableConcurrentQueue} STATIC $<TARGET_OBJECTS:objects_ReliableConcurrentQueue>)
target_link_libraries(${lib_ReliableConcurrentQueue}
    ${lib_TStore}
    ${lib_TransactionalReplicator}
    ${Ktlfull}
)

set_target_properties(${lib_ReliableConcurrentQueue} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 


