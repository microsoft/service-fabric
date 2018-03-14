// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::StateManager;

const ULONG32 MetadataOperationData::WriteBufferCount = 1;

NTSTATUS MetadataOperationData::Create(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KUri const & stateProviderName,
    __in ApplyType::Enum applyType,
    __in KAllocator & allocator,
    __out CSPtr & result)
{
    result = _new(METADATA_OPERATION_DATA_TAG, allocator) MetadataOperationData(
        WriteBufferCount,
        stateProviderId,
        stateProviderName,
        applyType,
        TRUE);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS MetadataOperationData::Create(
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationDataPtr,
    __out CSPtr & result)
{
    // State Manager always at least creates OperationData of one buffer to keeps dispatching information.
    ASSERT_IFNOT(operationDataPtr != nullptr,  "Null metadata operation data during create");
    ASSERT_IFNOT(operationDataPtr->BufferCount > 0, "Metadata operation data should have atleast one buffer during create");

    // Deserialize the operationData
    ULONG32 bufferCount = 0;
    FABRIC_STATE_PROVIDER_ID stateProviderId = 0;
    KUri::SPtr stateProviderName;
    ApplyType::Enum applyType;

    Deserialize(allocator, operationDataPtr, bufferCount, stateProviderId, stateProviderName, applyType);

    result = _new(METADATA_OPERATION_DATA_TAG, allocator) MetadataOperationData(
        bufferCount,
        stateProviderId,
        *stateProviderName, 
        applyType,
        FALSE);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

FABRIC_STATE_PROVIDER_ID MetadataOperationData::get_StateProviderId() const
{
    return stateProviderId_;
}

KUri::CSPtr MetadataOperationData::get_StateProviderName() const
{
    return stateProviderNameSPtr_;
}

ApplyType::Enum MetadataOperationData::get_ApplyType() const
{
    return applyType_;
}

void MetadataOperationData::Deserialize(
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationData,
    __out ULONG32 & bufferCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out KUri::SPtr & stateProviderName,
    __out ApplyType::Enum & applyType)
{
    byte applyTypeByte;

    ASSERT_IFNOT(operationData != nullptr, "Null metadata operation data during deserialization");
    ASSERT_IFNOT(operationData->BufferCount > 0, "Metadata operation data should have atleast one buffer during deserialization");

    KBuffer::CSPtr headerBufferSPtr = (*operationData)[0];
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null metadata operation header buffer");

    Utilities::BinaryReader binaryReader(*headerBufferSPtr, allocator);
    binaryReader.Read(bufferCount);
    binaryReader.Read(stateProviderId);
    binaryReader.Read(stateProviderName);
    binaryReader.Read(applyTypeByte);
    applyType = static_cast<ApplyType::Enum>(applyTypeByte);

    // Version 1 of the code only understands Buffer[0] (Header).
    // Hence it ignores the rest of the buffers.
    // Note that Buffer[0] always must exist.
    ASSERT_IFNOT(
        operationData->BufferCount == bufferCount, 
        "Metadata operation buffer count: {0} not equal to actual buffer count: {1}",
        operationData->BufferCount, bufferCount);
}

void MetadataOperationData::Serialize()
{
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    binaryWriter.Write(bufferCount_);
    binaryWriter.Write(stateProviderId_);
    binaryWriter.Write(*stateProviderNameSPtr_);
    binaryWriter.Write(static_cast<byte>(applyType_));

    Append(*binaryWriter.GetBuffer(0));
}

MetadataOperationData::MetadataOperationData(
    __in ULONG32 bufferCount,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KUri const & stateProviderName,
    __in ApplyType::Enum applyType,
    __in bool primeOperationData)
    : OperationData()
    , bufferCount_(bufferCount)
    , stateProviderId_(stateProviderId)
    , stateProviderNameSPtr_(&stateProviderName)
    , applyType_(applyType)
{
    if (primeOperationData)
    {
        Serialize();
    }
}

MetadataOperationData::~MetadataOperationData()
{
}
