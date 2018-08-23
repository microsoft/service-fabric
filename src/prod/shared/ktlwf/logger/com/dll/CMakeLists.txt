add_definitions(-DUPASSTHROUGH=1)

set( LINUX_SOURCES
    ../KPhysicalLog.cpp
    ../KPhysicalLog.Manager.cpp
    ../KPhysicalLog.Container.cpp
    ../KPhysicalLog.Stream.cpp
    dllmain.cpp
)

include_directories(
    ".."
    "../../../com/inc"
    "../../../../../src/ktllogger/sys/inc"
    "../../../../../src/ktllogger/sys/ktlshim"
)

add_library(objects_KPhysicalLog OBJECT ${LINUX_SOURCES})

add_library(${lib_KPhysicalLog} SHARED $<TARGET_OBJECTS:objects_KPhysicalLog>)

set_target_properties(${lib_KPhysicalLog} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_KPhysicalLog}.exports)
generate_exports_file(${lib_KPhysicalLog} "${CMAKE_CURRENT_SOURCE_DIR}/KPhysicalLogDll.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_KPhysicalLog} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_KPhysicalLog}
    ${lib_KComAdapter}
    ${lib_KtlLoggerShimUnify}
    ${KtlLoggerUser}
    ${Ktlfull}
    ${lib_FabricCommon}
    ${lib_FabricResources}
  	${lib_Transport}
  	${lib_Common}
  	ssh2
  	ssl
  	crypto
  	m
  	rt
  	pthread
  	dl
  	xml2
  	uuid
  	minizip
  	z
)

install(TARGETS ${lib_KPhysicalLog} DESTINATION ${TEST_OUTPUT_DIR})
