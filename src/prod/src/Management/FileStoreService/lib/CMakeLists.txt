set( LINUX_SOURCES
    ../CopyDescription.cpp
    ../Constants.cpp
    ../FileAsyncOperation.cpp
    ../FileCopyAsyncOperation.cpp
    ../FileDeleteAsyncOperation.cpp
    ../FileMetadata.cpp
    ../FileStoreClient.cpp
    ../FileStoreServiceConfig.cpp
    ../FileStoreServiceFactory.cpp
    ../FileStoreServiceMessage.cpp
    ../FileStoreServiceReplica.cpp
    ../FileUploadAsyncOperation.cpp
    ../ImpersonatedSMBCopyContext.cpp
    ../InternalFileStoreClient.cpp
    ../PartitionContext.cpp
    ../PendingWriteOperations.cpp
    ../ProcessRequestAsyncOperation.cpp
    ../ProcessCopyRequestAsyncOperation.cpp
    ../ProcessDeleteRequestAsyncOperation.cpp
    ../ProcessInternalListRequestAsyncOperation.cpp
    ../ProcessListRequestAsyncOperation.cpp
    ../ProcessUploadRequestAsyncOperation.cpp
    ../ReplicatedStoreWrapper.cpp
    ../ReplicationManager.cpp
    ../RequestManager.cpp
    ../StagingLocationInfo.cpp
    ../StoreTransactionAsyncOperation.cpp
    ../Utility.cpp
    ../VersionNumber.cpp
    ../UploadSessionMetadata.cpp
    ../uploadsessionmap.cpp
    ../processcommituploadsessionrequestasyncoperation.cpp
    ../processcreateuploadsessionrequestasyncoperation.cpp
    ../processdeleteuploadsessionrequestasyncoperation.cpp
    ../processlistuploadsessionrequestasyncoperation.cpp
    ../processuploadchunkrequestasyncoperation.cpp
    ../processuploadchunkcontentrequestasyncoperation.cpp
    ../processcheckexistencerequestasyncoperation.cpp
    ../FileStoreService.PerformanceCounters.cpp
)

include_directories("..")

add_library(objects_FileStoreService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_FileStoreService ../stdafx.h)

add_library(${lib_FileStoreService} STATIC $<TARGET_OBJECTS:objects_FileStoreService>)

set_target_properties(${lib_FileStoreService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

