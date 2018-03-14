// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data;
using namespace Data::StateManager;

NTSTATUS RedoOperationData::Create(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in KString const & typeName,
    __in OperationData const * const initializationParameters,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in SerializationMode::Enum serailizationMode,
    __in KAllocator & allocator,
    __out CSPtr & result)
{
    LONG32 bufferCountIncludeHeader = 1;

    if(initializationParameters != nullptr)
    {
        // Buffer count in the Header buffer includes the header buffer, so its initialization parameters buffers add 1
        // Note: Buffer count must be less then MAXLONG32, not include MAXLONG32 here because we will add 1 for the header.
        ULONG32 bufferCount = initializationParameters->get_BufferCount();
        ASSERT_IFNOT(
            bufferCount < MAXLONG32,
            "{0}: InitializationParameters buffer count should not be larger then MAXLONG32. BufferCount: {1}",
            traceId.TraceId,
            bufferCount);
        bufferCountIncludeHeader = static_cast<LONG32>(bufferCount) + bufferCountIncludeHeader;
    }

    result = _new(REDO_OPERATION_DATA_TAG, allocator) RedoOperationData(
        traceId,
        bufferCountIncludeHeader,
        typeName,
        parentStateProviderId,
        serailizationMode,
        initializationParameters);
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

NTSTATUS RedoOperationData::Create(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationData,
    __out CSPtr & result)
{
    // State Manager always at least creates OperationData of one buffer to keeps dispatching information.
    ASSERT_IFNOT(
        operationData != nullptr, 
        "{0}: Null redo operation data during create.",
        traceId.TraceId);
    ASSERT_IFNOT(
        operationData->BufferCount > 0, 
        "{0}: Redo operation data should have atleast one buffer during create. BufferCount: {1}.",
        traceId.TraceId,
        operationData->BufferCount);

    // Deserialize the operationData
    LONG32 bufferCount;
    KString::SPtr typeName;
    OperationData::CSPtr initializationParameters;
    FABRIC_STATE_PROVIDER_ID parentId;

    NTSTATUS status = Deserialize(
        traceId, 
        allocator, 
        operationData, 
        bufferCount, 
        typeName, 
        initializationParameters, 
        parentId);
    RETURN_ON_FAILURE(status);

    result = _new(REDO_OPERATION_DATA_TAG, allocator) RedoOperationData(
        traceId, 
        bufferCount,
        *typeName,
        parentId,
        initializationParameters.RawPtr());
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

NTSTATUS RedoOperationData::Deserialize(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationData,
    __out LONG32 & bufferCount,
    __out KString::SPtr & typeName,
    __out OperationData::CSPtr & initializationParameters,
    __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept
{
    ASSERT_IFNOT(
        operationData != nullptr,
        "{0}: Null redo operation data during deserialization.",
        traceId.TraceId);
    ASSERT_IFNOT(
        operationData->BufferCount > 0,
        "{0}: Redo operation data should have atleast one buffer during deserialization. BufferCount: {1}.",
        traceId.TraceId,
        operationData->BufferCount);

    KBuffer::CSPtr headerBufferSPtr = (*operationData)[0];
    ASSERT_IFNOT(
        headerBufferSPtr != nullptr, 
        "{0}: Null redo operation header buffer.",
        traceId.TraceId);

    Utilities::BinaryReader binaryReader(*headerBufferSPtr, allocator);
    RETURN_ON_FAILURE(binaryReader.Status());

    binaryReader.Read(typeName);
    binaryReader.Read(bufferCount);

    // To deserialize the RedoOperationData, first we read the typeName, which is string in both 
    // manged and native. Then we will read the BufferCount, which indicates the deserialization method.
    // For the manged code, the InitialzationParameters is a single buffer, so it writes the size of the 
    // buffer and if it is null, writes -1. While in the native code, the InitialzationParameters is a 
    // KArray of buffers, to differential both versions, we write the complement of the BufferCount, 
    // and write -1 as managed if it is nullptr.
    // So when we read the BufferCount, we can decide which way to deserialize the RedoOperationData.
    // If the bufferCount equals NULL_OPERATIONDATA(-1)
    //      It means the InitialzationParameters is nullptr, it fitx for both versions.
    // If the bufferCount is not negative.
    //      It means the code is in managed version, and the BufferCount is the size of the InitialzationParameters.
    //      since in the manged, the InitialzationParameters is a single buffer.
    // If the bufferCount is negative.
    //      It means the code version is native. And the complement of the bufferCount is the bufferCountIncludeHeader.
    //      Since this count includes header, the real InitialzationParameters Count should be bufferCountIncludeHeader - 1.
    if (bufferCount == NULL_OPERATIONDATA)
    {
        initializationParameters = nullptr;
        binaryReader.Read(parentId);
        ASSERT_IFNOT(
            operationData->BufferCount == 1,
            "{0}: Null initializationParameters case should have only one buffer.",
            traceId.TraceId);
        return STATUS_SUCCESS;
    }

    if (bufferCount >= 0)
    {
        return DeserializeManagedOperation(
            traceId,
            binaryReader,
            allocator,
            bufferCount,
            initializationParameters,
            parentId);
    }

    return DeserializeNativeOperation(
        traceId,
        binaryReader,
        allocator,
        operationData,
        bufferCount,
        initializationParameters,
        parentId);
}

NTSTATUS RedoOperationData::DeserializeNativeOperation(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in Utilities::BinaryReader & reader,
    __in KAllocator & allocator,
    __in_opt OperationData const * const operationData,
    __out LONG32 & bufferCount,
    __out OperationData::CSPtr & initializationParameters,
    __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept
{
    ASSERT_IFNOT(
        bufferCount < NULL_OPERATIONDATA,
        "{0}: BufferCount must be less then NULL_OPERATIONDATA in the deserialization native case. BufferCount: {1}.",
        traceId.TraceId,
        bufferCount);

    reader.Read(parentId);

    // Take the complement of the bufferCount to get the native buffer count
    bufferCount = ~bufferCount;

    OperationData::SPtr initParameters = OperationData::Create(allocator);

    for (LONG32 index = 1; index < bufferCount; index++)
    {
        KBuffer::SPtr buffer = const_cast<KBuffer *>((*operationData)[index].RawPtr());
        initParameters->Append(*buffer);
    }

    initializationParameters = initParameters.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS RedoOperationData::DeserializeManagedOperation(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in Utilities::BinaryReader & reader,
    __in KAllocator & allocator,
    __out LONG32 & bufferCount,
    __out OperationData::CSPtr & initializationParameters,
    __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept
{
    ASSERT_IFNOT(
        bufferCount >= 0,
        "{0}: BufferCount must be not negative in the deserialization managed case. BufferCount: {1}.",
        traceId.TraceId,
        bufferCount);

    // For the managed version: initializationParameters always has single buffer.
    OperationData::SPtr initParameters = OperationData::Create(allocator);

    if (bufferCount == 0)
    {
        initializationParameters = initParameters.RawPtr();
    }
    else
    {
        KBuffer::SPtr buffer = nullptr;
        NTSTATUS status = KBuffer::Create(
            bufferCount,
            buffer,
            allocator,
            REDO_OPERATION_DATA_TAG);
        RETURN_ON_FAILURE(status);

        // Since in the managed version there is only one buffer and the bufferCount
        // indicates the size of the buffer.
        // Note: Using the Read function with length, otherwise it will read size and then
        // buffer, which introduces two size read.
        reader.Read(bufferCount, buffer);

        initParameters->Append(*buffer.RawPtr());

        initializationParameters = initParameters.RawPtr();
    }

    reader.Read(parentId);

    return STATUS_SUCCESS;
}

NTSTATUS RedoOperationData::Serialize(
    __in SerializationMode::Enum serailizationMode) noexcept
{
    if (serailizationMode == SerializationMode::Managed || initializationParametersCSPtr_ == nullptr)
    {
        return this->SerializeManagedOperation();
    }

    ASSERT_IFNOT(
        serailizationMode == SerializationMode::Native,
        "{0}: SerailizationMode must be native.",
        TraceId);
    return this->SerializeNativeOperation();
}

// Note: The function helps to serialize Native RedoOperationData
NTSTATUS RedoOperationData::SerializeNativeOperation() noexcept
{
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    RETURN_ON_FAILURE(binaryWriter.Status());

    binaryWriter.Write(*typeNameSPtr_);

    ASSERT_IFNOT(
        bufferCountIncludeHeader_ >= 1,
        "{0}: RedoOperationData must at least have one buffer. BufferSize: {1}.",
        TraceId,
        bufferCountIncludeHeader_);

    // For native code, we take a complement of the buffer count.
    // BufferCountIncludeHeader at least has one buffer and the case it equals to 
    // one is empty initializeParameters.
    binaryWriter.Write(~bufferCountIncludeHeader_);
    binaryWriter.Write(parentStateProviderId_);
    Append(*binaryWriter.GetBuffer(0));

    ASSERT_IFNOT(
        initializationParametersCSPtr_ != nullptr,
        "{0}: InitializationParameters can not be nullptr.",
        TraceId);

    // Append each buffer in the initialization parameters array
    // Note: initializationParameters nullptr case should go to Managed serialize function.
    for (ULONG32 index = 0; index < initializationParametersCSPtr_->get_BufferCount(); index++)
    {
        Append(*((*initializationParametersCSPtr_)[index]));
    }

    return STATUS_SUCCESS;
}

// Note: The function helps to serialize Managed RedoOperationData
// and Native RedoOperationData with initializationParameters nullptr case.
NTSTATUS RedoOperationData::SerializeManagedOperation() noexcept
{
    Utilities::BinaryWriter binaryWriter(GetThisAllocator());
    RETURN_ON_FAILURE(binaryWriter.Status());

    binaryWriter.Write(*typeNameSPtr_);

    // In managed the initializationParameters has single buffer and when we
    // Serialize it, first write the size of the buffer then the actual buffer.
    // If the initializationParameters is null, write NULL_OPERATIONDATA(-1).
    LONG32 bufferSize;
    if (initializationParametersCSPtr_ == nullptr)
    {
        bufferSize = NULL_OPERATIONDATA;
    }
    else
    {
        bufferSize = initializationParametersCSPtr_->BufferCount == 0 ? 0 : (*initializationParametersCSPtr_)[0]->QuerySize();
    }

    ASSERT_IFNOT(
        bufferSize >= NULL_OPERATIONDATA,
        "{0}: BufferSize must be larger or equal to -1. BufferSize: {1}.",
        TraceId,
        bufferSize);

    binaryWriter.Write(bufferSize);
    if (bufferSize != NULL_OPERATIONDATA && bufferSize!= 0)
    {
        // This writer will just write the buffer instead of writing size and buffer.
        binaryWriter.Write((*initializationParametersCSPtr_)[0].RawPtr(), bufferSize);
        ASSERT_IFNOT(
            initializationParametersCSPtr_->BufferCount == 1,
            "{0}: Managed should only have 1 buffer for initialization parameters. BufferSize: {1}.",
            TraceId,
            initializationParametersCSPtr_->BufferCount);
    }

    binaryWriter.Write(parentStateProviderId_);
    Append(*binaryWriter.GetBuffer(0));

    return STATUS_SUCCESS;
}

KString::CSPtr RedoOperationData::get_TypeName() const
{
    return typeNameSPtr_;
}

Data::Utilities::OperationData::CSPtr RedoOperationData::get_InitParams() const
{
    return initializationParametersCSPtr_;
}

FABRIC_STATE_PROVIDER_ID RedoOperationData::get_ParentId() const
{
    return parentStateProviderId_;
}

RedoOperationData::RedoOperationData(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in LONG32 bufferCountIncludeHeader,
    __in KString const & typeName,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in SerializationMode::Enum serailizationMode,
    __in_opt OperationData const * const initializationParameters) noexcept
    : OperationData()
    , PartitionedReplicaTraceComponent(traceId)
    , bufferCountIncludeHeader_(bufferCountIncludeHeader)
    , typeNameSPtr_(&typeName)
    , parentStateProviderId_(parentStateProviderId)
    , initializationParametersCSPtr_(initializationParameters)
{
    NTSTATUS status = Status();
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    status = Serialize(serailizationMode);
    SetConstructorStatus(status);
}

RedoOperationData::RedoOperationData(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in LONG32 bufferCountIncludeHeader,
    __in KString const & typeName,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in_opt OperationData const * const initializationParameters) noexcept
    : OperationData()
    , PartitionedReplicaTraceComponent(traceId)
    , bufferCountIncludeHeader_(bufferCountIncludeHeader)
    , typeNameSPtr_(&typeName)
    , parentStateProviderId_(parentStateProviderId)
    , initializationParametersCSPtr_(initializationParameters)
{
    SetConstructorStatus(Status());
}

RedoOperationData::~RedoOperationData()
{
}
