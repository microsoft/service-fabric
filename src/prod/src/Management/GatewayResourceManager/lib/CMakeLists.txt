set( LINUX_SOURCES
    ../Constants.cpp		
    ../GatewayResourceManagerMessage.cpp	
    ../GatewayResourceManagerAgent.cpp
    ../GatewayResourceManagerAgentFactory.cpp
    ../GatewayResourceManagerConfig.cpp
    ../ProcessQueryAsyncOperation.cpp
    ../stdafx.cpp			
)

include_directories("..")

add_library(objects_GatewayResourceManager OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_GatewayResourceManager ../stdafx.h)

add_library(${lib_GatewayResourceManager} STATIC $<TARGET_OBJECTS:objects_GatewayResourceManager>)

set_target_properties(${lib_GatewayResourceManager} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

