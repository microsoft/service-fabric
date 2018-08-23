set( LINUX_SOURCES
    ../QueryAddress.cpp
    ../QueryAddresses.cpp
    ../QueryAddressGenerator.cpp
    ../QueryConfig.cpp
    ../QueryEventSource.cpp
    ../QueryGateway.cpp
    ../QueryMessageHandler.cpp
    ../QueryMessageUtility.cpp
    ../QueryMessage.cpp
    ../QuerySpecification.cpp
    ../QuerySpecificationStore.cpp
    ../GetApplicationListQuerySpecification.cpp
    ../GetApplicationPagedListDeployedOnNodeQuerySpecification.cpp
    ../GetNodeListQuerySpecification.cpp
    ../GetApplicationServiceListQuerySpecification.cpp
    ../GetReplicaResourceListQuerySpecification.cpp
    ../GetServicePartitionReplicaListQuerySpecification.cpp
    ../GetServicePartitionListQuerySpecification.cpp
    ../GetSystemServiceListQuerySpecification.cpp
    ../SwapReplicaQuerySpecificationHelper.cpp
    ../GetUnplacedReplicaInformationQuerySpecificationHelper.cpp
    ../GetServiceNameQuerySpecificationHelper.cpp
    ../ParallelQuerySpecification.cpp
    ../GetDeployedCodePackageParallelQuerySpecification.cpp
    ../GetApplicationResourceListQuerySpecification.cpp
    ../GetServiceResourceListQuerySpecification.cpp
    ../GetContainerCodePackageLogsQuerySpecification.cpp
)

include_directories("..")

add_library(objects_Query OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_Query ../stdafx.h)

add_library(${lib_Query} STATIC $<TARGET_OBJECTS:objects_Query>)

set_target_properties(${lib_Query} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

