set( LINUX_SOURCES
  ../ActiveStateProviderEnumerator.cpp
  ../ApiDispatcher.cpp
  ../ApplyType.cpp
  ../CheckpointFile.cpp
  ../CheckpointFileAsyncEnumerator.cpp
  ../CheckpointFileBlocks.cpp
  ../CheckpointFileProperties.cpp
  ../CheckpointManager.cpp
  ../Comparer.cpp
  ../CopyNamedOperationData.cpp
  ../CopyOperationDataStream.cpp
  ../FactoryArguments.cpp
  ../Helper.cpp
  ../Metadata.cpp
  ../MetadataManager.cpp
  ../MetadataMode.cpp
  ../MetadataOperationData.cpp
  ../NamedOperationContext.cpp
  ../NamedOperationData.cpp
  ../NamedOperationDataStream.cpp
  ../OperationDataEnumerator.cpp
  ../OpenParameters.cpp
  ../RedoOperationData.cpp
  ../SerializableMetadata.cpp
  ../SerializableMetadataEnumerator.cpp
  ../SerializationMode.cpp
  ../StateManager.cpp
  ../StateManagerEventSource.cpp
  ../StateManagerFactory.cpp
  ../StateManagerLockContext.cpp
  ../StateManagerTransactionContext.cpp
  ../StateProviderInfoInternal.cpp
)

include_directories("..")

add_library(objects_statemanager OBJECT ${LINUX_SOURCES})

#add_precompiled_header(objects_statemanager ../stdafx.h)

add_library(${lib_statemanager} STATIC $<TARGET_OBJECTS:objects_statemanager>)
target_link_libraries(${lib_statemanager}
    ${lib_txnreplicator_common}
    ${Ktlfull}
    )

set_target_properties(${lib_statemanager} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

