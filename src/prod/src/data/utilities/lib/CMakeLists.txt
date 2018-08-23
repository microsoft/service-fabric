set( LINUX_SOURCES
  ../AsyncLock.cpp
  ../BinaryReader.cpp
  ../BinaryWriter.cpp
  ../BlockHandle.cpp
  ../ComOperationData.cpp
  ../ComProxyOperationData.cpp
  ../CRC64.cpp
  ../FileFooter.cpp
  ../FileProperties.cpp
  ../KAsyncEventHelper.cpp
  ../KPath.cpp
  ../LockControlBlock.cpp
  ../LockHashTable.cpp
  ../LockHashValue.cpp
  ../LockManager.cpp
  ../LockModeComparer.cpp
  ../LockResourceControlBlock.cpp
  ../LongComparer.cpp
  ../MemoryStream.cpp
  ../OperationData.cpp
  ../IntComparer.cpp
  ../KStringComparer.cpp
  ../PartitionedReplicaId.cpp
  ../Partition.cpp
  ../PrimeLockRequest.cpp
  ../RandomGenerator.cpp
  ../ReaderWriterAsyncLock.cpp
  ../SharedException.cpp
  ../StatusConverter.cpp
  ../ULONG32Comparer.cpp
  ../UnsignedLongComparer.cpp
  ../VarInt.cpp
)

include_directories("..")

add_library(objects_data_utilities OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_data_utilities ../stdafx.h)

add_library(${lib_data_utilities} STATIC $<TARGET_OBJECTS:objects_data_utilities>)
target_link_libraries(${lib_data_utilities}
    ${Ktlfull}
    )

set_target_properties(${lib_data_utilities} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

