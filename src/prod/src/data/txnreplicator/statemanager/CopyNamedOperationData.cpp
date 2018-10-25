// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

const ULONG32 CopyNamedOperationData::NativeContextHeaderSize = sizeof(ULONG32) + sizeof(FABRIC_STATE_PROVIDER_ID);
const ULONG32 CopyNamedOperationData::NativeHeaderCount = 1;

const LONG32 CopyNamedOperationData::ManagedVersion = 1;
const LONG32 CopyNamedOperationData::ManagedVersionBufferSize = sizeof(LONG32);
const LONG32 CopyNamedOperationData::ManagedStateProviderBufferSize = sizeof(FABRIC_STATE_PROVIDER_ID);
const ULONG32 CopyNamedOperationData::ManagedHeaderCount = 2;

const ULONG32 CopyNamedOperationData::CurrentWriteMetadataOperationDataCount = 1;

NTSTATUS CopyNamedOperationData::Create(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in SerializationMode::Enum serailizationMode,
    __in KAllocator & allocator,
    __in OperationData const & userOperationData,
    __out CSPtr & result) noexcept
{
    ULONG32 metadataCount = serailizationMode == SerializationMode::Native ?
        CurrentWriteMetadataOperationDataCount :
        ManagedHeaderCount;

    result = _new(COPY_NAMED_OPERATION_DATA_TAG, allocator) CopyNamedOperationData(
        metadataCount,
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

NTSTATUS CopyNamedOperationData::Create(
    __in KAllocator & allocator,
    __in OperationData const & inputOperationData,
    __out CSPtr & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // State Manager always at least creates OperationData of one buffer to keeps dispatching information.
    ASSERT_IFNOT(inputOperationData.BufferCount > 0, "Operation data should have at least one buffer during create");

    // De-serialize the operationData
    ULONG32 metadataCount = 0;
    FABRIC_STATE_PROVIDER_ID stateProviderId = 0;
    OperationData::CSPtr userOperationData = nullptr;
    status = DeserializeContext(inputOperationData, allocator, metadataCount, stateProviderId, userOperationData);
    RETURN_ON_FAILURE(status);

    result = _new(COPY_NAMED_OPERATION_DATA_TAG, allocator) CopyNamedOperationData(
        metadataCount, 
        stateProviderId, 
        *userOperationData);
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

FABRIC_STATE_PROVIDER_ID CopyNamedOperationData::get_StateProviderId() const noexcept
{
    return stateProviderId_;
}

OperationData::CSPtr CopyNamedOperationData::get_UserOperationData() const noexcept
{
    return userOperationDataCSPtr_;
}

NTSTATUS CopyNamedOperationData::DeserializeContext(
    __in OperationData const & operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    ASSERT_IFNOT(operationData.BufferCount > 0, "Operation data should have at least one buffer during de-serialize context");

    // Detect whether this is a managed or native format.
    KBuffer::CSPtr headerBufferSPtr = (operationData)[0];
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null header buffer");
    if (headerBufferSPtr->QuerySize() == NativeContextHeaderSize)
    {
        return DeserializeNativeContext(operationData, allocator, metadataOperationDataCount, stateProviderId, userOperationData);
    }

    return DeserializeManagedContext(operationData, allocator, metadataOperationDataCount, stateProviderId, userOperationData);
}

NTSTATUS CopyNamedOperationData::DeserializeNativeContext(
    __in OperationData const & operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    KBuffer::CSPtr headerBufferSPtr((operationData)[0]);
    ASSERT_IFNOT(headerBufferSPtr != nullptr, "Null header buffer");

    ASSERT_IFNOT(
        headerBufferSPtr->QuerySize() == NativeContextHeaderSize,
        "(Native) CopyNamedOperationData header buffer size: {0} not equal to context header size: {1}",
        headerBufferSPtr->QuerySize(), NativeContextHeaderSize);

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
    binaryReader.Read(stateProviderId);

    if (operationData.BufferCount == metadataOperationDataCount)
    {
        userOperationData = OperationData::Create(allocator).RawPtr();
        return STATUS_SUCCESS;
    }

    ULONG32 userOperationDataCount = operationData.BufferCount - metadataOperationDataCount;

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    userOperationData = OperationData::Create(operationData, metadataOperationDataCount, userOperationDataCount, allocator);
    return STATUS_SUCCESS;
}

NTSTATUS CopyNamedOperationData::DeserializeManagedContext(
    __in OperationData const & operationData,
    __in KAllocator & allocator,
    __out ULONG32 & metadataOperationDataCount,
    __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
    __out OperationData::CSPtr & userOperationData) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(operationData.BufferCount >= ManagedHeaderCount, 
        "Managed: Unexpected Buffer Count: {0} Expected: {1}",
        operationData.BufferCount,
        ManagedHeaderCount);

    // Read version number.
    {
        KBuffer::CSPtr versionBufferCSptr((operationData)[0]);
        ASSERT_IFNOT(versionBufferCSptr != nullptr, "Null header buffer");
        ASSERT_IFNOT(
            versionBufferCSptr->QuerySize() == ManagedVersionBufferSize,
            "Unexpected state provider buffer size. ",
            ManagedVersionBufferSize,
            versionBufferCSptr->QuerySize());

        LONG32 version;
        BinaryReader versionBufferReader(*versionBufferCSptr, allocator);
        status = versionBufferReader.ReadByBytes(version);
        RETURN_ON_FAILURE(status);
        ASSERT_IFNOT(
            version >= ManagedVersion, 
            "Managed: Unexpected Version: {0} Expected: {1}",
            version,
            ManagedVersion);
    }

    // Read state provider id.
    {
        KBuffer::CSPtr stateProviderBufferCSptr((operationData)[1]);
        ASSERT_IFNOT(stateProviderBufferCSptr != nullptr, "Null header buffer");
        ASSERT_IFNOT(
            stateProviderBufferCSptr->QuerySize() == ManagedStateProviderBufferSize, 
            "Unexpected state provider buffer size. Expect: {0} Size: {1}",
            ManagedStateProviderBufferSize,
            stateProviderBufferCSptr->QuerySize());

        BinaryReader versionBufferReader(*stateProviderBufferCSptr, allocator);
        status = versionBufferReader.ReadByBytes(stateProviderId);
        RETURN_ON_FAILURE(status);
    }

    metadataOperationDataCount = ManagedHeaderCount;
    if (operationData.BufferCount == metadataOperationDataCount)
    {
        userOperationData = OperationData::Create(allocator).RawPtr();
        return STATUS_SUCCESS;
    }

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    userOperationData = OperationData::Create(operationData, metadataOperationDataCount, operationData.BufferCount - metadataOperationDataCount, allocator);
    return STATUS_SUCCESS;
}

NTSTATUS CopyNamedOperationData::Serialize(
    __in SerializationMode::Enum mode) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(userOperationDataCSPtr_ != nullptr, "User operation data cannot be nullptr.");

    if (mode == SerializationMode::Native)
    {
        status = CreateNativeContext();
        RETURN_ON_FAILURE(status);

        ASSERT_IFNOT(
            BufferCount == NativeHeaderCount, 
            "Native: Expected Header Count: {0} Count: {1}",
            NativeHeaderCount,
            BufferCount);
    }
    else 
    {
        status = CreateManagedContext();
        RETURN_ON_FAILURE(status);

        ASSERT_IFNOT(
            BufferCount == ManagedHeaderCount,
            "Managed: Expected Header Count: {0} Count: {1}",
            ManagedHeaderCount,
            BufferCount);
    }

    for (ULONG32 i = 0; i < userOperationDataCSPtr_->BufferCount; i++)
    {
        KBuffer::CSPtr bufferSPtr = (*userOperationDataCSPtr_)[i];

        // #10155818: This operation can throw in OOM cases. 
        // Since this is a fatal case, it is OK to bring down the process.
        Append(*bufferSPtr);
    }

    return status;
}

NTSTATUS CopyNamedOperationData::CreateNativeContext() noexcept
{
    OperationData::SPtr headerOperationDataSPtr = OperationData::Create(GetThisAllocator());

    // #10156411: BinaryWriter is not noexcept.
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    if (NT_SUCCESS(binaryWriter.Status()) == false)
    {
        return binaryWriter.Status();
    }

    // #10156411: BinaryWriter is not noexcept.
    // For now, we depend on
    // - Write operation will only fail in OOM scenarios where we excepted the process to go down anyways.
    // MCOSKUN: Order is important for packing.
    binaryWriter.Write(metadataCount_);
    binaryWriter.Write(stateProviderId_);

    ASSERT_IFNOT(
        binaryWriter.Position == static_cast<ULONG>(NativeContextHeaderSize),
        "Unexpected header size. Expected: {0} Size: {1}",
        NativeContextHeaderSize,
        binaryWriter.Position);

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    Append(*binaryWriter.GetBuffer(0));

    return STATUS_SUCCESS;
}

NTSTATUS CopyNamedOperationData::CreateManagedContext() noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // #10156411: BinaryWriter is not noexcept.
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    if (NT_SUCCESS(binaryWriter.Status()) == false)
    {
        return binaryWriter.Status();
    }

    // Step 1: Write Managed Version.
    status = binaryWriter.WriteInBytes(ManagedVersion);
    RETURN_ON_FAILURE(status);

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    Append(*binaryWriter.GetBuffer(0));
    binaryWriter.Position = 0;

    // Step 2: Write State ProviderId.
    status = binaryWriter.WriteInBytes(stateProviderId_);
    RETURN_ON_FAILURE(status);

    // #10155818: This operation can throw in OOM cases. 
    // Since this is a fatal case, it is OK to bring down the process.
    Append(*binaryWriter.GetBuffer(0));
    binaryWriter.Position = 0;

    return STATUS_SUCCESS;
}

CopyNamedOperationData::CopyNamedOperationData(
    __in ULONG32 metadataCount,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in OperationData const & userOperationData) noexcept
    : OperationData()
    , metadataCount_(metadataCount)
    , stateProviderId_(stateProviderId)
    , userOperationDataCSPtr_(&userOperationData)
{
    // In primed (do not serialize)
}

CopyNamedOperationData::CopyNamedOperationData(
    __in ULONG32 metadataCount,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in SerializationMode::Enum mode,
    __in OperationData const & userOperationData) noexcept
    : OperationData()
    , metadataCount_(metadataCount)
    , stateProviderId_(stateProviderId)
    , userOperationDataCSPtr_(&userOperationData)
{
    NTSTATUS status = Status();
    if (NT_SUCCESS(status) == false)
    {
        return;
    }

    status = Serialize(mode);
    SetConstructorStatus(status);
}

CopyNamedOperationData::~CopyNamedOperationData()
{
}
