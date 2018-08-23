set ( LINUX_SOURCES
    ../../../../pal/wc16.c
    ./lttngreader.cpp
    ./lttngreader_util.cpp
)

add_library(objects_lttngreader OBJECT ${LINUX_SOURCES})
add_library(${lib_LttngReader} SHARED $<TARGET_OBJECTS:objects_lttngreader>)

set_target_properties(${lib_LttngReader} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_LttngReader}.exports)
generate_exports_file(${lib_LttngReader} "${CMAKE_CURRENT_SOURCE_DIR}/LttngReader.def" ${EXPORTS_FILE})

set_target_properties(${lib_LttngReader} PROPERTIES
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_LttngReader}
    ${Babeltrace}
    ${BabeltraceCTF}
    lttng-ust
    lttng-ctl
    ${Cxx}
    ${CxxABI}
)
