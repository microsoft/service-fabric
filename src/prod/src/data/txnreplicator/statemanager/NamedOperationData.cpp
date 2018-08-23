// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

const LONG32 NamedOperationData::NullUserOperationData = -1;
const ULONG32 NamedOperationData::NativeContextHeaderSize = sizeof(ULONG32) + sizeof(ULONG32) + sizeof(FABRIC_STATE_PROVIDER_ID);
const ULONG32 NamedOperationData::ManagedContextHeaderSize = sizeof(ULONG32) + sizeof(FABRIC_STATE_PROVIDER_ID) + sizeof(bool);
const LONG32 NamedOperationData::ManagedVersion = 1;
const ULONG32 NamedOperationData::CurrentWriteMetadataOperationDataCount = 1;

NTSTATUS NamedOperationData::Create(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in SerializationMode::Enum serailizationMode,
    __in KAllocator & allocator,
    __in_opt OperationData const * const userOperationData,
    __out CSPtr & result) noexcept
{
    result = _new(NAMED_OPERATION_DATA_TAG, allocator) NamedOperationData(
        CurrentWriteMetadataOperationDataCount, 
        stateProviderId, 
        serailizationMode,
        userOperationData);
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

NTSTATUS NamedOperationData::Create(
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationDataPtr,
    __out CSPtr & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // State Manager always at least creates OperationData of one buffer to keeps dispatching information.
    ASSERT_IFNOT(operationDataPtr != nullptr, "Null named operation data during create");
    ASSERT_IFNOT(operationDataPtr->BufferCount > 0, "Named operation data should have at least one buffer during create");

    // De-serialize the operationData
    ULONG32 metadataCount = 0;
    FABRIC_STATE_PROVIDER_ID stateProviderId = 0;
    OperationData::CSPtr userOperationData = nullptr;
    status = DeserializeContext(operationDataPtr, allocator, metadataCount, stateProviderId, userOperationData);
    RETURN_ON_FAILURE(status);

    result = _new(NAMED_OPERATION_DATA_TAG, allocator) NamedOperationData(
        metadataCount, 
        stateProviderId, 
        userOperationData.RawPtr());
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

FABRIC_STATE_PROVIDER_ID NamedOperationData::get_StateProviderId() const noexcept
{
    return stateProviderId_;
}

OperationData::CSPtr NamedOperationData::get_UserOperationData() const noexcept
{
    return userOperationDataCSPtr_;
}

NTSTATUS NamedOperationData::DeserializeContext(
    __in OperationData const * const operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    ASSERT_IFNOT(operationData != nullptr, "Null named operation data during deserialize context");
    ASSERT_IFNOT(operationData->BufferCount > 0, "Named operation data should have at least one buffer during deserialize context");

    // Detect whether this is a managed or native format.
    KBuffer::CSPtr headerBufferSPtr = (*operationData)[operationData->BufferCount - 1];
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null named operation header buffer");
    if (headerBufferSPtr->QuerySize() == NativeContextHeaderSize)
    {
        return DeserializeNativeContext(*operationData, allocator, metadataOperationDataCount, stateProviderId, userOperationData);
    }

    return DeserializeManagedContext(*operationData, allocator, metadataOperationDataCount, stateProviderId, userOperationData);
}

NTSTATUS NamedOperationData::DeserializeNativeContext(
    __in OperationData const & operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    KBuffer::CSPtr headerBufferSPtr = (operationData)[operationData.BufferCount - 1];
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null named operation header buffer");

    ASSERT_IFNOT(
        headerBufferSPtr->QuerySize() == NativeContextHeaderSize,
        "(Native) NamedOperationData header buffer size: {0} not equal to context header size: {1}",
        headerBufferSPtr->QuerySize(), NativeContextHeaderSize);

    ULONG32 userOperationDataCount;

    // #10156411: BinaryReader is not noexcept.
    // For now this is OK to assume binary reader only fails in OOM scenarios where we except the process to go down.
    BinaryReader binaryReader(*headerBufferSPtr, allocator);
    if (NT_SUCCESS(binaryReader.Status()) == false)
    {
        userOperationData = nullptr;
        return binaryReader.Status();
    }

    // #10156411: BinaryReader is not noexcept.
    // For now this is OK to assume binary reader only fails in OOM scenarios where we except the process to go down.
    binaryReader.Read(metadataOperationDataCount);
    binaryReader.Read(userOperationDataCount);
    binaryReader.Read(stateProviderId);

    if (userOperationDataCount == -1)
    {
        ASSERT_IFNOT(
            operationData.BufferCount == metadataOperationDataCount,
            "Operation data buffer count: {0} not equal to metadata operation data count: {1}",
            operationData.BufferCount, 
            metadataOperationDataCount);
    }
    else
    {
        ASSERT_IFNOT(
            operationData.BufferCount == metadataOperationDataCount + userOperationDataCount,
            "Operation data buffer count: {0} not equal to metadata+user operation data count: {1}",
            operationData.BufferCount, 
            metadataOperationDataCount + userOperationDataCount);
    }

    if (userOperationDataCount == NullUserOperationData)
    {
        userOperationData = nullptr;
        return STATUS_SUCCESS;
    }

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    userOperationData = OperationData::Create(operationData, 0, userOperationDataCount, allocator);

    return STATUS_SUCCESS;
}

NTSTATUS NamedOperationData::DeserializeManagedContext(
    __in OperationData const & operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    KBuffer::CSPtr headerBufferSPtr = (operationData)[operationData.BufferCount - 1];
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null named operation header buffer");

    ASSERT_IFNOT(
        headerBufferSPtr->QuerySize() == ManagedContextHeaderSize,
        "(Managed) NamedOperationData header buffer size: {0} not equal to context header size: {1}",
        headerBufferSPtr->QuerySize(), ManagedContextHeaderSize);

    LONG32 userOperationDataCount = -2;

    // #10156411: BinaryReader is not noexcept.
    // For now this is OK to assume binary reader only fails in OOM scenarios where we except the process to go down.
    BinaryReader binaryReader(*headerBufferSPtr, allocator);
    if (NT_SUCCESS(binaryReader.Status()) == false)
    {
        userOperationData = nullptr;
        return binaryReader.Status();
    }

    // #10156411: BinaryReader is not noexcept.
    // For now this is OK to assume binary reader only fails in OOM scenarios where we except the process to go down.
    // Serialization Step A: Read Version and set metadataOperationDataCount.
    LONG32 version = 0;
    binaryReader.Read(version);
    metadataOperationDataCount = 1;

    // Serialization Step B: Read state provider ID
    binaryReader.Read(stateProviderId);

    // Serialization Step C: Read whether state provider
    bool isUserOperationDataNull;
    binaryReader.Read(isUserOperationDataNull);
    if (isUserOperationDataNull)
    {
        userOperationDataCount = NullUserOperationData;
        ASSERT_IFNOT(
            operationData.BufferCount == metadataOperationDataCount,
            "Operation data buffer count: {0} not equal to metadata operation data count: {1}",
            operationData.BufferCount,
            metadataOperationDataCount);

        userOperationData = nullptr;
        return STATUS_SUCCESS;
    }

    userOperationDataCount = operationData.BufferCount - metadataOperationDataCount;

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    userOperationData = OperationData::Create(operationData, 0, userOperationDataCount, allocator);

    return STATUS_SUCCESS;
}

NTSTATUS NamedOperationData::Serialize(
    __in SerializationMode::Enum mode) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (userOperationDataCSPtr_ != nullptr)
    {
        for (ULONG32 i = 0; i < userOperationDataCSPtr_->BufferCount; i++)
        {
            KBuffer::CSPtr bufferSPtr = (*userOperationDataCSPtr_)[i];

            // #10155818: This operation can throw in OOM cases. 
            // Since this is a fatal case, it is OK to bring down the process.
            Append(*bufferSPtr);
        }
    }

    OperationData::CSPtr contextSPtr = nullptr;
    if (mode == SerializationMode::Native)
    {
        status = CreateNativeContext(contextSPtr);
    }
    else 
    {
        status = CreateManagedContext(contextSPtr);
    }

    if (NT_SUCCESS(status) == false)
    {
        return status;
    }

    ASSERT_IFNOT(contextSPtr != nullptr, "Null named operation context during serialize");
    ASSERT_IFNOT(contextSPtr->BufferCount > 0, "Named operation context should have at least one buffer during serialize");

    for (ULONG32 i = 0; i < contextSPtr->BufferCount; i++)
    {
        KBuffer::CSPtr bufferSPtr = (*contextSPtr)[i];

        // #10155818: This operation can throw in OOM cases. 
        // Since this is a fatal case, it is OK to bring down the process.
        Append(*bufferSPtr);
    }

    return status;
}

NTSTATUS NamedOperationData::CreateNativeContext(
    __out OperationData::CSPtr & result) noexcept
{
    OperationData::SPtr headerOperationDataSPtr = OperationData::Create(GetThisAllocator());

    ULONG32 operationDataCount = userOperationDataCSPtr_ == nullptr ? -1 : userOperationDataCSPtr_->BufferCount;

    // #10156411: BinaryWriter is not noexcept.
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    if (NT_SUCCESS(binaryWriter.Status()) == false)
    {
        result = nullptr;
        return binaryWriter.Status();
    }

    // #10156411: BinaryWriter is not noexcept.
    // For now, we depend on
    // - Write operation will only fail in OOM scenarios where we excepted the process to go down anyways.
    // MCOSKUN: Order is important for packing.
    binaryWriter.Write(metadataCount_);
    binaryWriter.Write(operationDataCount);
    binaryWriter.Write(stateProviderId_);

    ASSERT_IFNOT(
        binaryWriter.Position == static_cast<ULONG>(NativeContextHeaderSize),
        "Unexpected header size. Expected: {0} Size: {1}",
        NativeContextHeaderSize,
        binaryWriter.Position);

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    headerOperationDataSPtr->Append(*binaryWriter.GetBuffer(0));

    result = headerOperationDataSPtr.RawPtr();

    return STATUS_SUCCESS;
}

NTSTATUS NamedOperationData::CreateManagedContext(
    __out OperationData::CSPtr & result) noexcept
{
    OperationData::SPtr headerOperationDataSPtr = OperationData::Create(GetThisAllocator());

    // #10156411: BinaryWriter is not noexcept.
    // Dependency: Only fail if OOM.
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    if (NT_SUCCESS(binaryWriter.Status()) == false)
    {
        result = nullptr;
        return binaryWriter.Status();
    }

    // #10156411: BinaryWriter is not noexcept.
    // For now, we depend on
    // - Write operation will only fail in OOM scenarios where we excepted the process to go down anyways.
    binaryWriter.Write(ManagedVersion);
    binaryWriter.Write(stateProviderId_);
    binaryWriter.Write(userOperationDataCSPtr_ == nullptr);

    ASSERT_IFNOT(
        binaryWriter.Position == static_cast<ULONG>(ManagedContextHeaderSize),
        "Unexpected header size. Expected: {0} Size: {1}",
        ManagedContextHeaderSize,
        binaryWriter.Position);

    // #10155818: This operation can throw in OOM cases. 
    // For now, we depend on
    // - Append operation will only fail in OOM scenarios where we excepted the process to go down anyways.
    headerOperationDataSPtr->Append(*binaryWriter.GetBuffer(0));

    result = headerOperationDataSPtr.RawPtr();

    return STATUS_SUCCESS;
}

NamedOperationData::NamedOperationData(
    __in ULONG32 metadataCount,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in_opt OperationData const * const userOperationData) noexcept
    : OperationData()
    , metadataCount_(metadataCount)
    , stateProviderId_(stateProviderId)
    , userOperationDataCSPtr_(userOperationData)
{
    // In primed (do not serialize)
}

NamedOperationData::NamedOperationData(
    __in ULONG32 metadataCount,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in SerializationMode::Enum mode,
    __in_opt OperationData const * const userOperationData) noexcept
    : OperationData()
    , metadataCount_(metadataCount)
    , stateProviderId_(stateProviderId)
    , userOperationDataCSPtr_(userOperationData)
{
    NTSTATUS status = Status();
    if (NT_SUCCESS(status) == false)
    {
        return;
    }

    status = Serialize(mode);
    SetConstructorStatus(status);
}

NamedOperationData::~NamedOperationData()
{
}
