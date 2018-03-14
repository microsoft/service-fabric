set( LINUX_SOURCES
    ../Constants.cpp		
    ../GetServiceMetadataReply.cpp	
    ../TokenValidationServiceAgent.cpp
    ../TokenValidationServiceAgentFactory.cpp
    ../TokenValidationServiceConfig.cpp
    ../stdafx.cpp			
)

include_directories("..")

add_library(objects_TokenValidationService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_TokenValidationService ../stdafx.h)

add_library(${lib_TokenValidationService} STATIC $<TARGET_OBJECTS:objects_TokenValidationService>)

set_target_properties(${lib_TokenValidationService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

