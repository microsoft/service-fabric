set( LINUX_SOURCES 
    ../Constants.cpp 
    ../HttpServer.cpp
    ../HttpServer.LinuxAsyncServiceBaseOperation.cpp
    ../SimpleHttpServer.cpp
    ../HttpServer.OpenLinuxAsyncServiceOperation.cpp
    ../HttpServer.CloseLinuxAsyncServiceOperation.cpp
    ../requestmessagecontext.getfilefromuploadasyncoperation.linux.cpp
    ../RequestMessageContext.SendResponseAsyncOperation.Linux.cpp
    ../requestmessagecontext.linux.cpp
    ../HttpUtil.cpp
    ../stdafx.cpp
    )

include_directories("..")

set(Casablanca_LIBRARY ${LIB}cpprest)
set(Casablanca_LIBRARIES ${Casablanca_LIBRARY} ${Boost_LIBRARIES} ${Boost_FRAMEWORK})

add_library(objects_HttpTransport OBJECT ${LINUX_SOURCES} )

add_precompiled_header(objects_HttpTransport ../stdafx.h)

add_library(${lib_httptransport} STATIC $<TARGET_OBJECTS:objects_HttpTransport>)

target_link_libraries( ${lib_httptransport}  
    ${Casablanca_LIBRARIES} ssl crypto 
    )

set_target_properties(${lib_httptransport} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR} )

