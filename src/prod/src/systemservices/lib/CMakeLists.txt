set( LINUX_SOURCES
    ../ServiceDirectMessagingAgent.cpp
    ../ServiceDirectMessagingAgentMessage.cpp
    ../ServiceDirectMessagingClient.cpp
    ../ServiceRoutingAgent.cpp
    ../ServiceRoutingAgentMessage.cpp
    ../ServiceRoutingAgentProxy.cpp
    ../SystemServiceFilterHeader.cpp
    ../SystemServiceLocation.cpp
    ../SystemServiceMessageFilter.cpp
    ../SystemServiceMessageHandlers.cpp
    ../SystemServiceResolver.cpp
    ../ExtractedEndpointList.cpp
)

include_directories("..")

add_library(objects_SystemServices OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_SystemServices ../stdafx.h)

add_library(${lib_SystemServices} STATIC $<TARGET_OBJECTS:objects_SystemServices>)

set_target_properties(${lib_SystemServices} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

