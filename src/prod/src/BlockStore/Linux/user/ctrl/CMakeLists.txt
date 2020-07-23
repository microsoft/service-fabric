include_directories("../../driver/inc/")
include_directories("../inc/")
add_compile_options(-rdynamic)

add_executable ( ${exe_BlockStoreCtl}
  ./ctrl.cpp
)

target_link_libraries( ${exe_BlockStoreCtl}
    ${lib_BlockStore}
    z
    m
    rt
    pthread
    c
    dl
    xml2
)

set_target_properties( ${exe_BlockStoreCtl} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
