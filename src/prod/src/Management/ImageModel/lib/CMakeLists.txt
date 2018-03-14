set( LINUX_SOURCES
    ../BuildLayoutSpecification.cpp
    ../ComBuildLayoutSpecification.cpp
    ../ComFabricDeploymentSpecification.cpp
    ../ComRunLayoutSpecification.cpp
    ../ComStoreLayoutSpecification.cpp
    ../ComWinFabLayoutSpecification.cpp
    ../ComWinFabRunLayoutSpecification.cpp
    ../FabricDeploymentSpecification.cpp
    ../ImageModelUtility.cpp
    ../NamePatterns.cpp
    ../RunLayoutSpecification.cpp
    ../StoreLayoutSpecification.cpp
    ../WinFabLayoutSpecification.cpp
    ../WinFabRunLayoutSpecification.cpp
)

include_directories("..")

add_library(objects_ImageModel OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ImageModel ../stdafx.h)

add_library(${lib_ImageModel} STATIC $<TARGET_OBJECTS:objects_ImageModel>)

set_target_properties(${lib_ImageModel} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

