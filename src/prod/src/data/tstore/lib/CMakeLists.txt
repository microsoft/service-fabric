set( LINUX_SOURCES
    ../ByteAlignedReaderWriterHelper.cpp
    ../CheckpointFile.cpp
    ../ConsolidationTask.cpp
    ../Constants.cpp
    ../CopyManager.cpp
    ../Diagnostics.cpp
    ../FileCountMergeConfiguration.cpp
    ../FileMetadata.cpp
    ../FilePropertySection.cpp
    ../Index.cpp
    ../KBufferComparer.cpp
    ../KeyCheckpointFile.cpp
    ../KeyCheckpointFileProperties.cpp
    ../KeyChunkMetadata.cpp
    ../KeySizeEstimator.cpp
    ../MemoryBuffer.cpp
    ../MetadataManager.cpp
    ../MetadataManagerFileProperties.cpp
    ../MetadataOperationData.cpp
    ../MetadataTable.cpp
    ../MergeHelper.cpp
    ../NullableStringStateSerializer.cpp
    ../PostMergeMetadataTableInformation.cpp
    ../PropertyChunkMetadata.cpp
    ../SharedBinaryReader.cpp
    ../RedoUndoOperationData.cpp
    ../SharedBinaryWriter.cpp
    ../StoreCopyStream.cpp
    ../StoreTraceComponent.cpp
    ../StreamPool.cpp
    ../StringStateSerializer.cpp
    ../ValueCheckpointFile.cpp
    ../ValueCheckpointFileProperties.cpp
    ../KBufferSerializer.cpp
    ../StoreEventSource.cpp
    ../StoreInitializationParameters.cpp
    ../StoreStateProviderFactory.cpp
    ../FileMetaDataComparer.cpp
    ../StorePerformanceCounters.cpp
    ../PerformanceCounterWriter.cpp
    ../CheckpointPerformanceCounterWriter.cpp
    ../CopyPerformanceCounterWriter.cpp
)

include_directories("..")

add_library(objects_TStore OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_TStore ../stdafx.h)

add_library(${lib_TStore} STATIC $<TARGET_OBJECTS:objects_TStore>)
target_link_libraries(${lib_TStore}
    ${lib_TransactionalReplicator}
    ${Ktlfull}
)

set_target_properties(${lib_TStore} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

