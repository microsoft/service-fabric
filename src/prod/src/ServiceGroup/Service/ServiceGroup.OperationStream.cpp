// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.OperationStream.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
COperationStream::COperationStream(__in const Common::Guid& partitionId, __in BOOLEAN isCopyStream) : partitionId_(partitionId)
{
    WriteNoise(
        isCopyStream ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
        "this={0} partitionId={1} - ctor", 
        this, 
        partitionId_
        );

    lastOperationSeen_ = FALSE;
    lastOperationDispatched_ = FALSE;
    operationUpdateEpoch_ = NULL;
    errorCode_ = S_OK;
    isCopyStream_ = isCopyStream;
}

COperationStream::~COperationStream(void)
{
    WriteNoise(
        isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
        "this={0} partitionId={1} - dtor", 
        this, 
        partitionId_
        );

    for (std::list<IFabricOperation*>::iterator iterateOperations = operationQueue_.begin(); operationQueue_.end() != iterateOperations; iterateOperations++)
    {
        (*iterateOperations)->Release();
    }
    if (NULL != operationUpdateEpoch_)
    {
        operationUpdateEpoch_->Release();
    }
}

//
// IFabricOperationStream methods.
//
STDMETHODIMP COperationStream::BeginGetOperation(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext ** context)
{
    HRESULT hr = E_FAIL;
    IFabricOperation* operationCompleted = NULL;
    std::list<IFabricOperation*>::iterator iterateOperationCompleted;

    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    CGetOperationAsyncOperationContext* asyncContext = new (std::nothrow) CGetOperationAsyncOperationContext();
    if (NULL == asyncContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            GetStreamName()
            );

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    hr = asyncContext->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(asyncContext),
            hr,
            GetStreamName()
            );

        asyncContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }
    asyncContext->set_Callback(callback);

    //
    // Acquire lock explicitly. This lock needs to be realeased on all return code paths.
    //
    operationLock_.AcquireExclusive();

    if (lastOperationDispatched_)
    {
        //
        // Release lock explicitly.
        //
        operationLock_.ReleaseExclusive();

        ServiceGroupReplicationEventSource::GetEvents().Info_0_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            GetStreamName()
            );

        asyncContext->set_CompletedSynchronously(TRUE);
        asyncContext->set_IsCompleted();
        if (NULL != callback)
        {
            callback->Invoke(asyncContext);
        }
        *context = asyncContext;
        return S_OK;
    }

    if (!operationQueue_.empty())
    {
        ASSERT_IF(!operationWaiters_.empty(), "Unexpected contexts");

        //
        // Operation is available for completion.
        //
        iterateOperationCompleted = operationQueue_.begin();
        operationCompleted = *iterateOperationCompleted;
        operationQueue_.erase(iterateOperationCompleted);

        //
        // Release lock explicitly.
        //
        operationLock_.ReleaseExclusive();

        WriteNoise(
            isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} synchronously with operation {3} in {4}",
            this,
            partitionId_,
            asyncContext,
            operationCompleted,
            GetStreamName()
            );

        asyncContext->set_Operation(S_OK, operationCompleted);
        asyncContext->set_CompletedSynchronously(TRUE);
        asyncContext->set_IsCompleted();
        if (NULL != callback)
        {
            callback->Invoke(asyncContext);
        }
        operationCompleted->Release();
        *context = asyncContext;
        return S_OK;
    }

    if (NULL != operationUpdateEpoch_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_1_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(operationUpdateEpoch_),
            GetStreamName()
            );

        operationUpdateEpoch_->Acknowledge();
        operationUpdateEpoch_->Release();
        operationUpdateEpoch_ = NULL;
    }

    if (lastOperationSeen_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_2_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            GetStreamName()
            );
        
        lastOperationDispatched_ = TRUE;
        waitLastOperationDispatched_.Set();

        //
        // Release lock explicitly.
        //
        operationLock_.ReleaseExclusive();

        WriteNoise(
            isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} synchronously with operation {3} in {4}",
            this,
            partitionId_,
            asyncContext,
            operationCompleted,
            GetStreamName()
            );

        asyncContext->set_CompletedSynchronously(TRUE);
        asyncContext->set_IsCompleted();
        if (NULL != callback)
        {
            callback->Invoke(asyncContext);
        }
        *context = asyncContext;
        return S_OK;
    }

    try 
    {
        //
        // Waiter is queued up.
        //
        operationWaiters_.insert(operationWaiters_.end(), asyncContext); 
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        //
        // Release lock explicitly.
        //
        operationLock_.ReleaseExclusive();

        ServiceGroupReplicationEventSource::GetEvents().Error_2_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(asyncContext),
            GetStreamName()
            );
    
        asyncContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    //
    // Waiter is set up to be completed asynchronously.
    //
    asyncContext->AddRef();
    asyncContext->set_CompletedSynchronously(FALSE);
    *context = asyncContext;

    //
    // Release lock explicitly.
    //
    operationLock_.ReleaseExclusive();

    WriteNoise(
        isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
        "this={0} partitionId={1} - Operation context {2} enqueued in {3}",
        this,
        partitionId_,
        asyncContext,
        GetStreamName()
        );

    return S_OK;
}

STDMETHODIMP COperationStream::EndGetOperation(__in IFabricAsyncOperationContext* context, __out IFabricOperation** operation)
{
    if (NULL == context || NULL == operation)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    CGetOperationAsyncOperationContext* asyncContext = (CGetOperationAsyncOperationContext*)context;

    WriteNoise(
        isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
        "this={0} partitionId={1} - Waiting for operation context {2} to complete ({3})",
        this,
        partitionId_,
        asyncContext,
        GetStreamName()
        );

    asyncContext->Wait(INFINITE);

    HRESULT hr = asyncContext->get_Operation(operation);

    WriteNoise(
        isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
        "this={0} partitionId={1} - Retrieved operation {2} from operation context {3} with error code {4} ({5})",
        this,
        partitionId_,
        (*operation),
        asyncContext,
        hr,
        GetStreamName()
        );

    asyncContext->FinalDestruct();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

HRESULT COperationStream::ReportFault(__in FABRIC_FAULT_TYPE faultType)
{
    UNREFERENCED_PARAMETER(faultType);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

//
// Initialization/Cleanup.
//
HRESULT COperationStream::FinalConstruct(void)
{
    return S_OK;
}

//
// Additional methods.
//
HRESULT COperationStream::EnqueueOperation(__in_opt IFabricOperation* operation)
{
    CGetOperationAsyncOperationContext* contextCompleted = NULL;
    IFabricOperation* operationCompleted = NULL;
    HRESULT hr = S_OK;
    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;
    const FABRIC_OPERATION_METADATA* operationMetadata = NULL;
    std::list<CGetOperationAsyncOperationContext*>::iterator iterateWaiterCompleted;
    std::list<CGetOperationAsyncOperationContext*> waitersCompleted;

    if (NULL != operation)
    {
        hr = operation->GetData(&bufferCount, &replicaBuffers);
        ASSERT_IF(FAILED(hr), "GetData");
        operationMetadata = operation->get_Metadata();
        ASSERT_IF(NULL == operationMetadata, "Unexpected operation metadata");
    }

    {
        Common::AcquireWriteLock lockOperationQueue(operationLock_);

        if (lastOperationSeen_)
        {
            if (FAILED(errorCode_))
            {
                if (NULL != operation)
                {
                    ServiceGroupReplicationEventSource::GetEvents().Warning_0_OperationStream_StatefulServiceMember(
                        isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        reinterpret_cast<uintptr_t>(operation),
                        GetStreamName()
                        );

                    return errorCode_;
                }
                return S_OK;
            }
            ASSERT_IFNOT(NULL == operation, "Last operation has been already seen");
            return S_OK;
        }

        if (NULL == operationMetadata)
        {
            ASSERT_IFNOT(NULL == operation, "Unexpected operation");
            lastOperationSeen_ = TRUE;
        }
        else
        {
            ASSERT_IF(NULL == operation, "Unexpected operation");
        }
        
        if (operationWaiters_.empty())
        {
            if (NULL != operation)
            {
                try 
                {
                    operationQueue_.insert(operationQueue_.end(), operation); 

                    WriteNoise(
                        isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                        "this={0} partitionId={1} - Operation {2} enqueued in {3} of queue size {4}",
                        this,
                        partitionId_,
                        operation,
                        GetStreamName(),
                        operationQueue_.size()
                        );
                }
                catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
                catch (...) { hr = E_FAIL; }
                if (FAILED(hr))
                {
                    ServiceGroupReplicationEventSource::GetEvents().Error_3_OperationStream_StatefulServiceMember(
                        isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        reinterpret_cast<uintptr_t>(operation),
                        GetStreamName()
                        );
        
                    return hr;
                }
            }
        }
        else
        {
            ASSERT_IFNOT(operationQueue_.empty(), "Unexpected operation queue");

            operationCompleted = operation;
            if (lastOperationSeen_)
            {
                ASSERT_IF(TRUE == lastOperationDispatched_, "Last operation should not have been dispatched");

                lastOperationDispatched_ = TRUE;
                waitersCompleted = std::move(operationWaiters_);

                if (NULL != operationUpdateEpoch_)
                {
                    ServiceGroupReplicationEventSource::GetEvents().Info_3_OperationStream_StatefulServiceMember(
                        isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        reinterpret_cast<uintptr_t>(operationUpdateEpoch_),
                        GetStreamName()
                        );
                    
                    operationUpdateEpoch_->Acknowledge();
                    operationUpdateEpoch_->Release();
                    operationUpdateEpoch_ = NULL;
                }
            }
            else
            {
                iterateWaiterCompleted = operationWaiters_.begin();
                contextCompleted = *iterateWaiterCompleted;
                operationWaiters_.erase(iterateWaiterCompleted);

                WriteNoise(
                    isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                    "this={0} partitionId={1} - Operation context {2} dequeued from {3}",
                    this,
                    partitionId_,
                    contextCompleted,
                    GetStreamName()
                    );
            }
        }
    }
    if (!waitersCompleted.empty())
    {
        ASSERT_IF(NULL != contextCompleted, "Unexpected waiter completed");

        for (iterateWaiterCompleted = waitersCompleted.begin(); waitersCompleted.end() != iterateWaiterCompleted; iterateWaiterCompleted++)
        {
            if (NULL == operationCompleted)
            {
                WriteNoise(
                    isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                    "this={0} partitionId={1} - Completing operation context {2} asynchronously with last operation (NULL) ({3})",
                    this,
                    partitionId_,
                    (*iterateWaiterCompleted),
                    GetStreamName()
                    );
            }
            else
            {
                operationCompleted->AddRef();

                WriteNoise(
                    isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                    "this={0} partitionId={1} - Completing operation context {2} asynchronously with last operation (empty) ({3})",
                    this,
                    partitionId_,
                    (*iterateWaiterCompleted),
                    GetStreamName()
                    );
            }

            COperationStream::CompleteWaiterOnThreadPool(*iterateWaiterCompleted, operationCompleted);
        }
        waitersCompleted.clear();

        if (NULL == operationCompleted)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_4_OperationStream_StatefulServiceMember(
                isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                GetStreamName()
                );
        }
        else
        {
            WriteNoise(
                isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                "this={0} partitionId={1} - Signal that the last operation (empty) has been dispatched in {2}",
                this,
                partitionId_,
                GetStreamName()
                );

            operationCompleted->Release();
        }

        waitLastOperationDispatched_.Set();
    }

    if (NULL != contextCompleted)
    {
        ASSERT_IF(!waitersCompleted.empty(), "Unexpected waiters completed");
        ASSERT_IF(NULL == operationCompleted, "Unexpected operation completed");

        WriteNoise(
            isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} asynchronously with operation {3} ({4})",
            this,
            partitionId_,
            contextCompleted,
            operationCompleted,
            GetStreamName()
            );
        
        COperationStream::CompleteWaiterOnThreadPool(contextCompleted, operationCompleted);
    }
    return S_OK;
}

void COperationStream::CompleteWaiter(__in CGetOperationAsyncOperationContext* contextCompleted, __in IFabricOperation* operationCompleted)
{
    IFabricAsyncOperationCallback* callbackCompleted = NULL;
    contextCompleted->set_Operation(S_OK, operationCompleted);
    contextCompleted->set_IsCompleted();
    contextCompleted->get_Callback(&callbackCompleted);
    if (NULL != callbackCompleted)
    {
        callbackCompleted->Invoke(contextCompleted);
        callbackCompleted->Release();
    }
    contextCompleted->Release();
    if (NULL != operationCompleted)
    {
        operationCompleted->Release();
    }
}

void COperationStream::CompleteWaiterOnThreadPool(__in CGetOperationAsyncOperationContext* contextCompleted, __in IFabricOperation* operationCompleted)
{
    try
    {
        Common::Threadpool::Post
        (
            [contextCompleted, operationCompleted]
            {
                COperationStream::CompleteWaiter(contextCompleted, operationCompleted); 
            }
        );
    }
    catch (std::bad_alloc const &)
    {
        Common::Assert::CodingError(SERVICE_REPLICA_DOWN);
    }
}

HRESULT COperationStream::ForceDrain(__in HRESULT errorCode, __in BOOLEAN waitForNullDispatch)
{
    std::list<CGetOperationAsyncOperationContext*> waitersCompleted;

    {
        Common::AcquireWriteLock lockOperationQueue(operationLock_);
        if (FAILED(errorCode_))
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_5_OperationStream_StatefulServiceMember(
                isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                GetStreamName(),
                errorCode_
                );
        }
        for (std::list<IFabricOperation*>::iterator iterateOperation = operationQueue_.begin();
               operationQueue_.end() != iterateOperation;
               iterateOperation++)
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_1_OperationStream_StatefulServiceMember(
                isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                reinterpret_cast<uintptr_t>((*iterateOperation)),
                GetStreamName()
                );

            (*iterateOperation)->Release();
        }
        operationQueue_.clear();
        waitersCompleted = std::move(operationWaiters_);
        lastOperationSeen_ = TRUE;
        errorCode_ = errorCode;
        if (!waitForNullDispatch || !waitersCompleted.empty())
        {
            //
            // If waitersCompleted is not empty, the last operation (NULL) will be dispatched to these waiters.
            // Otherwise, the next GetNextOperation will dispatch NULL and set the event.
            //
            lastOperationDispatched_ = TRUE;
            waitLastOperationDispatched_.Set();
        }
        if (NULL != operationUpdateEpoch_)
        {
            operationUpdateEpoch_->Cancel();
            operationUpdateEpoch_->Release();
            operationUpdateEpoch_ = NULL;
        }
    }

    for (std::list<CGetOperationAsyncOperationContext*>::iterator iterateWaiterCompleted = waitersCompleted.begin(); 
           waitersCompleted.end() != iterateWaiterCompleted; 
           iterateWaiterCompleted++)
    {
        WriteNoise(
            isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} with last (NULL) operation and error code {3} ({4})",
            this,
            partitionId_,
            *iterateWaiterCompleted,
            errorCode,
            GetStreamName()
            );

        COperationStream::CompleteWaiterOnThreadPool(*iterateWaiterCompleted, NULL);
    }
    return S_OK;
}

HRESULT COperationStream::Drain(void)
{
    BOOLEAN lastOperationDispatched = FALSE;
    size_t count = 0;
    {
        Common::AcquireExclusiveLock lockOperationQueue(operationLock_);
        count = operationQueue_.size();

        //
        // Only create waithandle if waiting is necessary.
        //
        lastOperationDispatched = lastOperationDispatched_;
        if (!lastOperationDispatched)
        {
            WriteNoise(
                isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                "this={0} partitionId={1} - Draining {2}. Waiting for last operation to be dispatched.",
                this,
                partitionId_,
                GetStreamName()
                );
        }
        else
        {
            WriteNoise(
                isCopyStream_ ? TraceSubComponentStatefulServiceMemberCopyProcessing : TraceSubComponentStatefulServiceMemberReplicationProcessing, 
                "this={0} partitionId={1} - Draining {2}. Last operation has already been dispatched.",
                this,
                partitionId_,
                GetStreamName()
                );
        }
    }

    if (0 != count)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_6_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            GetStreamName(),
            count
            );
    }

    HRESULT errorCode = S_OK;
    if (!lastOperationDispatched)
    {
        waitLastOperationDispatched_.WaitOne();
        waitLastOperationDispatched_.Reset();
        {
            Common::AcquireExclusiveLock lockOperationQueue(operationLock_);

            errorCode = errorCode_;
        }
    }
    else
    {
        Common::AcquireReadLock lockOperationQueue(operationLock_);
        errorCode = errorCode_;
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_7_OperationStream_StatefulServiceMember(
        isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        GetStreamName()
        );

    return errorCode;
}

HRESULT COperationStream::EnqueueUpdateEpochOperation(__in IOperationBarrier* operation)
{
    ASSERT_IF(NULL == operation, "Unexpected update epoch operation");
    ASSERT_IF(NULL != operationUpdateEpoch_, "Unexpected existent update epoch operation");
    ASSERT_IF(TRUE == isCopyStream_, "Unexpected update epoch operation in copy stream");

    ServiceGroupReplicationEventSource::GetEvents().Info_8_OperationStream_StatefulServiceMember(
        isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    Common::AcquireWriteLock lockOperationQueue(operationLock_);

    if (FAILED(errorCode_))
    {
        ServiceGroupReplicationEventSource::GetEvents().Warning_2_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(operation)
            );

        return errorCode_;
    }
    if (lastOperationDispatched_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_9_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(operation)
            );

        operation->Acknowledge();
        return S_OK;
    }
    if (operationQueue_.empty())
    {
        if (operationWaiters_.empty())
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_10_OperationStream_StatefulServiceMember(
                isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                reinterpret_cast<uintptr_t>(operation)
                );
    
            operation->AddRef();
            operationUpdateEpoch_ = operation;
        }
        else
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_11_OperationStream_StatefulServiceMember(
                isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                reinterpret_cast<uintptr_t>(operation)
                );
        
            operation->Acknowledge();
        }
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_12_OperationStream_StatefulServiceMember(
            isCopyStream_ ? TraceCopyProcessingTask : TraceReplicationProcessingTask,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(operation)
            );

        operation->AddRef();
        operationUpdateEpoch_ = operation;
    }
    return S_OK;
}


