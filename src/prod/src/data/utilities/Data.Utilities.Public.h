// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !PLATFORM_UNIX
#include <sal.h>
#endif
#include <ktl.h>
#include <KTpl.h> 
#include <kstream.h>
#include <kfilestream.h>
#include "FabricRuntime_.h"
#include "Common/Common.h"
#include <KComAdapter.h>

#include "../data.public.h"

namespace Data
{
    namespace Utilities
    {
        using ::_delete;
    }
}

// Helper macro used in the product to throw exceptions on allocation failures
#define THROW_ON_ALLOCATION_FAILURE(pointer) \
    if (!pointer) \
    { \
        throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES); \
    } \

// Helper macro used in the product to throw exceptions on constructor status failures of sub objects used
#define THROW_ON_CONSTRUCTOR_FAILURE(object) \
    if (!NT_SUCCESS(object.Status())) \
    { \
        throw ktl::Exception(object.Status()); \
    } \

// Helper macro used in the product to convert NTSTATUS to exception
#define THROW_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) \
    { \
        throw ktl::Exception(status); \
    } \

#define RETURN_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) \
    { \
        return status; \
    } \

#define CO_RETURN_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) \
    { \
        co_return status; \
    } \

namespace Data
{
    namespace Utilities
    {
        class BinaryReader;
        class BinaryWriter;
    }
}

#include "Encoding.h"
#include "KAsyncEventHelper.h"
#include "KAutoHashTable.h"
#include "ThreadSafeSPtrCache.h"
#include "ThreadSafeSPtrCache.h"
#include "PartitionedReplicaId.h"
#include "PartitionedReplicaTraceComponent.h"
#include "VarInt.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "RandomGenerator.h"
#include "DefaultComparer.h"
#include "ShareableArray.h"
#include "ConcurrentSkipList.h"
#include "ConcurrentSkipListEnumerator.h"
#include "ConcurrentDictionary.h"
#include "ConcurrentHashTable.h"
#include "ConcurrentHashTableEnumerator.h"
#include "KHashSet.h"
#include "KHashSetEnumerator.h"
#include "SharedException.h"
#include "IDisposable.h"
#include "IAsyncEnumerator.h"
#include "IAsyncEnumeratorNoExcept.h"
#include "ReaderWriterAsyncLock.h"
#include "TaskUtilities.h"
#include "CRC64.h"
#include "BlockHandle.h"
#include "FileBlock.h"
#include "FileProperties.h"
#include "FileFooter.h"
#include "TraceUtilities.h"
#include "OperationData.h"
#include "ComOperationData.h"
#include "ComProxyOperationData.h"
#include "KStringComparer.h"
#include "IntComparer.h"
#include "LongComparer.h"
#include "UnsignedLongComparer.h"
#include "ULONG32Comparer.h"
#include "Sort.h"
#include "StatusConverter.h"
#include "MemoryStream.h"
#include "KPath.h"
#include "AsyncLock.h"
#include "IStatefulPartition.h"
#include "Partition.h"
#include "Dictionary.h"
#include "DictionaryEnumerator.h"
#include "LockStatus.h"
#include "LockMode.h"
#include "LockModeComparer.h"
#include "LockControlBlock.h"
#include "LockResourceControlBlock.h"
#include "LockHashValue.h"
#include "LockHashTable.h"
#include "LockManager.h"
#include "ILock.h"
#include "PrimeLockRequest.h"
