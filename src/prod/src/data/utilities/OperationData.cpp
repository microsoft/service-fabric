// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

ULONG32 NULL_OPERATION_DATA_COUNT = MAXULONG;

OperationData::OperationData()
    : KObject()
    , KShared()
    , buffers_(GetThisAllocator())
{
    if (!NT_SUCCESS(buffers_.Status()))
    {
        throw Exception(buffers_.Status());
    }
}

OperationData::OperationData(__in KArray<KBuffer::CSPtr> const & buffers)
    : KObject()
    , KShared()
    , buffers_(buffers)
{
    if (!NT_SUCCESS(buffers_.Status()))
    {
        throw Exception(buffers_.Status());
    }
}

OperationData::~OperationData()
{
}

OperationData::SPtr OperationData::Create(__in KAllocator & allocator)
{
    OperationData* pointer = _new(OPERATIONDATA_TAG, allocator) OperationData();

    if (!pointer)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return OperationData::SPtr(pointer);
}

OperationData::CSPtr OperationData::Create(
    __in KArray<KBuffer::CSPtr> const & buffers,
    __in KAllocator & allocator)
{
    OperationData const * pointer = _new(OPERATIONDATA_TAG, allocator) OperationData(buffers);

    if (!pointer)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return OperationData::CSPtr(pointer);;
}

OperationData::CSPtr OperationData::Create(
    __in OperationData const& operationData, 
    __in KAllocator& allocator)
{
    return Create(
        operationData, 
        0, 
        operationData.BufferCount, 
        allocator);
}

OperationData::CSPtr OperationData::Create(
    __in OperationData const& operationData, 
    __in ULONG32 startingIndex, 
    __in ULONG32 count, 
    __in KAllocator& allocator)
{
    ASSERT_IFNOT(
        startingIndex + count <= operationData.BufferCount, 
        "Starting index: {0} + count: {1} should be <= operation data buffer count: {2}",
        startingIndex, count, operationData.BufferCount);

    if (count == 0)
    {
        return Create(allocator).RawPtr();
    }

    OperationData * pointer = _new(OPERATIONDATA_TAG, allocator) OperationData(allocator);
 
    if (!pointer)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Shallow copy
    for (ULONG32 i = startingIndex; i < startingIndex + count; i++)
    {
        pointer->Append(*operationData[i]);
    }

    return OperationData::CSPtr(pointer);
}

KBuffer::CSPtr OperationData::IncrementIndexAndGetBuffer(
    __in OperationData const & operationData,
    __inout INT & index)
{
    index += 1;
    return operationData[index];
}

void OperationData::Serialize(
    __inout BinaryWriter & binaryWriter,
    __in_opt OperationData const * const toSerialize,
    __inout OperationData & serializedOutput)
{
    ULONG startPos = binaryWriter.Position;

    if (!toSerialize)
    {
        binaryWriter.Write(NULL_OPERATION_DATA_COUNT);
        KBuffer::SPtr countBuffer = binaryWriter.GetBuffer(startPos);
        serializedOutput.Append(*countBuffer);
        return;
    }

    // Ensure maximum number of buffers is less than the MAXULONG as this is used for encoding an empty buffer
    ASSERT_IFNOT(toSerialize->BufferCount < MAXULONG, "Buffer count should be less than MAXULONG");

    binaryWriter.Write(toSerialize->BufferCount);

    KBuffer::SPtr buffer = binaryWriter.GetBuffer(startPos);
    serializedOutput.Append(*buffer);

    for (ULONG i = 0; i < toSerialize->BufferCount; i++)
    {
        ULONG32 size = (*toSerialize)[i]->QuerySize();

        startPos = binaryWriter.Position;
        binaryWriter.Write(size);
        buffer = binaryWriter.GetBuffer(startPos);
        serializedOutput.Append(*buffer);
 
        // NOTE: Include the buffer only if it has some data. Otherwise, V1 replicator fails replication
        if (size > 0)
        {
            serializedOutput.Append(*(*toSerialize)[i]);
        }
    }
}

OperationData::CSPtr OperationData::DeSerialize(
    __in OperationData const & inputData,
    __inout INT & index,
    __in KAllocator & allocator)
{
    auto buffer = IncrementIndexAndGetBuffer(inputData, index);
    BinaryReader reader(*buffer, allocator);

    ULONG32 bufferCount = 0;
    reader.Read(bufferCount);

    if (bufferCount == NULL_OPERATION_DATA_COUNT)
    {
        return nullptr;
    }

    KArray<KBuffer::CSPtr> items(allocator);
    if (!NT_SUCCESS(items.Status()))
    {
        throw Exception(items.Status());
    }

    NTSTATUS status;
    for (ULONG i = 0; i < bufferCount; i++)
    {
        KBuffer::CSPtr lBuffer = DeSerializeBytes(inputData, index, allocator);
        status = items.Append(lBuffer);

        if (!NT_SUCCESS(status))
        {
            throw Exception(status);
        }
    }

    OperationData::CSPtr constOperationData = OperationData::Create(items, allocator);

    return constOperationData;
}

OperationData::CSPtr OperationData::DeSerialize(
    __in BinaryReader & binaryReader,
    __in KAllocator & allocator)
{
    ULONG32 bufferCount = 0;

    binaryReader.Read(bufferCount);

    if (bufferCount == NULL_OPERATION_DATA_COUNT)
    {
        return nullptr;
    }

    KArray<KBuffer::CSPtr> items(allocator);
    if (!NT_SUCCESS(items.Status()))
    {
        throw Exception(items.Status());
    }

    NTSTATUS status;

    for (ULONG i = 0; i < bufferCount; i++)
    {
        KBuffer::CSPtr buffer = DeSerializeBytes(binaryReader, allocator);
        status = items.Append(buffer);

        if (!NT_SUCCESS(status))
        {
            throw Exception(status);
        }
    }

    OperationData::CSPtr constOperationData = OperationData::Create(items, allocator);

    return constOperationData;
}

KBuffer::CSPtr OperationData::DeSerializeBytes(
    __in OperationData const & inputData,
    __inout INT & index,
    __in KAllocator & allocator)
{
    auto buffer = IncrementIndexAndGetBuffer(inputData, index);
    BinaryReader reader(*buffer, allocator);

    ULONG32 count = 0;
    reader.Read(count);

    if (count == 0)
    {
        KBuffer::CSPtr returnBuffer;
        KBuffer::SPtr emptyBuffer;

        NTSTATUS status = KBuffer::Create(0, emptyBuffer, allocator);
        if (!NT_SUCCESS(status))
        {
            throw Exception(status);
        }

        returnBuffer = emptyBuffer.RawPtr();

        return returnBuffer;
    }

    return IncrementIndexAndGetBuffer(inputData, index);
}

KBuffer::CSPtr OperationData::DeSerializeBytes(
    __in BinaryReader & binaryReader,
    __in KAllocator & allocator)
{
    ULONG32 size;
    binaryReader.Read(size);
    
    KBuffer::SPtr buffer = nullptr;

    NTSTATUS status = KBuffer::Create(
        size,
        buffer,
        allocator,
        BINARYREADER_TAG);

    if (!NT_SUCCESS(status))
    {
        throw Exception(status);
    }

    if(size > 0)
    {
        binaryReader.Read(size, buffer);
    }

    return KBuffer::CSPtr(buffer.RawPtr());
}

LONG64 OperationData::GetOperationSize(
    __in OperationData const & operation)
{
    LONG64 size = 0;
    for (ULONG32 i = 0; i < operation.BufferCount; i++)
    {
        size += operation[i]->QuerySize();
    }

    return size;
}

void OperationData::Append(__in KBuffer const & buffer)
{
    ASSERT_IFNOT(BufferCount + 1 < MAXULONG, "Invalid buffer count: {0}", BufferCount);

    KBuffer::CSPtr sharedBuffer(&buffer); 
    NTSTATUS status = buffers_.Append(sharedBuffer);

    if (!NT_SUCCESS(status))
    {
        throw Exception(status);
    }
}

void OperationData::Append(__in OperationData const & operationData)
{
    ASSERT_IFNOT(
        BufferCount + operationData.BufferCount < MAXULONG, 
        "Invalid total buffer count: {0}", 
        BufferCount + operationData.BufferCount);

    INT intBufferCount = static_cast<INT>(operationData.BufferCount);
    NTSTATUS status = STATUS_SUCCESS;

    for (INT index = 0; index < intBufferCount; index++)
    {
        status = buffers_.Append(operationData[index]);
        if (!NT_SUCCESS(status))
        {
            throw Exception(status);
        }
    }
}

void OperationData::InsertAt0(__in KBuffer const & buffer)
{
    ASSERT_IFNOT(BufferCount + 1 < MAXULONG, "Invalid buffer count: {0}", BufferCount);

    KBuffer::CSPtr sharedBuffer(&buffer); 
    NTSTATUS status = buffers_.InsertAt(0, sharedBuffer);

    if (!NT_SUCCESS(status))
    {
        throw Exception(status);
    }
}

bool OperationData::Test_Equals(__in OperationData const & other) const
{
    if(buffers_.Count() != other.buffers_.Count())
    {
        return false;
    }

    ULONG bufferSize;

    for(ULONG i = 0; i < buffers_.Count(); i++)
    {
        bufferSize = buffers_[i]->QuerySize();
        if(bufferSize != other.buffers_[i]->QuerySize())
        {
            return false;
        }

        // NOTE: This is due to a bug in KBuffer which requires the left hand value to be const
        KBuffer::CSPtr currentConstBuffer = buffers_[i];
        KBuffer const & currentBufferRef = *currentConstBuffer;
        KBuffer & currentBuffer = const_cast<KBuffer &>(currentBufferRef);

        if(currentBuffer != *other.buffers_[i])
        {
            return false;
        }
    }

    return true;
}
