set( LINUX_SOURCES
    NetworkInventoryManagerConfig.cpp
    NIMNetworkInventoryService.cpp
    NIMNetworkAllocationPool.cpp
    NIMNetworkInventoryAllocationManager.cpp
    NIMNetworkManager.cpp
    NIMNetworkNodeAllocationStoreData.cpp
    NIMNetworkDefinitionStoreData.cpp
    NIMNetworkAddressPoolStoreData.cpp
    NIMMessage.cpp
    NIMNetworkAllocationRequestMessage.cpp
    NIMNetworkErrorCodeResponseMessage.cpp
    NIMPublishNetworkTablesRequestMessageBody.cpp
)

add_library(objects_NetworkInventoryManager OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_NetworkInventoryManager stdafx.h)

add_library(${lib_NetworkInventoryManager} STATIC $<TARGET_OBJECTS:objects_NetworkInventoryManager>)

set_target_properties(${lib_NetworkInventoryManager} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

