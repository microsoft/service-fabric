set( LINUX_SOURCES
    ../CacheModeHeader.cpp
    ../Constants.cpp
    ../NamingMessage.cpp
    ../NamingCommonEventSource.cpp
    ../NamingConfig.cpp
    ../NamingService.cpp
    ../NamingServiceCuidCollection.cpp
    ../NamingUtility.cpp
    ../HierarchyNameData.cpp
    ../HierarchyNameState.cpp
    ../MockServiceTableEntryCache.cpp
    ../namepropertykey.cpp
    ../NamingCounters.cpp
    ../NamingStore.cpp
    ../PendingNamesHierarchy.cpp
    ../RequestInstance.cpp
    ../requestinstancetracker.cpp
    ../ServiceNotificationManager.cpp
    ../ServiceNotificationSender.cpp
    ../PsdCache.cpp
    ../RecoverPrimaryAsyncOperation.cpp
    ../RemoveServiceAtAuthorityAsyncOperation.cpp
    ../RemoveServiceAtNameOwnerAsyncOperation.cpp
    ../StoreService.cpp
    ../storeservicehealthmonitor.cpp
    ../StoreServiceFactory.cpp
    ../StoreService.ProcessCreateNameRequestAsyncOperation.cpp
    ../StoreService.ProcessCreateServiceRequestAsyncOperation.cpp
    ../StoreService.ProcessDeleteNameRequestAsyncOperation.cpp
    ../StoreService.ProcessDeleteServiceRequestAsyncOperation.cpp
    ../StoreService.ProcessEnumeratePropertiesRequestAsyncOperation.cpp
    ../StoreService.ProcessEnumerateSubNamesRequestAsyncOperation.cpp
    ../StoreService.ProcessInnerCreateNameRequestAsyncOperation.cpp
    ../StoreService.ProcessInnerCreateServiceRequestAsyncOperation.cpp
    ../StoreService.ProcessInnerDeleteNameRequestAsyncOperation.cpp
    ../StoreService.ProcessInnerDeleteServiceRequestAsyncOperation.cpp
    ../StoreService.ProcessInnerUpdateServiceRequestAsyncOperation.cpp
    ../StoreService.ProcessNameExistsRequestAsyncOperation.cpp
    ../StoreService.ProcessPropertyBatchRequestAsyncOperation.cpp
    ../StoreService.ProcessRequestAsyncOperation.cpp
    ../StoreService.ProcessServiceExistsRequestAsyncOperation.cpp
    ../StoreService.ProcessPrefixResolveRequestAsyncOperation.cpp
    ../StoreService.ProcessUpdateServiceRequestAsyncOperation.cpp    
    ../WriteServiceAtAuthorityAsyncOperation.cpp
    ../UpdateServiceAtAuthorityAsyncOperation.cpp
    ../WriteServiceAtNameOwnerAsyncOperation.cpp
)

include_directories("..")

add_library(objects_StoreService OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_StoreService ../stdafx.h)
add_library(${lib_StoreService} STATIC $<TARGET_OBJECTS:objects_StoreService>)

set_target_properties(${lib_StoreService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 


