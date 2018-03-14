set( LINUX_SOURCES
    ../servicecommunicationclientfactory.cpp
    ../ServiceCommunicationListener.cpp
    ../ServiceCommunicationListenerFactory.cpp
    ../servicecommunicationtransportsettings.cpp
    ../ServiceMethodCallDispatcher.cpp
    ../Constants.cpp
    ../ServiceCommunicationClient.cpp
    ../ServiceCommunicationClient.SendRequestAsyncOperation.cpp
    ../ServiceCommunicationTransport.cpp
    ../ServiceCommunicationMessageHandlerCollection.cpp
    ../ServiceCommunicationHelper.cpp
	../ServiceCommunicationClient.DisconnectAsyncOperation.cpp
	../ServiceCommunicationClient.TryConnectAsyncOperation.cpp
	
)

include_directories("..")

add_library(objects_TcpServiceCommunication OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_TcpServiceCommunication ../stdafx.h)

add_library(${lib_TcpServiceCommunication} STATIC $<TARGET_OBJECTS:objects_TcpServiceCommunication>)

set_target_properties(${lib_TcpServiceCommunication} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
