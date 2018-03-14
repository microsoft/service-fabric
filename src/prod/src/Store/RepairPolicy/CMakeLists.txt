set( LINUX_SOURCES
EseReplicatedStoreRepairPolicy.cpp
RepairAction.cpp
RepairCondition.cpp
)

include_directories("..")
include_directories("../pch")

add_library(objects_store_repairpolicy OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_store_repairpolicy ../stdafx.h)
add_library(${lib_StoreRepairPolicy} STATIC $<TARGET_OBJECTS:objects_store_repairpolicy>)


set_target_properties(${lib_StoreRepairPolicy} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
