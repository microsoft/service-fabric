// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.AtomicGroup.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
CReplicationAtomicGroupOperationCallback::CReplicationAtomicGroupOperationCallback(void)
{
    WriteNoise(TraceSubComponentReplicationAtomicGroupOperationCallback, "this={0} - ctor", this);

    externalCallback_ = NULL;
    internalContext_ = NULL;
}

CReplicationAtomicGroupOperationCallback::~CReplicationAtomicGroupOperationCallback(void)
{
    WriteNoise(TraceSubComponentReplicationAtomicGroupOperationCallback, "this={0} - dtor", this);

    if (NULL != externalCallback_)
    {
        externalCallback_->Release();
    }
    if (NULL != internalContext_)
    {
        internalContext_->Release();
    }
}

//
// IFabricAsyncOperationCallback methods.
//
void STDMETHODCALLTYPE CReplicationAtomicGroupOperationCallback::Invoke(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    ASSERT_IF(NULL == externalCallback_, "Unexpected external callback");
    
    if (!context->CompletedSynchronously())
    {
        WriteNoise(TraceSubComponentReplicationAtomicGroupOperationCallback, "this={0} - System context {1} completed asynchronously", this, context);

        ((CAtomicGroupAsyncOperationContext*)internalContext_)->set_SystemContext(context);
        if (NULL != externalCallback_)
        {
            externalCallback_->Invoke(internalContext_);
            
            WriteNoise(
                TraceSubComponentReplicationAtomicGroupOperationCallback, 
                "this={0} - Service callback completed",
                this
                );
            externalCallback_->Release();
            externalCallback_ = NULL;
        }
        internalContext_->Release();
        internalContext_ = NULL;
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_0_ReplicationAtomicGroupOperationCallback(
            reinterpret_cast<uintptr_t>(this),
            reinterpret_cast<uintptr_t>(context)
            );
    }
}

//
// Initialization/Cleanup for this callback.
//
STDMETHODIMP CReplicationAtomicGroupOperationCallback::FinalConstruct(__in IFabricAsyncOperationCallback* externalCallback, __in IFabricAsyncOperationContext* internalContext)
{
    ASSERT_IF(NULL == internalContext, "Unexpected internal context");

    if (NULL != externalCallback)
    {
        externalCallback->AddRef();
        externalCallback_ = externalCallback;

    }
    internalContext->AddRef();
    internalContext_ = internalContext;
    return S_OK;
}

//
// Constructor/Destructor.
//
CAtomicGroupAsyncOperationContext::CAtomicGroupAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - ctor", this);

    systemContext_ = NULL;
    sequenceNumber_ = -1;
    atomicGroupId_ = FABRIC_INVALID_ATOMIC_GROUP_ID;
    operationType_ = FABRIC_OPERATION_TYPE_INVALID;
}

CAtomicGroupAsyncOperationContext::~CAtomicGroupAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - dtor", this);

    if (NULL != systemContext_)
    {
        systemContext_->Release();
    }
}

//
// IFabricAsyncOperationContext methods.
//
BOOLEAN STDMETHODCALLTYPE CAtomicGroupAsyncOperationContext::IsCompleted()
{
    ASSERT_IF(NULL == systemContext_, "Unexpected inner context");
    return systemContext_->IsCompleted();
}

BOOLEAN STDMETHODCALLTYPE CAtomicGroupAsyncOperationContext::CompletedSynchronously()
{
    ASSERT_IF(NULL == systemContext_, "Unexpected inner context");
    BOOLEAN completedSynchronously = systemContext_->CompletedSynchronously();

    WriteNoise(TraceSubComponentAtomicGroupAsyncOperationContext, "this={0} - System context {1} completed {2}", this, systemContext_, completedSynchronously);

    return completedSynchronously;
}

STDMETHODIMP CAtomicGroupAsyncOperationContext::get_Callback(__out IFabricAsyncOperationCallback** state)
{
    ASSERT_IF(NULL == systemContext_, "Unexpected inner context");
    return systemContext_->get_Callback(state);
}

STDMETHODIMP CAtomicGroupAsyncOperationContext::Cancel(void)
{
    ServiceGroupReplicationEventSource::GetEvents().Info_0_AtomicGroupAsyncOperationContext(reinterpret_cast<uintptr_t>(this));

    return Common::ComUtility::OnPublicApiReturn(S_FALSE);
}

//
// Initialization/Cleanup for this callback.
//
STDMETHODIMP CAtomicGroupAsyncOperationContext::FinalConstruct(__in FABRIC_OPERATION_TYPE operationType, __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in const Common::Guid& participant)
{
    operationType_ = operationType;
    atomicGroupId_ = atomicGroupId;
    participant_ = participant;
    return S_OK;
}

//
// Constructor/Destructor.
//
CAtomicGroupCommitRollback::CAtomicGroupCommitRollback(__in BOOLEAN isCommit)
{
    WriteNoise(TraceSubComponentAtomicGroupCommitRollbackOperation, "this={0} - ctor", this);

    innerOperation_ = NULL;
    count_ = 0;
    dataBuffer_.Buffer = NULL;
    dataBuffer_.BufferSize = 0;
    isCommit_ = isCommit;
}

CAtomicGroupCommitRollback::~CAtomicGroupCommitRollback(void)
{
    WriteNoise(TraceSubComponentAtomicGroupCommitRollbackOperation, "this={0} - dtor", this);

    if (NULL != innerOperation_)
    {
        innerOperation_->Release();
    }
}

//
// IFabricOperation methods.
//
const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE CAtomicGroupCommitRollback::get_Metadata(void)
{
    return &operationMetadata_;
}

STDMETHODIMP CAtomicGroupCommitRollback::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    ASSERT_IF(NULL == innerOperation_, "Unexpected inner operation");

    if (NULL == count || NULL == buffers)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    *count = 1;
    *buffers = &dataBuffer_;

    return S_OK;
}

STDMETHODIMP CAtomicGroupCommitRollback::Acknowledge(void)
{
    if (0 == ::InterlockedDecrement(&count_))
    {
        ASSERT_IF(NULL == innerOperation_, "Unexpected inner operation");

        WriteNoise(
            TraceSubComponentAtomicGroupCommitRollbackOperation, 
            "this={0} - Acknowledge atomic group {1}", 
            this, 
            operationMetadata_.AtomicGroupId
            );

        innerOperation_->Acknowledge();
        innerOperation_->Release();
        innerOperation_ = NULL;

        ServiceGroupReplicationEventSource::GetEvents().Info_0_AtomicGroupCommitRollbackOperation(
            reinterpret_cast<uintptr_t>(this),
            operationMetadata_.AtomicGroupId,
            isCommit_ ? L"committed" : L"rolled back"
            );

        return Common::ComUtility::OnPublicApiReturn(S_OK);
    }
    return S_OK;
}

//
// Initialization/Cleanup for this operation.
//
STDMETHODIMP CAtomicGroupCommitRollback::FinalConstruct(
    __in size_t count,
    __in FABRIC_OPERATION_TYPE atomicOperationType, 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in IFabricOperation* operation)
{
    ASSERT_IF(NULL == operation, "Unexpected inner operation");
    ASSERT_IFNOT(
        FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == atomicOperationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == atomicOperationType, 
        "Unexpected operation type");

    HRESULT hr = ::SizeTToLong(count, &count_);
    if (FAILED(hr))
    {
        return hr;
    }
        
    const FABRIC_OPERATION_METADATA* operationMetadata = operation->get_Metadata();
    ASSERT_IF(NULL == operationMetadata, "Unexpected operation metadata");

    operationMetadata_.Type = atomicOperationType;
    operationMetadata_.SequenceNumber = operationMetadata->SequenceNumber;
    operationMetadata_.AtomicGroupId = atomicGroupId;
    operationMetadata_.Reserved = NULL;

    operation->AddRef();
    innerOperation_ = operation;

    return S_OK;
}

//
// Constructor/Destructor.
//
CUpdateEpochOperation::CUpdateEpochOperation(void)
{
    WriteNoise(TraceSubComponentUpdateEpochOperation, "this={0} - ctor", this);

    errorCode_ = S_OK;
    operationMetadata_.Type = FABRIC_OPERATION_TYPE_INVALID;
    operationMetadata_.SequenceNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
    operationMetadata_.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
    operationMetadata_.Reserved = NULL;
}

CUpdateEpochOperation::~CUpdateEpochOperation(void)
{
    WriteNoise(TraceSubComponentUpdateEpochOperation, "this={0} - dtor", this);
}

//
// IFabricOperation methods.
//
const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE CUpdateEpochOperation::get_Metadata(void)
{
    return &operationMetadata_;
}

STDMETHODIMP CUpdateEpochOperation::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(buffers);

    Common::Assert::CodingError("Unexpected call");
}

STDMETHODIMP CUpdateEpochOperation::Acknowledge(void)
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_UpdateEpochOperation(reinterpret_cast<uintptr_t>(this));
    
    Common::AcquireWriteLock lockErrorCode(lock_);
    return Common::ComUtility::OnPublicApiReturn(handle_.Set().ToHResult());
}

//
// Initialization/Cleanup for this operation.
//
STDMETHODIMP CUpdateEpochOperation::FinalConstruct(void)
{
    return S_OK;
}

//
// IOperationBarrier methods.
//
STDMETHODIMP CUpdateEpochOperation::Barrier(void)
{
    auto waitResult = handle_.WaitOne();
    ASSERT_IFNOT(waitResult, "Unexpected barrier"); 

    Common::AcquireReadLock lockErrorCode(lock_);
    return errorCode_;
}

STDMETHODIMP CUpdateEpochOperation::Cancel(void)
{
    ServiceGroupStatefulEventSource::GetEvents().Error_1_UpdateEpochOperation(reinterpret_cast<uintptr_t>(this));

    Common::AcquireWriteLock lockErrorCode(lock_);
    errorCode_ = E_ABORT;
    return Common::ComUtility::OnPublicApiReturn(handle_.Set().ToHResult());
}

