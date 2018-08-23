set( LINUX_SOURCES
    ../stdafx.cpp
    ../BackupCallbackHandler.cpp
    ../ConfigurationPackageChangeHandler.cpp
    ../DictionaryChangeHandler.cpp
    ../StateManagerChangeHandler.cpp
    ../CompatRDStateProvider.cpp
    ../ComStateProviderFactory.cpp
    ../CancellationTokenSourceCExports.cpp
    ../Helper.cpp
    ../RCREventSource.cpp
    ../StateProviderCExports.cpp
    ../StateProviderFactory.cpp
    ../StateProviderInfo.cpp
    ../StoreKeyEnumeratorCExports.cpp
    ../StoreKeyValueEnumeratorCExports.cpp
    ../TestSettingsCExports.cpp
    ../TransactionCExports.cpp
    ../TStoreCExports.cpp
    ../ReliableConcurrentQueueCExports.cpp
    ../TransactionalReplicatorCExports.cpp
    ../StateProviderEnumeratorCExports.cpp
    ../ReliableCollectionRuntimeImpl.cpp
    ../ReliableCollectionRuntimeMock.cpp
    ../TransactionChangeHandler.cpp
)

include_directories("..")

add_library(objects_ReliableCollectionRuntimeImpl OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliableCollectionRuntimeImpl ../stdafx.h)

add_library(${lib_ReliableCollectionRuntimeImpl} STATIC $<TARGET_OBJECTS:objects_ReliableCollectionRuntimeImpl>)

target_link_libraries(${lib_ReliableCollectionRuntimeImpl}
    ${lib_ReliableConcurrentQueue}
    ${lib_TStore}
    ${lib_TransactionalReplicator}
    ${Ktlfull}
)

set_target_properties(${lib_ReliableCollectionRuntimeImpl} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

