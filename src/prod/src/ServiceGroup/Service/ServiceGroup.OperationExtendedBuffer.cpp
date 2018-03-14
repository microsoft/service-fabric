// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.OperationExtendedBuffer.h"

using namespace ServiceGroup;

HRESULT CExtendedOperationDataBuffer::Read(CExtendedOperationDataBuffer & extendedOperationDataBuffer, FABRIC_OPERATION_DATA_BUFFER const & buffer)
{
    ASSERT_IF(0 == buffer.BufferSize, "Unexpected buffer size");
    ASSERT_IF(NULL == buffer.Buffer, "Unexpected buffer");

    Common::ErrorCode error(Common::ErrorCodeValue::Success);

    try
    {
        error = Common::FabricSerializer::Deserialize(extendedOperationDataBuffer, buffer.BufferSize, buffer.Buffer);
    }
    catch (...)
    {
        Common::Assert::CodingError("Could not deserialize extended operation data.");
    }

    return error.ToHResult();
}

//
// Constructor/Destructor.
//
COutgoingOperationDataExtendedBuffer::COutgoingOperationDataExtendedBuffer(void)
    : extendedOperationData_()
    , innerOperationData_(NULL)
    , replicaBuffers_(NULL)
    , bufferCount_(0)
{
    WriteNoise(TraceSubComponentOutgoingOperationData, "this={0} - ctor", this);
}

COutgoingOperationDataExtendedBuffer::~COutgoingOperationDataExtendedBuffer(void)
{
    WriteNoise(TraceSubComponentOutgoingOperationData, "this={0} - dtor", this);

    delete[] replicaBuffers_;
    if (NULL != innerOperationData_)
    {
        innerOperationData_->Release();
        innerOperationData_ = NULL;
    }
}

//
// IFabricOperationData methods.
//
STDMETHODIMP COutgoingOperationDataExtendedBuffer::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER ** buffers)
{
    if (NULL == count || NULL == buffers)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ASSERT_IF(NULL == replicaBuffers_[0].Buffer, "Unexpected buffer");
    ASSERT_IF(0 == replicaBuffers_[0].BufferSize, "Unexpected buffer size");
    
    *count = bufferCount_;
    *buffers = replicaBuffers_;

    return S_OK;
}

//
// Initialization for this operation data.
//
STDMETHODIMP COutgoingOperationDataExtendedBuffer::FinalConstruct(
    __in FABRIC_OPERATION_TYPE operationType, 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in IFabricOperationData* operationData,
    __in const Common::Guid& partitionId,
    __in BOOLEAN emptyOperationIgnored
    )
{
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;
    HRESULT hr = S_OK;

    if (NULL != operationData)
    {
        hr = operationData->GetData(&bufferCount_, &replicaBuffers);
        if (FAILED(hr))
        {
            return hr;
        }
        operationData->AddRef();
        innerOperationData_ = operationData;
    }

    if (0 == bufferCount_ && !emptyOperationIgnored)
    {
        return E_INVALIDARG;
    }
    if (SUCCEEDED(hr))
    {
        replicaBuffers_ = new(std::nothrow) FABRIC_OPERATION_DATA_BUFFER[bufferCount_ + 1];
        if (NULL == replicaBuffers_)
        {
            return E_OUTOFMEMORY;
        }
        replicaBuffers_[0].Buffer = NULL;
        replicaBuffers_[0].BufferSize = 0;
        extendedOperationData_.OperationType = operationType;
        extendedOperationData_.AtomicGroupId = atomicGroupId;
        extendedOperationData_.PartitionId = partitionId.AsGUID();
        if (0 != bufferCount_)
        {
            ::CopyMemory(&replicaBuffers_[1], replicaBuffers, bufferCount_ * sizeof(FABRIC_OPERATION_DATA_BUFFER));
        }
        bufferCount_++;
    }

    return hr;
}

BOOLEAN COutgoingOperationDataExtendedBuffer::IsEmpty(void)
{
    return (1 == bufferCount_);
}

FABRIC_PARTITION_ID const & COutgoingOperationDataExtendedBuffer::GetPartitionId()
{
    return extendedOperationData_.PartitionId;
}

FABRIC_ATOMIC_GROUP_ID COutgoingOperationDataExtendedBuffer::GetAtomicGroupId()
{
    return extendedOperationData_.AtomicGroupId;
}

FABRIC_OPERATION_TYPE COutgoingOperationDataExtendedBuffer::GetOperationType()
{
    return extendedOperationData_.OperationType;
}

void COutgoingOperationDataExtendedBuffer::SetOperationType(FABRIC_OPERATION_TYPE operationType)
{
    extendedOperationData_.OperationType = operationType;
}

HRESULT COutgoingOperationDataExtendedBuffer::SerializeExtendedOperationDataBuffer()
{
    ASSERT_IF(NULL == replicaBuffers_, "Unexpected buffers");
    
    ASSERT_IFNOT(NULL == replicaBuffers_[0].Buffer, "Unexpected buffer");
    ASSERT_IFNOT(0 == replicaBuffers_[0].BufferSize, "Unexpected buffer size");

    try
    {
        Common::ErrorCode error = Common::FabricSerializer::Serialize(&extendedOperationData_, extendedOperationDataBuffer_);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        replicaBuffers_[0].Buffer = extendedOperationDataBuffer_.data();
        replicaBuffers_[0].BufferSize = (ULONG)extendedOperationDataBuffer_.size();
    }
    catch (std::bad_alloc const &)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_FAIL;
    }

    return S_OK;
}

//
// Constructor/Destructor.
//
CIncomingOperationExtendedBuffer::CIncomingOperationExtendedBuffer(void)
{
    WriteNoise(TraceSubComponentIncomingOperation, "this={0} - ctor", this);

    innerOperation_ = NULL;
    operationMetadata_.Type = FABRIC_OPERATION_TYPE_NORMAL;
    operationMetadata_.SequenceNumber = 1;
    operationMetadata_.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
    operationMetadata_.Reserved = NULL;
}

CIncomingOperationExtendedBuffer::~CIncomingOperationExtendedBuffer(void)
{
    WriteNoise(TraceSubComponentIncomingOperation, "this={0} - dtor", this);

    if (NULL != innerOperation_)
    {
        innerOperation_->Release();
    }
}

//
// IFabricOperation methods.
//
const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE CIncomingOperationExtendedBuffer::get_Metadata(void)
{
    return &operationMetadata_;
}

STDMETHODIMP CIncomingOperationExtendedBuffer::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    if (NULL == count || NULL == buffers)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (NULL == innerOperation_)
    {
        *count = 0;
        *buffers = NULL;
    }
    else
    {
        HRESULT hr = innerOperation_->GetData(count, buffers);
        if (FAILED(hr))
        {
            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        ASSERT_IFNOT(*count >= 1, "Incoming extended buffer must have at lease 1 replica buffer");

        --(*count);
        if (0 == *count)
        {
            *buffers = NULL;
        }
        else
        {
            ++(*buffers);
        }
    }

    return S_OK;
}

STDMETHODIMP CIncomingOperationExtendedBuffer::Acknowledge(void)
{
    if (NULL != innerOperation_)
    {
        WriteNoise(TraceSubComponentIncomingOperation, "this={0} - Acknowledge", this);

        return Common::ComUtility::OnPublicApiReturn(innerOperation_->Acknowledge());
    }
    return S_OK;
}

//
// Initialization/Cleanup for this operation.
//
STDMETHODIMP CIncomingOperationExtendedBuffer::FinalConstruct(__in FABRIC_OPERATION_TYPE operationType, __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in IFabricOperation* operation)
{
    ASSERT_IF(NULL == operation, "Unexpected operation");

    const FABRIC_OPERATION_METADATA* operationMetadata = operation->get_Metadata();
    ASSERT_IF(NULL == operationMetadata, "Unexpected operation metadata");
    
    operationMetadata_.Type = operationType;
    operationMetadata_.SequenceNumber = operationMetadata->SequenceNumber;
    operationMetadata_.AtomicGroupId = atomicGroupId;
    operationMetadata_.Reserved = NULL;

    operation->AddRef();
    innerOperation_ = operation;

    return S_OK;
}
