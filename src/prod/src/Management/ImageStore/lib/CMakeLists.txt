set( LINUX_SOURCES
    ../AzureImageStore.cpp
    ../FileImageStore.cpp
    ../IImageStore.cpp
    ../ImageCacheConstants.cpp
    ../ImageStoreAccessDescription.cpp
    ../ImageStoreFactory.cpp
    ../ImageStoreServiceConfig.cpp
    ../ImageStoreUtility.cpp
    ../NativeImageStore.cpp
    ../NativeImageStoreProgressState.cpp
)

include_directories("..")

add_library(objects_ImageStore OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ImageStore ../stdafx.h)

add_library(${lib_ImageStore} STATIC $<TARGET_OBJECTS:objects_ImageStore>)

set_target_properties(${lib_ImageStore} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

