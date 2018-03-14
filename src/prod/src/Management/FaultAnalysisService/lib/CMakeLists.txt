set( LINUX_SOURCES
    ../Constants.cpp
    ../FaultAnalysisServiceAgent.cpp
    ../FaultAnalysisServiceAgentFactory.cpp
    ../FaultAnalysisServiceConfig.cpp
    ../FaultAnalysisServiceMessage.cpp
    ../ProcessQueryAsyncOperation.cpp
    ../ClientRequestAsyncOperation.cpp
    ../FaultAnalysisServiceAgentQueryMessageHandler.cpp
)

include_directories("..")

add_library(objects_FaultAnalysisService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_FaultAnalysisService ../stdafx.h)

add_library(${lib_FaultAnalysisService} STATIC $<TARGET_OBJECTS:objects_FaultAnalysisService>)

set_target_properties(${lib_FaultAnalysisService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

