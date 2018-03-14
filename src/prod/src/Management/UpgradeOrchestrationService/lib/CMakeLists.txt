set( LINUX_SOURCES
    ../Constants.cpp
    ../UpgradeOrchestrationServiceAgent.cpp
    ../UpgradeOrchestrationServiceAgentFactory.cpp
    ../UpgradeOrchestrationServiceConfig.cpp
)

include_directories("..")

add_library(objects_UpgradeOrchestrationService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_UpgradeOrchestrationService ../stdafx.h)

add_library(${lib_UpgradeOrchestrationService} STATIC $<TARGET_OBJECTS:objects_UpgradeOrchestrationService>)

set_target_properties(${lib_UpgradeOrchestrationService} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
