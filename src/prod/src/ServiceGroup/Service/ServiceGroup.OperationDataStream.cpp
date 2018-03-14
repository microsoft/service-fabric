// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.OperationDataStream.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
COperationDataStream::COperationDataStream(const Common::Guid& partitionId) : partitionId_(partitionId)
{
    WriteNoise(TraceSubComponentStatefulServiceMemberCopyContextProcessing, "this={0} partitionId={1} - ctor", this, partitionId_);

    lastOperationDataSeen_ = FALSE;
    lastOperationDataDispatched_ = FALSE;
    errorCode_ = S_OK;
}

COperationDataStream::~COperationDataStream(void)
{
    WriteNoise(TraceSubComponentStatefulServiceMemberCopyContextProcessing, "this={0} partitionId={1} - dtor", this, partitionId_);

    for (std::list<IFabricOperationData*>::iterator iterateOperations = operationDataQueue_.begin(); operationDataQueue_.end() != iterateOperations; iterateOperations++)
    {
        (*iterateOperations)->Release();
    }
}

//
// IFabricOperationDataStream methods.
//
STDMETHODIMP COperationDataStream::BeginGetNext(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext ** context)
{
    HRESULT hr = E_FAIL;
    IFabricOperationData* operationCompleted = NULL;
    std::list<IFabricOperationData*>::iterator iterateOperationCompleted;

    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    CGetOperationDataAsyncOperationContext* asyncContext = new (std::nothrow) CGetOperationDataAsyncOperationContext();
    if (NULL == asyncContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    hr = asyncContext->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(asyncContext),
            hr
            );

        asyncContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }
    asyncContext->set_Callback(callback);

    //
    // Acquire lock explicitly. This lock needs to be realeased on all return code paths.
    //
    operationDataLock_.AcquireExclusive();

    if (FAILED(errorCode_))
    {
        //
        // Release lock explicitly.
        //
        operationDataLock_.ReleaseExclusive();

        ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            errorCode_
            );

        asyncContext->set_OperationData(S_OK, NULL);
        asyncContext->set_CompletedSynchronously(TRUE);
        asyncContext->set_IsCompleted();
        if (NULL != callback)
        {
            callback->Invoke(asyncContext);
        }
        *context = asyncContext;
        return S_OK;
    }

    if (lastOperationDataDispatched_)
    {
        //
        // Release lock explicitly.
        //
        operationDataLock_.ReleaseExclusive();
        
        ServiceGroupReplicationEventSource::GetEvents().Warning_1_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
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

    if (!operationDataQueue_.empty())
    {
        ASSERT_IF(!operationDataWaiters_.empty(), "Unexpected contexts");

        //
        // Operation is available for completion.
        //
        iterateOperationCompleted = operationDataQueue_.begin();
        operationCompleted = *iterateOperationCompleted;
        operationDataQueue_.erase(iterateOperationCompleted);
    
        //
        // Release lock explicitly.
        //
        operationDataLock_.ReleaseExclusive();
    
        WriteNoise(
            TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} synchronously with operation {3}",
            this,
            partitionId_,
            asyncContext,
            operationCompleted
            );

        asyncContext->set_OperationData(S_OK, operationCompleted);
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

    if (lastOperationDataSeen_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );
        
        lastOperationDataDispatched_ = TRUE;
        waitLastOperationDataDispatched_.Set();

        //
        // Release lock explicitly.
        //
        operationDataLock_.ReleaseExclusive();

        WriteNoise(
            TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} synchronously with operation {3}",
            this,
            partitionId_,
            asyncContext,
            operationCompleted
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
        operationDataWaiters_.insert(operationDataWaiters_.end(), asyncContext); 
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        //
        // Release lock explicitly.
        //
        operationDataLock_.ReleaseExclusive();
        
        ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceMemberCopyContextProcessing(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            reinterpret_cast<uintptr_t>(asyncContext)
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
    operationDataLock_.ReleaseExclusive();
    
    WriteNoise(
        TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
        "this={0} partitionId={1} - Operation context {2} enqueued",
        this,
        partitionId_,
        asyncContext
        );

    return S_OK;
}

STDMETHODIMP COperationDataStream::EndGetNext(__in IFabricAsyncOperationContext* context, __out IFabricOperationData** operationData)
{
    if (NULL == context || NULL == operationData)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    CGetOperationDataAsyncOperationContext* asyncContext = (CGetOperationDataAsyncOperationContext*)context;

    WriteNoise(
        TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
        "this={0} partitionId={1} - Waiting for operationData context {2} to complete",
        this,
        partitionId_,
        asyncContext
        );

    asyncContext->Wait(INFINITE);

    HRESULT hr = asyncContext->get_OperationData(operationData);

    WriteNoise(
        TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
        "this={0} partitionId={1} - Retrieved operationData {2} from operation context {3} with error code {4}",
        this,
        partitionId_,
        (*operationData),
        asyncContext,
        hr
        );

    asyncContext->FinalDestruct();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// Initialization/Cleanup.
//
HRESULT COperationDataStream::FinalConstruct(void)
{
    return S_OK;
}

//
// Additional methods.
//
HRESULT COperationDataStream::EnqueueOperationData(
    __in HRESULT errorCode,
    __in_opt IFabricOperationData* operationData)
{
    CGetOperationDataAsyncOperationContext* contextCompleted = NULL;
    IFabricOperationData* operationDataCompleted = NULL;
    HRESULT hr = S_OK;
    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;
    std::list<CGetOperationDataAsyncOperationContext*>::iterator iterateWaiterCompleted;
    std::list<CGetOperationDataAsyncOperationContext*> waitersCompleted;

    ASSERT_IFNOT(SUCCEEDED(errorCode) || NULL == operationData, "errorCode and operationData do not match");
    if (NULL != operationData)
    {
        hr = operationData->GetData(&bufferCount, &replicaBuffers);
        ASSERT_IF(FAILED(hr), "GetData");
    }

    {
        Common::AcquireWriteLock lockOperationQueue(operationDataLock_);

        if (lastOperationDataSeen_)
        {
            ASSERT_IFNOT(NULL == operationData, "Last operationData has been already seen");
            return S_OK;
        }
        errorCode_ = errorCode;
        lastOperationDataSeen_ = (NULL == operationData) || FAILED(errorCode_);
        if (operationDataWaiters_.empty())
        {
            if (SUCCEEDED(errorCode_) && NULL != operationData)
            {
                try { operationDataQueue_.insert(operationDataQueue_.end(), operationData); }
                catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
                catch (...) { hr = E_FAIL; }
                if (FAILED(hr))
                {
                    ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceMemberCopyContextProcessing(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        reinterpret_cast<uintptr_t>(operationData)
                        );
        
                    return hr;
                }

                WriteNoise(
                    TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                    "this={0} partitionId={1} - Operation data {2} enqueued",
                    this,
                    partitionId_,
                    operationData
                    );
            }
        }
        else
        {
            ASSERT_IFNOT(operationDataQueue_.empty(), "Unexpected operationData queue");

            operationDataCompleted = operationData;
            if (lastOperationDataSeen_)
            {
                ASSERT_IF(TRUE == lastOperationDataDispatched_, "Last operationData should not have been dispatched");

                lastOperationDataDispatched_ = TRUE;
                waitersCompleted = std::move(operationDataWaiters_);
            }
            else
            {
                iterateWaiterCompleted = operationDataWaiters_.begin();
                contextCompleted = *iterateWaiterCompleted;
                operationDataWaiters_.erase(iterateWaiterCompleted);

                WriteNoise(
                    TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                    "this={0} partitionId={1} - Operation context {2} dequeued",
                    this,
                    partitionId_,
                    contextCompleted
                    );
            }
        }
        hr = errorCode_;
    }
    if (!waitersCompleted.empty())
    {
        ASSERT_IF(NULL != contextCompleted, "Unexpected waiter completed");

        for (iterateWaiterCompleted = waitersCompleted.begin(); waitersCompleted.end() != iterateWaiterCompleted; iterateWaiterCompleted++)
        {
            if (FAILED(hr))
            {
                WriteNoise(
                    TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                    "this={0} partitionId={1} - Completing operation context {2} asynchronously with error code {3}",
                    this,
                    partitionId_,
                    (*iterateWaiterCompleted),
                    hr
                    );
            }
            else if (NULL == operationDataCompleted)
            {
                WriteNoise(
                    TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                    "this={0} partitionId={1} - Completing operation context {2} asynchronously with last operation data {3}",
                    this,
                    partitionId_,
                    (*iterateWaiterCompleted),
                    L"(NULL)"
                    );
            }
            else
            {
                operationDataCompleted->AddRef();

                WriteNoise(
                    TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                    "this={0} partitionId={1} - Completing operation context {2} asynchronously with last operation data {3}",
                    this,
                    partitionId_,
                    (*iterateWaiterCompleted),
                    L"(empty)"
                    );
            }

            COperationDataStream::CompleteWaiterOnThreadPool(hr, *iterateWaiterCompleted, operationDataCompleted);
        }
        waitersCompleted.clear();

        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceMemberCopyContextProcessing(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                hr
                );
        }
        else if (NULL == operationDataCompleted)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceMemberCopyContextProcessing(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                L"NULL"
                );
        }
        else
        {
            WriteNoise(
                TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                "this={0} partitionId={1} - Signal that the last operation data {2} has been dispatched",
                this,
                partitionId_,
                L"(empty)"
                );

            operationDataCompleted->Release();
        }

        waitLastOperationDataDispatched_.Set();
    }
    if (NULL != contextCompleted)
    {
        ASSERT_IF(!waitersCompleted.empty(), "Unexpected waiters completed");
        ASSERT_IF(NULL == operationDataCompleted, "Unexpected operationData completed");

        if (FAILED(errorCode_))
        {
            WriteNoise(
                TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                "this={0} partitionId={1} - Completing operation context {2} asynchronously with error code {3}",
                this,
                partitionId_,
                contextCompleted,
                errorCode_
                );
        }
        else
        {
            WriteNoise(
                TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
                "this={0} partitionId={1} - Completing operation context {2} asynchronously with operation data {3}",
                this,
                partitionId_,
                contextCompleted,
                operationDataCompleted
                );
        }

        COperationDataStream::CompleteWaiterOnThreadPool(hr, contextCompleted, operationDataCompleted);
    }
    return hr;
}

HRESULT COperationDataStream::ForceDrain(__in HRESULT errorCode)
{
    std::list<CGetOperationDataAsyncOperationContext*> waitersCompleted;

    {
        Common::AcquireWriteLock lockOperationQueue(operationDataLock_);
        for (std::list<IFabricOperationData*>::iterator iterateOperation = operationDataQueue_.begin();
               operationDataQueue_.end() != iterateOperation;
               iterateOperation++)
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_2_StatefulServiceMemberCopyContextProcessing(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                reinterpret_cast<uintptr_t>((*iterateOperation))
                );

            (*iterateOperation)->Release();
        }
        operationDataQueue_.clear();
        waitersCompleted = std::move(operationDataWaiters_);
        lastOperationDataSeen_ = TRUE;
        errorCode_ = errorCode;
        lastOperationDataDispatched_ = TRUE;
        waitLastOperationDataDispatched_.Set();
    }

    for (std::list<CGetOperationDataAsyncOperationContext*>::iterator iterateWaiterCompleted = waitersCompleted.begin(); 
           waitersCompleted.end() != iterateWaiterCompleted; 
           iterateWaiterCompleted++)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceMemberCopyContextProcessing, 
            "this={0} partitionId={1} - Completing operation context {2} with last (NULL) operation data and error code {3}",
            this,
            partitionId_,
            *iterateWaiterCompleted,
            errorCode
            );

        COperationDataStream::CompleteWaiterOnThreadPool(S_OK, *iterateWaiterCompleted, NULL);
    }
    return S_OK;
}

void COperationDataStream::CompleteWaiter(
    __in HRESULT errorCode, 
    __in CGetOperationDataAsyncOperationContext* contextCompleted, 
    __in IFabricOperationData* operationCompleted)
{
    IFabricAsyncOperationCallback* callbackCompleted = NULL;
    contextCompleted->set_OperationData(errorCode, operationCompleted);
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

void COperationDataStream::CompleteWaiterOnThreadPool(
    __in HRESULT errorCode, 
    __in CGetOperationDataAsyncOperationContext* contextCompleted, 
    __in IFabricOperationData* operationCompleted)
{
    BOOL threadPoolPostSuccess = TRUE;
    try
    {
        Common::Threadpool::Post
        (
            [errorCode, contextCompleted, operationCompleted]
            {
                COperationDataStream::CompleteWaiter(errorCode, contextCompleted, operationCompleted); 
            }
        );
    }
    catch (std::bad_alloc const &)
    {
        threadPoolPostSuccess = FALSE;
    }
    
    if (!threadPoolPostSuccess)
    {
        CompleteWaiter(errorCode, contextCompleted, operationCompleted);
    }
}

