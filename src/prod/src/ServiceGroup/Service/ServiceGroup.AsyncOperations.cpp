// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.OperationExtendedBuffer.h"
#include "ServiceGroup.StatefulService.h"

using namespace ServiceGroup;
using namespace ServiceModel;

//
// Constructor/Destructor.
//
COperationContext::COperationContext(void)
{
    WriteNoise(TraceSubComponentOperationContext, "this={0} - ctor", this);

    completedSynchronously_ = FALSE;
    state_ = NULL;
    errorCode_ = S_OK;
}

COperationContext::~COperationContext(void)
{
    WriteNoise(TraceSubComponentOperationContext, "this={0} - dtor", this);

    if (NULL != state_)
    {
        state_->Release();
    }
}

//
// Initialize/Cleanup of the context.
//
STDMETHODIMP COperationContext::FinalConstruct(void)
{
    return S_OK;
}

STDMETHODIMP COperationContext::FinalDestruct(void)
{
    if (NULL != state_)
    {
        state_->Release();
        state_ = NULL;
    }
    return S_OK;
}

//
// IFabricAsyncOperationContext methods.
//
BOOLEAN STDMETHODCALLTYPE COperationContext::IsCompleted()
{
    return wait_.IsSet();
}

BOOLEAN STDMETHODCALLTYPE COperationContext::CompletedSynchronously()
{
    return completedSynchronously_;
}

STDMETHODIMP COperationContext::set_Callback(__in IFabricAsyncOperationCallback* state)
{
    if (NULL == state)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    state->AddRef();
    state_ = state;
    return S_OK;
}

STDMETHODIMP COperationContext::get_Callback(__out IFabricAsyncOperationCallback** state)
{
    if (NULL == state)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    if (NULL != state_)
    {
        state_->AddRef();
    }
    *state = state_;
    return S_OK;
}

STDMETHODIMP COperationContext::Cancel(void)
{
    ServiceGroupCommonEventSource::GetEvents().Info_0_OperationContext(reinterpret_cast<uintptr_t>(this));

    return Common::ComUtility::OnPublicApiReturn(S_FALSE);
}

STDMETHODIMP COperationContext::set_IsCompleted(void)
{
    return Common::ComUtility::OnPublicApiReturn(wait_.Set().ToHResult());
}

STDMETHODIMP COperationContext::set_CompletedSynchronously(__in BOOLEAN completedSynchronously)
{
    completedSynchronously_ = completedSynchronously;
    return S_OK;
}

STDMETHODIMP COperationContext::Wait(__in DWORD dwMilliseconds)
{
    auto waitResult = wait_.Wait(dwMilliseconds);
    return Common::ComUtility::OnPublicApiReturn(waitResult.ToHResult());
}

//
// IFabricAsyncOperationCallback methods.
//
void STDMETHODCALLTYPE COperationContext::Invoke(__in IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);
}

//
// Constructor/Destructor.
//
CGetOperationAsyncOperationContext::CGetOperationAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentGetOperationAsyncContext, "this={0} - ctor", this);

    operation_ = NULL;
}

CGetOperationAsyncOperationContext::~CGetOperationAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentGetOperationAsyncContext, "this={0} - dtor", this);

    if (NULL != operation_)
    {
        operation_->Release();
    }
}

//
// Overrides.
//
STDMETHODIMP CGetOperationAsyncOperationContext::set_Callback(__in IFabricAsyncOperationCallback* state)
{
    if (NULL != state)
    {
        state->AddRef();
        state_ = state;
    }
    return S_OK;
}

STDMETHODIMP CGetOperationAsyncOperationContext::get_Callback(__out IFabricAsyncOperationCallback** state)
{
    if (NULL != state_)
    {
        state_->AddRef();
    }
    *state = state_;
    return S_OK;
}

//
// Additional methods.
//
STDMETHODIMP CGetOperationAsyncOperationContext::set_Operation(__in HRESULT errorCode, __in_opt IFabricOperation* operation)
{
    if (NULL != operation)
    {
        operation->AddRef();
    }
    errorCode_ = errorCode;
    operation_ = operation;
    return S_OK;
}

STDMETHODIMP CGetOperationAsyncOperationContext::get_Operation(__out IFabricOperation** operation)
{
    if (NULL == operation)
    {
        return E_POINTER;
    }
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }
    if (NULL != operation_)
    {
        operation_->AddRef();
    }
    *operation = operation_;
    return S_OK;
}

STDMETHODIMP CGetOperationAsyncOperationContext::FinalDestruct(void)
{
    COperationContext::FinalDestruct();
    if (NULL != operation_)
    {
        operation_->Release();
        operation_ = NULL;
    }
    return S_OK;
}

//
// Constructor/Destructor.
//
CGetOperationDataAsyncOperationContext::CGetOperationDataAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentGetOperationDataAsyncContext, "this={0} - ctor", this);

    operationData_ = NULL;
}

CGetOperationDataAsyncOperationContext::~CGetOperationDataAsyncOperationContext(void)
{
    WriteNoise(TraceSubComponentGetOperationDataAsyncContext, "this={0} - dtor", this);

    if (NULL != operationData_)
    {
        operationData_->Release();
    }
}

//
// Overrides.
//
STDMETHODIMP CGetOperationDataAsyncOperationContext::set_Callback(__in IFabricAsyncOperationCallback* state)
{
    if (NULL != state)
    {
        state->AddRef();
        state_ = state;
    }
    return S_OK;
}

STDMETHODIMP CGetOperationDataAsyncOperationContext::get_Callback(__out IFabricAsyncOperationCallback** state)
{
    if (NULL != state_)
    {
        state_->AddRef();
    }
    *state = state_;
    return S_OK;
}

//
// Additional methods.
//
STDMETHODIMP CGetOperationDataAsyncOperationContext::set_OperationData(__in HRESULT errorCode, __in_opt IFabricOperationData* operationData)
{
    if (NULL != operationData)
    {
        operationData->AddRef();
    }
    errorCode_ = errorCode;
    operationData_ = operationData;
    return S_OK;
}

STDMETHODIMP CGetOperationDataAsyncOperationContext::get_OperationData(__out IFabricOperationData** operationData)
{
    if (NULL == operationData)
    {
        return E_POINTER;
    }
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }
    if (NULL != operationData_)
    {
        operationData_->AddRef();
    }
    *operationData = operationData_;
    return S_OK;
}

STDMETHODIMP CGetOperationDataAsyncOperationContext::FinalDestruct(void)
{
    COperationContext::FinalDestruct();
    if (NULL != operationData_)
    {
        operationData_->Release();
        operationData_ = NULL;
    }
    return S_OK;
}

//
// Constructor/Destructor.
//
CBaseAsyncOperation::CBaseAsyncOperation(__in_opt CBaseAsyncOperation* parent)
{
    parentOperation_ = parent;
    errorCode_ = S_OK;
    startedSuccessfully_ = FALSE;
}

CBaseAsyncOperation::~CBaseAsyncOperation(void)
{
}

//
// Overrides from CAsyncOperationContext.
//
STDMETHODIMP CBaseAsyncOperation::set_IsCompleted(void)
{
    if (NULL != parentOperation_)
    {
        parentOperation_->set_IsCompleted();
    }
    COperationContext::set_IsCompleted();  
    return S_OK;
}

//
// Additional methods.
//
STDMETHODIMP CBaseAsyncOperation::End(void)
{
    COperationContext::Wait(INFINITE);
    return errorCode_;
}

STDMETHODIMP CBaseAsyncOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);

    CBaseAsyncOperation::set_IsCompleted();
    return errorCode_;
}

BOOLEAN CBaseAsyncOperation::HasStartedSuccessfully(void)
{
    return startedSuccessfully_;
}

//
// Constructor/Destructor.
//
CCompositeAsyncOperation::CCompositeAsyncOperation(__in_opt CBaseAsyncOperation* parent) :
    CBaseAsyncOperation(parent)
{
    WriteNoise(TraceSubComponentCompositeAsyncOperation, "this={0} - ctor", this);

    count_ = 0;
}

CCompositeAsyncOperation::~CCompositeAsyncOperation(void)
{
    WriteNoise(TraceSubComponentCompositeAsyncOperation, "this={0} - dtor", this);

    for (BaseAsyncOperation_Iterator iterate = asyncOperations_.begin(); asyncOperations_.end() != iterate; iterate++)
    {
        (*iterate)->Release();
    }
}

//
// Overrides from CBaseAsyncOperation/CAsyncOperationContext.
//
STDMETHODIMP CCompositeAsyncOperation::set_IsCompleted(void)
{
    if (0 == ::InterlockedDecrement64(&count_))
    {
        CBaseAsyncOperation::set_IsCompleted();  
    }
    return S_OK;
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CCompositeAsyncOperation::Begin(void)
{
    count_ = asyncOperations_.size();
    size_t startedSuccessfullyCount = 0;
    ASSERT_IF(0 == count_, "Unexpected number of child operations");
    HRESULT hr = E_FAIL;
    BaseAsyncOperation_Iterator iterateEach;
    BaseAsyncOperation_Iterator iterateEachFailed;
    for (iterateEach = asyncOperations_.begin(); asyncOperations_.end() != iterateEach; iterateEach++)
    {
        hr = (*iterateEach)->Begin();
        if ((*iterateEach)->HasStartedSuccessfully())
        {
            startedSuccessfullyCount++;
        }
        if (FAILED(hr))
        {
            break;
        }
    }
    if (FAILED(hr))
    {
        for (iterateEachFailed = asyncOperations_.begin(); asyncOperations_.end() != iterateEachFailed; iterateEachFailed++)
        {
            if (iterateEachFailed == iterateEach) break;
            (*iterateEachFailed)->End();
        }
    }
    startedSuccessfully_ = (startedSuccessfullyCount == asyncOperations_.size());
    return hr;
}

STDMETHODIMP CCompositeAsyncOperation::End(void)
{
    HRESULT hr = COperationContext::Wait(INFINITE);
    ASSERT_IF(FAILED(hr), "Unexpected composite operation termination");
    for (BaseAsyncOperation_Iterator iterateEach = asyncOperations_.begin(); asyncOperations_.end() != iterateEach; iterateEach++)
    {
        hr = (*iterateEach)->End();
        if (FAILED(hr) && SUCCEEDED(errorCode_))
        {
            errorCode_ = hr;
        }
    }
    return errorCode_;
}

//
// Additional methods.
//
STDMETHODIMP CCompositeAsyncOperation::Compose(__in CBaseAsyncOperation* operation)
{
    if (NULL == operation)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    try { asyncOperations_.insert(asyncOperations_.end(), operation); }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }

    return hr;
}

//
// Constructor/Destructor.
//
CCompositeAsyncOperationAsync::CCompositeAsyncOperationAsync(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId) 
    : CCompositeAsyncOperation(parent), partitionId_(partitionId)
{
    WriteNoise(TraceSubComponentCompositeAsyncOperationAsync, "this={0} - ctor", this);

    count_ = 0;
}

CCompositeAsyncOperationAsync::~CCompositeAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentCompositeAsyncOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CCompositeAsyncOperationAsync::set_IsCompleted(void)
{
    if (0 == ::InterlockedDecrement64(&count_))
    {
        if (NULL != parentOperation_)
        {
            parentOperation_->AddRef();
        }
        COperationContext::set_IsCompleted();  
        if (NULL != parentOperation_)
        {
            WriteNoise(
                 TraceSubComponentCompositeAsyncOperationAsync, 
                 "this={0} partitionId={1} - Completing parent operation",
                 this,
                 partitionId_
                 );

            parentOperation_->set_IsCompleted();
            parentOperation_->Release();
        }
        else
        {
            WriteNoise(
                 TraceSubComponentCompositeAsyncOperationAsync, 
                 "this={0} partitionId={1} - Invoking callback",
                 this,
                 partitionId_
                 );

            ASSERT_IF(NULL == state_, "Unexpected callback state");
            IFabricAsyncOperationCallback* callback = state_;
            state_ = NULL;
            callback->Invoke(this);
            callback->Release();
        }
    }
    return S_OK;
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CCompositeAsyncOperationAsync::Begin(void)
{
    WriteNoise(
         TraceSubComponentCompositeAsyncOperationAsync, 
         "this={0} partitionId={1} - Beginning operation",
         this,
         partitionId_
         );

    return CCompositeAsyncOperation::Begin();
}

STDMETHODIMP CCompositeAsyncOperationAsync::End(void)
{
    WriteNoise(
         TraceSubComponentCompositeAsyncOperationAsync, 
         "this={0} partitionId={1} - Ending operation",
         this,
         partitionId_
         );

    return CCompositeAsyncOperation::End();
}

//
// Constructor/Destructor.
//
CSingleAsyncOperation::CSingleAsyncOperation(__in_opt CBaseAsyncOperation* parent) : 
    CBaseAsyncOperation(parent)
{
}

CSingleAsyncOperation::~CSingleAsyncOperation(void)
{
}

//
// Overrides from CAsyncOperationContext.
//
void STDMETHODCALLTYPE CSingleAsyncOperation::Invoke(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    if (!context->CompletedSynchronously())
    {
        errorCode_ = Complete(context);
    }
}

//
// Constructor/Destructor.
//
CStatefulAsyncOperation::CStatefulAsyncOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStatefulServiceReplica* serviceReplica
    ) : CSingleAsyncOperation(parent)
{
    if (NULL != serviceReplica)
    {
        serviceReplica->AddRef();
    }
    serviceReplica_ = serviceReplica;
}

CStatefulAsyncOperation::~CStatefulAsyncOperation(void)
{
    if (NULL != serviceReplica_)
    {
        serviceReplica_->Release();
    }
}

//
// Constructor/Destructor.
//
CStatefulOpenOperation::CStatefulOpenOperation(
        __in_opt CBaseAsyncOperation* parent,
        __in FABRIC_REPLICA_OPEN_MODE openMode,
        __in IFabricStatefulServiceReplica* serviceReplica,
        __in IFabricStatefulServicePartition* servicePartition
    ) : CStatefulAsyncOperationAsync(parent, serviceReplica)
{
    WriteNoise(TraceSubComponentSingleStatefulOpenOperationAsync, "this={0} - ctor", this);

    replicator_ = NULL;
    if (NULL != servicePartition)
    {
        servicePartition->AddRef();
    }
    openMode_ = openMode;
    servicePartition_ = servicePartition;
    replicator_ = NULL;
}

CStatefulOpenOperation::~CStatefulOpenOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatefulOpenOperationAsync, "this={0} - dtor", this);

    if (NULL != replicator_)
    {
        replicator_->Release();
    }
    if (NULL != servicePartition_)
    {
        servicePartition_->Release();
    }
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatefulOpenOperation::Begin(void)
{
    ASSERT_IF(NULL == serviceReplica_, "Unexpected service replica");
    ASSERT_IF(NULL == servicePartition_, "Unexpected service partition");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = serviceReplica_->BeginOpen(openMode_, servicePartition_, (IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatefulOpenOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = serviceReplica_->EndOpen(context, &replicator_);
    return CStatefulAsyncOperationAsync::Complete(context);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulOpenOperation::get_Replicator(__out IFabricReplicator** replicator)
{
    if (NULL == replicator)
    {
        return E_POINTER;
    }
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }
    if (NULL == replicator_)
    {
        return E_UNEXPECTED;
    }
    replicator_->AddRef();
    *replicator = replicator_;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatefulAsyncOperationAsync::CStatefulAsyncOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStatefulServiceReplica* serviceReplica
    ) : CStatefulAsyncOperation(parent, serviceReplica)
{
    WriteNoise(TraceSubComponentSingleStatefulOperationAsync, "this={0} - ctor", this);
}

CStatefulAsyncOperationAsync::~CStatefulAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentSingleStatefulOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatefulAsyncOperationAsync::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);

    HRESULT hr = errorCode_;
    if (NULL != parentOperation_)
    {
        parentOperation_->AddRef();
    }
    COperationContext::set_IsCompleted();  
    if (NULL != parentOperation_)
    {
        WriteNoise(
             TraceSubComponentSingleStatefulOperationAsync, 
             "this={0} - Completing parent operation",
             this
             );

        parentOperation_->set_IsCompleted();
        parentOperation_->Release();
    }
    return hr;
}

//
// Constructor/Destructor.
//
CStatefulCloseOperation::CStatefulCloseOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStatefulServiceReplica* serviceReplica
    ) : CStatefulAsyncOperationAsync(parent, serviceReplica)
{
    WriteNoise(TraceSubComponentSingleStatefulCloseOperationAsync, "this={0} - ctor", this);
}

CStatefulCloseOperation::~CStatefulCloseOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatefulCloseOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatefulCloseOperation::Begin(void)
{
    ASSERT_IF(NULL == serviceReplica_, "Unexpected service replica");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = serviceReplica_->BeginClose((IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatefulCloseOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = serviceReplica_->EndClose(context);
    return CStatefulAsyncOperationAsync::Complete(context);
}

//
// Constructor/Destructor.
//
CStatefulChangeRoleOperation::CStatefulChangeRoleOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in std::wstring const & serviceName,
    __in IFabricStatefulServiceReplica* serviceReplica,
    __in FABRIC_REPLICA_ROLE newReplicaRole
    ) : CStatefulAsyncOperationAsync(parent, serviceReplica)
      , serviceName_(serviceName)
{
    WriteNoise(TraceSubComponentSingleStatefulChangeRoleOperationAsync, "this={0} - ctor", this);

    newReplicaRole_ = newReplicaRole;
    serviceEndpoint_ = NULL;
}

CStatefulChangeRoleOperation::~CStatefulChangeRoleOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatefulChangeRoleOperationAsync, "this={0} - dtor", this);

    if (NULL != serviceEndpoint_)
    {
        serviceEndpoint_->Release();
    }
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatefulChangeRoleOperation::Begin(void)
{
    ASSERT_IF(NULL == serviceReplica_, "Unexpected service replica");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = serviceReplica_->BeginChangeRole(
        newReplicaRole_,
        (IFabricAsyncOperationCallback*)this, 
        &context
        );
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);  
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatefulChangeRoleOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = serviceReplica_->EndChangeRole(context, &serviceEndpoint_);
    return CStatefulAsyncOperationAsync::Complete(context);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulChangeRoleOperation::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    ASSERT_IF(FAILED(errorCode_), "Unexpected call");
    if (NULL == serviceEndpoint)
    {
        return E_POINTER;
    }
    if (NULL != serviceEndpoint_)
    {
        serviceEndpoint_->AddRef();

        LPCWSTR bufferedValue = serviceEndpoint_->get_String();
        if (NULL != bufferedValue)
        {
            WriteNoise(
                TraceSubComponentSingleStatefulChangeRoleOperationAsync, 
                "this={0} - Returning service endpoint {1}",
                this,
                bufferedValue
                );
        }
        else
        {
            WriteNoise(
                TraceSubComponentSingleStatefulChangeRoleOperationAsync, 
                "this={0} - Returning service endpoint NULL",
                this
                );
        }
    }
    *serviceEndpoint = serviceEndpoint_;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatelessAsyncOperation::CStatelessAsyncOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStatelessServiceInstance* serviceInstance
    ) : CSingleAsyncOperation(parent)
{
    if (NULL != serviceInstance)
    {
        serviceInstance->AddRef();
    }
    serviceInstance_ = serviceInstance;
}

CStatelessAsyncOperation::~CStatelessAsyncOperation(void)
{
    if (NULL != serviceInstance_)
    {
        serviceInstance_->Release();
    }
}

//
// Constructor/Destructor.
//
CStatelessAsyncOperationAsync::CStatelessAsyncOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
            __in IFabricStatelessServiceInstance* serviceInstance
    ) : CStatelessAsyncOperation(parent, serviceInstance)
{
    WriteNoise(TraceSubComponentSingleStatelessOperationAsync, "this={0} - ctor", this);
}

CStatelessAsyncOperationAsync::~CStatelessAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentSingleStatelessOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatelessAsyncOperationAsync::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);

    HRESULT hr = errorCode_;
    COperationContext::set_IsCompleted();  
    if (NULL != parentOperation_)
    {
        parentOperation_->set_IsCompleted();
    }
    return hr;
}

//
// Constructor/Destructor.
//
CStatelessOpenOperation::CStatelessOpenOperation(
        __in_opt CBaseAsyncOperation* parent,
        __in std::wstring const & serviceName,
        __in IFabricStatelessServiceInstance* serviceInstance,
        __in IFabricStatelessServicePartition* servicePartition
    ) : CStatelessAsyncOperationAsync(parent, serviceInstance), serviceName_(serviceName)
{
    WriteNoise(TraceSubComponentSingleStatelessOpenOperationAsync, "this={0} - ctor", this);

    if (NULL != servicePartition)
    {
        servicePartition->AddRef();
    }
    servicePartition_ = servicePartition;
    serviceEndpoint_ = NULL;
}

CStatelessOpenOperation::~CStatelessOpenOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatelessOpenOperationAsync, "this={0} - dtor", this);

    if (NULL != serviceEndpoint_)
    {
        serviceEndpoint_->Release();
    }
    if (NULL != servicePartition_)
    {
        servicePartition_->Release();
    }
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatelessOpenOperation::Begin(void)
{
    ASSERT_IF(NULL == serviceInstance_, "Unexpected service instance");
    ASSERT_IF(NULL == servicePartition_, "Unexpected service partition");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = serviceInstance_->BeginOpen(servicePartition_, (IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatelessOpenOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = serviceInstance_->EndOpen(context, &serviceEndpoint_);
    return CStatelessAsyncOperationAsync::Complete(context);
}

//
// Additional methods.
//
STDMETHODIMP CStatelessOpenOperation::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    ASSERT_IF(FAILED(errorCode_), "Unexpected call");
    if (NULL == serviceEndpoint)
    {
        return E_POINTER;
    }
    if (NULL != serviceEndpoint_)
    {
        serviceEndpoint_->AddRef();

        LPCWSTR bufferedValue = serviceEndpoint_->get_String();
        if (NULL != bufferedValue)
        {
            WriteNoise(
                TraceSubComponentSingleStatelessOpenOperationAsync, 
                "this={0} - Returning service endpoint {1}",
                this,
                bufferedValue
                );
        }
        else
        {
            WriteNoise(
                TraceSubComponentSingleStatelessOpenOperationAsync, 
                "this={0} - Returning service endpoint NULL",
                this
                );
        }
    }
    *serviceEndpoint = serviceEndpoint_;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatelessCloseOperation::CStatelessCloseOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStatelessServiceInstance* serviceInstance
    ) : CStatelessAsyncOperationAsync(parent, serviceInstance)
{
    WriteNoise(TraceSubComponentSingleStatelessCloseOperationAsync, "this={0} - ctor", this);
}

CStatelessCloseOperation::~CStatelessCloseOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatelessCloseOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatelessCloseOperation::Begin(void)
{
    ASSERT_IF(NULL == serviceInstance_, "Unexpected service replica");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = serviceInstance_->BeginClose((IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatelessCloseOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = serviceInstance_->EndClose(context);
    return CStatelessAsyncOperationAsync::Complete(context);
}

//
// Constructor/Destructor.
//
CStatelessCompositeOpenOperation::CStatelessCompositeOpenOperation(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId)
    : CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatelessCompositeOpenOperationAsync, "this={0} - ctor", this);
}

CStatelessCompositeOpenOperation::~CStatelessCompositeOpenOperation(void)
{
    WriteNoise(TraceSubComponentStatelessCompositeOpenOperationAsync, "this={0} - dtor", this);
}

//
// Additional methods.
//
STDMETHODIMP CStatelessCompositeOpenOperation::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    ASSERT_IF(FAILED(errorCode_), "Unexpected call");
    if (NULL == serviceEndpoint)
    {
        return E_POINTER;
    }
    CCompositeServiceEndpoint* compositeServiceEndpoint = new (std::nothrow) CCompositeServiceEndpoint();
    if (NULL == compositeServiceEndpoint)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = E_FAIL;
    std::list<CBaseAsyncOperation*>::iterator iterateList;
    for (iterateList = asyncOperations_.begin(); asyncOperations_.end() != iterateList; iterateList++)
    {
        IFabricStringResult* stringResult = NULL;
        hr = ((CStatelessOpenOperation*)(*iterateList))->get_ServiceEndpoint(&stringResult);
        if (FAILED(hr))
        {
            break;
        }
        if (NULL == stringResult)
        {
            continue;
        }
        hr = compositeServiceEndpoint->Compose(stringResult, ((CStatelessOpenOperation*)(*iterateList))->ServiceName);
        stringResult->Release();
        if (FAILED(hr))
        {
            break;
        }
    }
    if (FAILED(hr))
    {
        compositeServiceEndpoint->Release();
        return hr;
    }
    *serviceEndpoint = compositeServiceEndpoint;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatefulCompositeChangeRoleOperation::CStatefulCompositeChangeRoleOperation(__in_opt CBaseAsyncOperation* parent,  __in FABRIC_PARTITION_ID partitionId, __in FABRIC_REPLICA_ROLE newReplicaRole)
    : CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatefulCompositeChangeRoleOperationAsync, "this={0} - ctor", this);

    newReplicaRole_ = newReplicaRole;
}

CStatefulCompositeChangeRoleOperation::~CStatefulCompositeChangeRoleOperation(void)
{
    WriteNoise(TraceSubComponentStatefulCompositeChangeRoleOperationAsync, "this={0} - dtor", this);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulCompositeChangeRoleOperation::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    ASSERT_IF(FAILED(errorCode_), "Unexpected call");
    if (NULL == serviceEndpoint)
    {
        return E_POINTER;
    }
    CCompositeServiceEndpoint* compositeServiceEndpoint = new (std::nothrow) CCompositeServiceEndpoint();
    if (NULL == compositeServiceEndpoint)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = E_FAIL;
    std::list<CBaseAsyncOperation*>::iterator iterateList;
    for (iterateList = asyncOperations_.begin(); asyncOperations_.end() != iterateList; iterateList++)
    {
        IFabricStringResult* stringResult = NULL;
        hr = ((CStatefulChangeRoleOperation*)(*iterateList))->get_ServiceEndpoint(&stringResult);
        if (FAILED(hr))
        {
            break;
        }
        if (NULL == stringResult)
        {
            continue;
        }
        hr = compositeServiceEndpoint->Compose(stringResult, ((CStatefulChangeRoleOperation*)(*iterateList))->ServiceName);
        stringResult->Release();
        if (FAILED(hr))
        {
            break;
        }
    }
    if (FAILED(hr))
    {
        compositeServiceEndpoint->Release();
        return hr;
    }
    *serviceEndpoint = compositeServiceEndpoint;
    return S_OK;
}

STDMETHODIMP CStatefulCompositeChangeRoleOperation::get_ReplicaRole(__out FABRIC_REPLICA_ROLE* newReplicaRole)
{

    if (NULL == newReplicaRole)
    {
        return E_POINTER;
    }
    *newReplicaRole = newReplicaRole_;
    return S_OK;
}

//
// Constructor/Destructor.
//
CCompositeServiceEndpoint::CCompositeServiceEndpoint(void)
{
    WriteNoise(TraceSubComponentCompositeServiceEndpoint, "this={0} - ctor", this);
}

CCompositeServiceEndpoint::~CCompositeServiceEndpoint(void)
{
    WriteNoise(TraceSubComponentCompositeServiceEndpoint, "this={0} - dtor", this);
}

//
// IFabricStringResult methods.
//
LPCWSTR STDMETHODCALLTYPE CCompositeServiceEndpoint::get_String(void)
{
    WriteNoise(TraceSubComponentCompositeServiceEndpoint, "this={0} - Returning service endpoint \"{1}\"", this, serviceEndpoint_.c_str());

    return serviceEndpoint_.c_str();
}

//
// Additional methods.
//
STDMETHODIMP CCompositeServiceEndpoint::Compose(
    __in IFabricStringResult* serviceEndpoint,
    __in std::wstring const & serviceName)
{
    HRESULT hr = S_OK;
    if (NULL == serviceEndpoint)
    {
        return E_POINTER;
    }
    LPCWSTR result = serviceEndpoint->get_String();
    try 
    { 
        serviceEndpoint_ += SERVICE_ADDRESS_DOUBLE_DELIMITER;
        serviceEndpoint_ += serviceName;
        serviceEndpoint_ += MEMBER_SERVICE_NAME_DELIMITER;
        if (NULL != result)
        {
            std::wstring resultString(result);
            Common::StringUtility::Replace(resultString, SERVICE_ADDRESS_DELIMITER, SERVICE_ADDRESS_ESCAPED_DELIMITER);
            serviceEndpoint_ += resultString; 
        }
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }

    return hr;
}

//
// Constructor/Destructor.
//
CStateProviderAsyncOperationAsync::CStateProviderAsyncOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStateProvider* stateProvider
    ) : CSingleAsyncOperation(parent)
{
    WriteNoise(TraceSubComponentSingleStateProviderOperationAsync, "this={0} - ctor", this);

    if (NULL != stateProvider)
    {
        stateProvider->AddRef();
    }
    stateProvider_ = stateProvider;
}

CStateProviderAsyncOperationAsync::~CStateProviderAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentSingleStateProviderOperationAsync, "this={0} - dtor", this);

    if (NULL != stateProvider_)
    {
        stateProvider_->Release();
    }
}

STDMETHODIMP CStateProviderAsyncOperationAsync::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);

    HRESULT hr = errorCode_;   
    if (NULL != parentOperation_)
    {
        parentOperation_->AddRef();
    }
    COperationContext::set_IsCompleted();  
    if (NULL != parentOperation_)
    {
        WriteNoise(
            TraceSubComponentSingleStateProviderOperationAsync, 
            "this={0} - Completing parent operation",
            this
            );

        parentOperation_->set_IsCompleted();
        parentOperation_->Release();
    }
    return hr;
}

//
// Constructor/Destructor.
//
CStatefulOnDataLossOperation::CStatefulOnDataLossOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStateProvider* stateProvider
    ) : CStateProviderAsyncOperationAsync(parent, stateProvider)
{
    WriteNoise(TraceSubComponentSingleStatefulOnDataLossOperationAsync, "this={0} - ctor", this);
}

CStatefulOnDataLossOperation::~CStatefulOnDataLossOperation(void)
{
    WriteNoise(TraceSubComponentSingleStatefulOnDataLossOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CStatefulOnDataLossOperation::Begin(void)
{
    ASSERT_IF(NULL == stateProvider_, "Unexpected state provider");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = stateProvider_->BeginOnDataLoss((IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CStatefulOnDataLossOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = stateProvider_->EndOnDataLoss(context, &isStateChanged_);
    return CStateProviderAsyncOperationAsync::Complete(context);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulOnDataLossOperation::get_IsStateChanged(__out BOOLEAN* isStateChanged)
{
    if (NULL == isStateChanged)
    {
        return E_POINTER;
    }

    *isStateChanged = isStateChanged_;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatefulCompositeOnDataLossOperation::CStatefulCompositeOnDataLossOperation( __in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId) :
    CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatefulCompositeOnDataLossOperationAsync, "this={0} - ctor", this);
}

CStatefulCompositeOnDataLossOperation::~CStatefulCompositeOnDataLossOperation(void)
{
    WriteNoise(TraceSubComponentStatefulCompositeOnDataLossOperationAsync, "this={0} - dtor", this);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulCompositeOnDataLossOperation::get_IsStateChanged(__out BOOLEAN* isStateChanged)
{
    if (NULL == isStateChanged)
    {
        return E_POINTER;
    }

    *isStateChanged = FALSE;

    HRESULT hr = E_FAIL;
    std::list<CBaseAsyncOperation*>::iterator iterateList;
    for (iterateList = asyncOperations_.begin(); asyncOperations_.end() != iterateList; iterateList++)
    {
        BOOLEAN innerIsStateChanged = FALSE;
        hr = ((CStatefulOnDataLossOperation*)(*iterateList))->get_IsStateChanged(&innerIsStateChanged);
        if (FAILED(hr))
        {
            break;
        }
        if (innerIsStateChanged)
        {
            *isStateChanged = TRUE;
            break;
        }
    }
    return hr;
}

//
// Constructor/Destructor.
//
CEmptyOperationDataAsyncEnumerator::CEmptyOperationDataAsyncEnumerator(void)
{
    WriteNoise(TraceSubComponentEmptyOperationDataAsyncEnumerator, "this={0} - ctor", this);
}

CEmptyOperationDataAsyncEnumerator::~CEmptyOperationDataAsyncEnumerator(void)
{
    WriteNoise(TraceSubComponentEmptyOperationDataAsyncEnumerator, "this={0} - dtor", this);
}

//
// IFabricOperationDataStream methods.
//
STDMETHODIMP CEmptyOperationDataAsyncEnumerator::BeginGetNext(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    COperationContext* asyncContext = new (std::nothrow) COperationContext();
    if (NULL == asyncContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_EmptyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    HRESULT hr = asyncContext->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_EmptyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            hr
            );

        asyncContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_0_EmptyOperationDataAsyncEnumerator(
        reinterpret_cast<uintptr_t>(this),
        reinterpret_cast<uintptr_t>(asyncContext)
        );

    asyncContext->set_Callback(callback);
    asyncContext->set_IsCompleted();
    asyncContext->set_CompletedSynchronously(TRUE);
    callback->Invoke(asyncContext);
    *context = asyncContext;
    return S_OK;
}

STDMETHODIMP CEmptyOperationDataAsyncEnumerator::EndGetNext( 
    __in IFabricAsyncOperationContext* context,
    __out IFabricOperationData** operationData
    )
{
    if (NULL == context || NULL == operationData)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    
    COperationContext* asyncContext = (COperationContext*)context;

    WriteNoise(
        TraceSubComponentEmptyOperationDataAsyncEnumerator, 
        "this={0} - Waiting for operation context {1} to complete",
        this,
        asyncContext
        );

    asyncContext->Wait(INFINITE);

    ServiceGroupReplicationEventSource::GetEvents().Info_1_EmptyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

    *operationData = NULL;
    return S_OK;
}

//
// Constructor/Destructor.
//
CCompositeAtomicGroupCommitRollbackOperation::CCompositeAtomicGroupCommitRollbackOperation(void)
    : CCompositeAsyncOperation(NULL)
{
    WriteNoise(TraceSubComponentCompositeAtomicGroupCommitRollbackOperation, "this={0} - ctor", this);
}

CCompositeAtomicGroupCommitRollbackOperation::~CCompositeAtomicGroupCommitRollbackOperation(void)
{
    WriteNoise(TraceSubComponentCompositeAtomicGroupCommitRollbackOperation, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CCompositeAtomicGroupCommitRollbackOperation::Begin(void)
{
    count_ = asyncOperations_.size();
    ASSERT_IF(0 == count_, "Unexpected number of child operations");
    HRESULT hr = E_FAIL;
    BaseAsyncOperation_Iterator iterateEach;
    for (iterateEach = asyncOperations_.begin(); asyncOperations_.end() != iterateEach; iterateEach++)
    {
        hr = (*iterateEach)->Begin();
        ASSERT_IF(FAILED(hr), SERVICE_REPLICA_DOWN);
    }
    return hr;
}

//
// Constructor/Destructor.
//
CAtomicGroupCommitRollbackOperation::CAtomicGroupCommitRollbackOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
    __in BOOLEAN isCommit,
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber
    ) : CSingleAsyncOperation(parent)
{
    WriteNoise(TraceSubComponentSingleAtomicGroupCommitRollbackOperation, "this={0} - ctor", this);

    if (NULL != atomicGroupStateProvider)
    {
        atomicGroupStateProvider->AddRef();
    }
    atomicGroupStateProvider_ = atomicGroupStateProvider;
    isCommit_ = isCommit;
    atomicGroupId_ = atomicGroupId;
    sequenceNumber_ = sequenceNumber;
}

CAtomicGroupCommitRollbackOperation::~CAtomicGroupCommitRollbackOperation(void)
{
    WriteNoise(TraceSubComponentSingleAtomicGroupCommitRollbackOperation, "this={0} - dtor", this);

    if (NULL != atomicGroupStateProvider_)
    {
        atomicGroupStateProvider_->Release();
    }
}
//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CAtomicGroupCommitRollbackOperation::Begin(void)
{
    ASSERT_IF(NULL == atomicGroupStateProvider_, "Unexpected service replica");

    IFabricAsyncOperationContext* context = NULL;
    if (isCommit_)
    {
        errorCode_ = atomicGroupStateProvider_->BeginAtomicGroupCommit(atomicGroupId_, sequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    }
    else
    {
        errorCode_ = atomicGroupStateProvider_->BeginAtomicGroupRollback(atomicGroupId_, sequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    }
    if (FAILED(errorCode_))
    {
        return CBaseAsyncOperation::Complete(context);
    }
    ASSERT_IF(NULL == context, "Unexpected context");
    if (context->CompletedSynchronously())
    {
        errorCode_ = Complete(context);
    }
    context->Release();
    return errorCode_;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CAtomicGroupCommitRollbackOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    if (isCommit_)
    {
        errorCode_ = atomicGroupStateProvider_->EndAtomicGroupCommit(context);
    }
    else
    {
        errorCode_ = atomicGroupStateProvider_->EndAtomicGroupRollback(context);
    }
    return CBaseAsyncOperation::Complete(context);
}

//
// Constructor/Destructor.
//
CAtomicGroupUndoProgressOperation::CAtomicGroupUndoProgressOperation(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber
    ) : CStateProviderAsyncOperationAsync(parent, NULL)
{
    WriteNoise(TraceSubComponentSingleAtomicGroupUndoProgressOperationAsync, "this={0} - ctor", this);

    if (NULL != atomicGroupStateProvider)
    {
        atomicGroupStateProvider->AddRef();
    }
    atomicGroupStateProvider_ = atomicGroupStateProvider;
    fromCommitSequenceNumber_ = sequenceNumber;
}

CAtomicGroupUndoProgressOperation::~CAtomicGroupUndoProgressOperation(void)
{
    WriteNoise(TraceSubComponentSingleAtomicGroupUndoProgressOperationAsync, "this={0} - dtor", this);

    if (NULL != atomicGroupStateProvider_)
    {
        atomicGroupStateProvider_->Release();
    }
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CAtomicGroupUndoProgressOperation::Begin(void)
{
    ASSERT_IF(NULL == atomicGroupStateProvider_, "Unexpected service replica");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = atomicGroupStateProvider_->BeginUndoProgress(fromCommitSequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CAtomicGroupUndoProgressOperation::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = atomicGroupStateProvider_->EndUndoProgress(context);
    return CStateProviderAsyncOperationAsync::Complete(context);
}

//
// Constructor/Destructor.
//
CUpdateEpochOperationAsync::CUpdateEpochOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricStateProvider* stateProvider,
    __in FABRIC_EPOCH const & epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber
    ) : CStateProviderAsyncOperationAsync(parent, stateProvider)
{
    WriteNoise(TraceSubComponentSingleUpdateEpochOperationAsync, "this={0} - ctor", this);

    epoch_ = epoch;
    previousEpochLastSequenceNumber_ = previousEpochLastSequenceNumber;
} 

CUpdateEpochOperationAsync::~CUpdateEpochOperationAsync(void)
{
    WriteNoise(TraceSubComponentSingleUpdateEpochOperationAsync, "this={0} - dtor", this);
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CUpdateEpochOperationAsync::Begin(void)
{
    ASSERT_IF(NULL == stateProvider_, "Unexpected state provider");

    IFabricAsyncOperationContext* context = NULL;
    errorCode_ = stateProvider_->BeginUpdateEpoch(&epoch_, previousEpochLastSequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    if (FAILED(errorCode_))
    {
        return errorCode_;
    }

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CUpdateEpochOperationAsync::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    errorCode_ = stateProvider_->EndUpdateEpoch(context);
    return CStateProviderAsyncOperationAsync::Complete(context);
}

//
// Constructor/Destructor.
//
CCopyOperationDataAsyncEnumerator::CCopyOperationDataAsyncEnumerator()
{
    WriteNoise(TraceSubComponentCopyOperationDataAsyncEnumerator, "this={0} - ctor", this);
}

CCopyOperationDataAsyncEnumerator::~CCopyOperationDataAsyncEnumerator()
{
    WriteNoise(TraceSubComponentCopyOperationDataAsyncEnumerator, "this={0} - dtor", this);

    for (std::map<Common::Guid, IFabricOperationDataStream*>::iterator iterateEnums = copyOperationEnumerators_.begin();
         iterateEnums != copyOperationEnumerators_.end();
         iterateEnums++
         )
    {
        iterateEnums->second->Release();
    }
    copyOperationEnumerators_.clear();
}

HRESULT CCopyOperationDataAsyncEnumerator::FinalConstruct(
    __in Common::Guid const & serviceGroupGuid,
    __in std::map<Common::Guid, IFabricOperationDataStream*> & copyOperationEnumerators
    )
{
    serviceGroupGuid_ = serviceGroupGuid;
    copyOperationEnumerators_ = std::move(copyOperationEnumerators);
    enumeratorIterator_ = copyOperationEnumerators_.begin();

    return S_OK;
}

HRESULT CCopyOperationDataAsyncEnumerator::BeginGetNext(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;

    CCopyOperationCallbackContext* outerCallbackContext = new (std::nothrow) CCopyOperationCallbackContext();
    if (NULL == outerCallbackContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_EmptyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    hr = outerCallbackContext->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_EmptyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            hr
            );

        outerCallbackContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    if (copyOperationEnumerators_.end() == enumeratorIterator_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_0_CopyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        outerCallbackContext->set_CompletedSynchronously(TRUE);
        outerCallbackContext->set_IsCompleted();
        *context = outerCallbackContext;
        return S_OK;
    }

    outerCallbackContext->set_Callback(callback);
    outerCallbackContext->set_CompletedSynchronously(FALSE);

    IFabricAsyncOperationContext* serviceContext = NULL;
    hr = enumeratorIterator_->second->BeginGetNext(outerCallbackContext, &serviceContext);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_CopyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            enumeratorIterator_->first,
            hr
            );

        outerCallbackContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    hr = S_OK;
    if (serviceContext->CompletedSynchronously())
    {
        WriteNoise(
            TraceSubComponentCopyOperationDataAsyncEnumerator, 
            "this={0} - Service BeginGetNext completed synchronously - Context {1}",
            this,
            outerCallbackContext
            );

        outerCallbackContext->set_OperationContext(serviceContext);
        outerCallbackContext->set_CompletedSynchronously(TRUE);
        outerCallbackContext->set_IsCompleted();
    }
    else
    {
        WriteNoise(
            TraceSubComponentCopyOperationDataAsyncEnumerator, 
            "this={0} - Service BeginGetNext completed asynchronously - Context {1}",
            this,
            outerCallbackContext
            );
    }
    serviceContext->Release();

    *context = outerCallbackContext;
    return S_OK;
}

STDMETHODIMP CCopyOperationDataAsyncEnumerator::EndGetNext(
    __in IFabricAsyncOperationContext* context,
    __out IFabricOperationData** operationData
    )
{
    if (NULL == context || NULL == operationData)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    WriteNoise(
        TraceSubComponentCopyOperationDataAsyncEnumerator, 
        "this={0} - Context {1}",
        this,
        context
        );

    HRESULT hr = S_OK;

    CCopyOperationCallbackContext* outerCallbackContext = (CCopyOperationCallbackContext*)context;
    outerCallbackContext->Wait(INFINITE);

    IFabricAsyncOperationContext* serviceContext = NULL;
    hr = outerCallbackContext->get_OperationContext(&serviceContext);
    ASSERT_IF(FAILED(hr), "get_OperationContext");
    hr = outerCallbackContext->remove_OperationContext();
    ASSERT_IF(FAILED(hr), "remove_OperationContext");

    if (NULL == serviceContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_1_CopyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        *operationData = NULL;
        return S_OK;
    }

    IFabricOperationData* pdata = NULL;
    hr = enumeratorIterator_->second->EndGetNext(serviceContext, &pdata);
    serviceContext->Release();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_CopyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            enumeratorIterator_->first,
            hr
            );

        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    if (NULL != pdata)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_2_CopyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            enumeratorIterator_->first
            );

        hr = GetExtendedOperationDataBuffer(pdata, enumeratorIterator_->first, operationData);
        pdata->Release();
    }
    else
    {
        WriteNoise(
            TraceSubComponentCopyOperationDataAsyncEnumerator, 
            "this={0} - {1} Service EndGetNext returned NULL operation data",
            this,
            enumeratorIterator_->first);

        ServiceGroupReplicationEventSource::GetEvents().Info_3_CopyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            enumeratorIterator_->first
            );

        enumeratorIterator_++;

        hr = GetExtendedOperationDataBuffer(NULL, serviceGroupGuid_, operationData);
    }
    return Common::ComUtility::OnPublicApiReturn(hr);
}

BOOLEAN CCopyOperationDataAsyncEnumerator::IsEmptyOperationData(
    __in IFabricOperationData* operationData
    )
{
    if (NULL == operationData)
    {
        return TRUE;
    }

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

    HRESULT hr = operationData->GetData(&bufferCount, &replicaBuffers);

    ASSERT_IF(FAILED(hr), "GetData");
    ASSERT_IFNOT(
        (0 == bufferCount && NULL == replicaBuffers) ||
        (0 < bufferCount && NULL != replicaBuffers),
        "bufferData and lengthData dont match");

    return (0 == bufferCount) ? TRUE : FALSE;
}

HRESULT CCopyOperationDataAsyncEnumerator::GetExtendedOperationDataBuffer(
    __in IFabricOperationData* operationData,
    __in Common::Guid const & partitionId,
    __out IFabricOperationData** extendedOperationData
    )
{
    if (NULL == extendedOperationData)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    COutgoingOperationDataExtendedBuffer* pExtOperationData = new (std::nothrow) COutgoingOperationDataExtendedBuffer();
    if (NULL == pExtOperationData)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_2_CopyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));
        return E_OUTOFMEMORY;
    }

    hr = pExtOperationData->FinalConstruct(
        FABRIC_OPERATION_TYPE_NORMAL,
        FABRIC_INVALID_ATOMIC_GROUP_ID,
        operationData,
        partitionId,
        TRUE
        );

    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_CopyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        pExtOperationData->Release();
        return hr;
    }

    hr = pExtOperationData->SerializeExtendedOperationDataBuffer();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_4_CopyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            hr
            );

        pExtOperationData->Release();
        return hr;
    }

    *extendedOperationData = pExtOperationData;
    return S_OK;
}

//
// Constructor/Destructor.
//
CCopyOperationCallbackContext::CCopyOperationCallbackContext()
{
    WriteNoise(TraceSubComponentCopyOperationCallback, "this={0} - ctor", this);

    serviceContext_ = NULL;
}

CCopyOperationCallbackContext::~CCopyOperationCallbackContext(void)
{
    WriteNoise(TraceSubComponentCopyOperationCallback, "this={0} - dtor", this);

    if (NULL != serviceContext_)
    {
        serviceContext_->Release();
    }
}

//
// Override from COperationContext.
//
void STDMETHODCALLTYPE CCopyOperationCallbackContext::Invoke(
    __in IFabricAsyncOperationContext* context
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");

    WriteNoise(
        TraceSubComponentCopyOperationCallback, 
        "this={0} - Invoked with context {1}",
        this,
        context
        );

    if (!context->CompletedSynchronously())
    {
        set_OperationContext(context);
        set_IsCompleted();

        state_->Invoke(this);
        
    WriteNoise(
            TraceSubComponentCopyOperationCallback, 
            "this={0} - Service callback completed",
            this
            );
    }
}

CCopyContextOperationData::CCopyContextOperationData(
    __in IFabricOperationData* operationData
    )
{
    WriteNoise(TraceSubComponentCopyContextOperationData, "this={0} - ctor", this);

    ASSERT_IF(NULL == operationData, "operationData");
    operationData->AddRef();
    operationData_ = operationData;
}

CCopyContextOperationData::~CCopyContextOperationData()
{
    WriteNoise(TraceSubComponentCopyContextOperationData, "this={0} - dtor", this);

    ASSERT_IF(NULL == operationData_, "operationData_");
    operationData_->Release();
    operationData_ = NULL;
}

STDMETHODIMP CCopyContextOperationData::GetData(
    __out ULONG * count,
    __out const FABRIC_OPERATION_DATA_BUFFER ** buffers
    )
{
    HRESULT hr = S_OK;

    ASSERT_IF(NULL == operationData_, "operationData_");
    hr = operationData_->GetData(count, buffers);
    ASSERT_IF(FAILED(hr), "GetData");

    ASSERT_IF(*count < 1, "must have at least extended replica buffer");
    ASSERT_IF(NULL == *buffers, "NULL buffers");

    //
    // Strip off extended replica buffer.
    //
    (*count)--;

    if (0 == (*count))
    {
        *buffers = NULL;
    }
    else
    {
        (*buffers)++;
    }

    return S_OK;
}

//
// Constructor/Destructor.
//
CStatefulCompositeOpenUndoCloseAsyncOperationAsync::CStatefulCompositeOpenUndoCloseAsyncOperationAsync(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId) 
    : CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatefulCompositeOpenUndoCloseOperationAsync, "this={0} - ctor", this);

    openOperationAsync_ = NULL;
    undoOperationAsync_ = NULL;
    closeOperationAsync_ = NULL;
    phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Open;
}

CStatefulCompositeOpenUndoCloseAsyncOperationAsync::~CStatefulCompositeOpenUndoCloseAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentStatefulCompositeOpenUndoCloseOperationAsync, "this={0} - dtor", this);

    openOperationAsync_->Release();
    if (NULL != undoOperationAsync_)
    {
        undoOperationAsync_->Release();
    }
    closeOperationAsync_->Release();
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Begin(void)
{
    ASSERT_IF(NULL == openOperationAsync_, "Unexpected open operation");
    ASSERT_IF(NULL == closeOperationAsync_, "Unexpected close operation");
    ASSERT_IF(CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Open != phase_, "Unexpected phase");

    count_ = 1;
    HRESULT hr = S_OK;

    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulCompositeOpenUndoCloseOperationAsync(
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    hr = openOperationAsync_->Begin();
    if (FAILED(hr) && !openOperationAsync_->HasStartedSuccessfully())
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulCompositeOpenUndoCloseOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            hr
            );

        errorCode_ = hr;
        phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Close;
        count_ = 1;

        ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeOpenUndoCloseOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );

        hr = closeOperationAsync_->Begin();
        if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeOpenUndoCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                hr
                );
            ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulCompositeOpenUndoCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                errorCode_
                );
            ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulCompositeOpenUndoCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );

            phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;

            CBaseAsyncOperation::set_IsCompleted();
    
            ASSERT_IF(NULL == state_, "Unexpected callback state");
            IFabricAsyncOperationCallback* callback = state_;
            state_ = NULL;
            callback->Invoke(this);
            callback->Release();
        }
    }
    return S_OK;
}

STDMETHODIMP CStatefulCompositeOpenUndoCloseAsyncOperationAsync::set_IsCompleted(void)
{
    HRESULT hr = S_OK;
    if (0 == ::InterlockedDecrement64(&count_))
    {
        BOOLEAN fDone = FALSE;
        while (!fDone)
        {
            if (CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Open == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
        
                hr = openOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );
        
                    errorCode_ = hr;
                    phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Close;
                    count_ = 1;

                    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
    
                    hr = closeOperationAsync_->Begin();
                    if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
                    {
                        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_,
                            hr
                            );

                        phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                        continue;
                    }     
                    break;
                }     
                else
                {
                    ServiceGroupStatefulEventSource::GetEvents().Info_5_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
        
                    if (NULL != undoOperationAsync_)
                    {
                        phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Undo;
                        count_ = 1;
    
                        ServiceGroupStatefulEventSource::GetEvents().Info_6_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_
                            );

                        hr = undoOperationAsync_->Begin();
                        if (FAILED(hr) && !undoOperationAsync_->HasStartedSuccessfully())
                        {
                            ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulCompositeOpenUndoCloseOperationAsync(
                                reinterpret_cast<uintptr_t>(this),
                                partitionId_,
                                hr
                                );
                
                            errorCode_ = hr;
                            phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Close;
                            count_ = 1;
                            
                            ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeOpenUndoCloseOperationAsync(
                                reinterpret_cast<uintptr_t>(this),
                                partitionId_
                                );
                            
                            hr = closeOperationAsync_->Begin();
                            if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
                            {
                                ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeOpenUndoCloseOperationAsync(
                                    reinterpret_cast<uintptr_t>(this),
                                    partitionId_,
                                    hr
                                    );

                                phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                                continue;
                            }
                            break;
                        }
                        break;
                    }
                    else
                    {
                        ServiceGroupStatefulEventSource::GetEvents().Info_7_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_
                            );

                        phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                        continue;
                    }
                }
                break;
            }

            if (CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Undo == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_8_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
    
                hr = undoOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );
                    
                    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
                    
                    errorCode_ = hr;
                    phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Close;
                    count_ = 1;
    
                    hr = closeOperationAsync_->Begin();
                    if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
                    {
                        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_,
                            hr
                            );
                     
                        phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                        continue;
                    }
                    break;
                }
                else
                {
                    ServiceGroupStatefulEventSource::GetEvents().Info_9_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
                
                    phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                    continue;
                }
                break;
            }

            if (CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Close == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_10_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
            
                hr = closeOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_5_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );
                }
                else
                {
                    ServiceGroupStatefulEventSource::GetEvents().Info_11_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
                }
            
                phase_ = CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done;
                continue;
            }
        
            if (CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Done == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
            
                CBaseAsyncOperation::set_IsCompleted();
            
                ASSERT_IF(NULL == state_, "Unexpected callback state");
                IFabricAsyncOperationCallback* callback = state_;
                state_ = NULL;
                callback->Invoke(this);
                callback->Release();
            
                break;
            }
        }
    }
    return S_OK;
}

STDMETHODIMP CStatefulCompositeOpenUndoCloseAsyncOperationAsync::End(void)
{
    return CBaseAsyncOperation::End();
}

HRESULT CStatefulCompositeOpenUndoCloseAsyncOperationAsync::Register(
    __in CCompositeAsyncOperationAsync* openOperationAsync,
    __in_opt CCompositeAsyncOperationAsync* undoOperationAsync,
    __in CCompositeAsyncOperationAsync* closeOperationAsync
    )
{
    openOperationAsync_ = openOperationAsync;
    undoOperationAsync_ = undoOperationAsync;
    closeOperationAsync_ = closeOperationAsync;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatefulCompositeOnDataLossUndoAsyncOperationAsync::CStatefulCompositeOnDataLossUndoAsyncOperationAsync(__in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId)
    : CCompositeAsyncOperationAsync(parent, partitionId)
{

    WriteNoise(TraceSubComponentStatefulCompositeDataLossUndoOperationAsync, "this={0} - ctor", this);

    dataLossOperationAsync_ = NULL;
    undoOperationAsync_ = NULL;
    phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::DataLoss;
}

CStatefulCompositeOnDataLossUndoAsyncOperationAsync::~CStatefulCompositeOnDataLossUndoAsyncOperationAsync(void)
{

    WriteNoise(TraceSubComponentStatefulCompositeDataLossUndoOperationAsync, "this={0} - dtor", this);

    dataLossOperationAsync_->Release();
    if (NULL != undoOperationAsync_)
    {

        undoOperationAsync_->Release();
    }
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Begin(void)
{
    ASSERT_IF(NULL == dataLossOperationAsync_, "Unexpected data loss operation");
    ASSERT_IF(CStatefulCompositeOnDataLossUndoAsyncOperationAsync::DataLoss != phase_, "Unexpected phase");

    count_ = 1;
    HRESULT hr = S_OK;

    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulCompositeDataLossUndoOperationAsync(
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    hr = dataLossOperationAsync_->Begin();
    if (FAILED(hr) && !dataLossOperationAsync_->HasStartedSuccessfully())
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulCompositeDataLossUndoOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            hr
            );

        ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulCompositeOpenUndoCloseOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );

        errorCode_ = hr;

        phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;

        CBaseAsyncOperation::set_IsCompleted();

        ASSERT_IF(NULL == state_, "Unexpected callback state");
        IFabricAsyncOperationCallback* callback = state_;
        state_ = NULL;
        callback->Invoke(this);
        callback->Release();
    }
    return S_OK;
}

STDMETHODIMP CStatefulCompositeOnDataLossUndoAsyncOperationAsync::End(void)
{

    return CBaseAsyncOperation::End();
}

STDMETHODIMP CStatefulCompositeOnDataLossUndoAsyncOperationAsync::set_IsCompleted(void)
{

    HRESULT hr = S_OK;
    if (0 == ::InterlockedDecrement64(&count_))
    {
        BOOLEAN fDone = FALSE;
        while (!fDone)
        {
            if (CStatefulCompositeOnDataLossUndoAsyncOperationAsync::DataLoss == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeDataLossUndoOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
        
                hr = dataLossOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeDataLossUndoOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );
        
                    errorCode_ = hr;
                    phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;
                    continue;
                }     
                else
                {
                    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulCompositeDataLossUndoOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );

                    BOOLEAN isStateChanged = FALSE;
                    dataLossOperationAsync_->get_IsStateChanged(&isStateChanged);
                    if (NULL != undoOperationAsync_ && isStateChanged)
                    {
                        phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Undo;
                        count_ = 1;
    
                        ServiceGroupStatefulEventSource::GetEvents().Info_6_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_
                            );

                        hr = undoOperationAsync_->Begin();
                        if (FAILED(hr) && !undoOperationAsync_->HasStartedSuccessfully())
                        {
                            ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulCompositeOpenUndoCloseOperationAsync(
                                reinterpret_cast<uintptr_t>(this),
                                partitionId_,
                                hr
                                );
                
                            errorCode_ = hr;
                            phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;
                            continue;
                        }
                        break;
                    }
                    else
                    {

                        ServiceGroupStatefulEventSource::GetEvents().Info_7_StatefulCompositeOpenUndoCloseOperationAsync(
                            reinterpret_cast<uintptr_t>(this),
                            partitionId_
                            );

                        phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;
                        continue;
                    }

                }
                break;
            }

            if (CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Undo == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_8_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
    
                hr = undoOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );
                    
                    errorCode_ = hr;
                    phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;
                    continue;
                }
                else
                {

                    ServiceGroupStatefulEventSource::GetEvents().Info_9_StatefulCompositeOpenUndoCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_
                        );
                
                    phase_ = CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done;
                    continue;
                }
                break;
            }

            if (CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Done == phase_)
            {

                ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulCompositeOpenUndoCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
            
                CBaseAsyncOperation::set_IsCompleted();
            
                ASSERT_IF(NULL == state_, "Unexpected callback state");
                IFabricAsyncOperationCallback* callback = state_;
                state_ = NULL;
                callback->Invoke(this);
                callback->Release();
            
                break;
            }
        }
    }
    return S_OK;
}

//
// Addtional methods.
//
HRESULT CStatefulCompositeOnDataLossUndoAsyncOperationAsync::Register(
    __in CStatefulCompositeOnDataLossOperation* dataLossOperationAsync,
    __in_opt CCompositeAsyncOperationAsync* undoOperationAsync
    )
{

    dataLossOperationAsync_ = dataLossOperationAsync;
    undoOperationAsync_ = undoOperationAsync;
    return S_OK;
}

STDMETHODIMP CStatefulCompositeOnDataLossUndoAsyncOperationAsync::get_IsStateChanged(__out BOOLEAN* isStateChanged)
{
    return dataLossOperationAsync_->get_IsStateChanged(isStateChanged);
}

//
// Constructor/Destructor.
//
CStatefulCompositeRollbackUpdateEpochOperationAsync::CStatefulCompositeRollbackUpdateEpochOperationAsync(
    __in CStatefulServiceGroup* owner,
    __in_opt CBaseAsyncOperation* parent, 
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_EPOCH const & epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber)
    : CStatefulCompositeRollbackWithPostProcessingOperationAsync(
        owner, 
        parent, 
        partitionId, 
        TraceSubComponentStatefulCompositeRollbackUpdateEpochOperation, // const reference to global constant
        TraceOperationNameUpdateEpoch)                                  // const reference to global constant
    , epoch_(epoch)
    , previousEpochLastSequenceNumber_(previousEpochLastSequenceNumber)
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackUpdateEpochOperation, "this={0} - ctor", this);
}

CStatefulCompositeRollbackUpdateEpochOperationAsync::~CStatefulCompositeRollbackUpdateEpochOperationAsync()
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackUpdateEpochOperation, "this={0} - dtor", this);
}

//
// Cleanup
//
void CStatefulCompositeRollbackUpdateEpochOperationAsync::OnRollbackCompleted()
{
    ASSERT_IF(NULL == owner_, "Unexpected owner");

    owner_->RemoveAtomicGroupsOnUpdateEpoch(epoch_, previousEpochLastSequenceNumber_);
}

//
// Constructor/Destructor.
//
CStatefulCompositeRollbackChangeRoleOperationAsync::CStatefulCompositeRollbackChangeRoleOperationAsync(
    __in CStatefulServiceGroup* owner,
    __in_opt CBaseAsyncOperation* parent, 
    __in FABRIC_PARTITION_ID partitionId
    )
    : CStatefulCompositeRollbackWithPostProcessingOperationAsync(
        owner, 
        parent, 
        partitionId, 
        TraceSubComponentStatefulCompositeRollbackChangeRoleOperation, // const reference to global constant
        TraceOperationNameChangeRole)                                  // const reference to global constant
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackChangeRoleOperation, "this={0} - ctor", this);
}

CStatefulCompositeRollbackChangeRoleOperationAsync::~CStatefulCompositeRollbackChangeRoleOperationAsync()
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackChangeRoleOperation, "this={0} - dtor", this);
}

//
// Additional methods.
//
STDMETHODIMP CStatefulCompositeRollbackChangeRoleOperationAsync::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    ASSERT_IF(NULL == postProcessingOperationAsync_, "Unexpected operation");

    return ((CStatefulCompositeChangeRoleOperation*)postProcessingOperationAsync_)->get_ServiceEndpoint(serviceEndpoint);
}

STDMETHODIMP CStatefulCompositeRollbackChangeRoleOperationAsync::get_ReplicaRole(__out FABRIC_REPLICA_ROLE* newReplicaRole)
{
    ASSERT_IF(NULL == postProcessingOperationAsync_, "Unexpected operation");

    return ((CStatefulCompositeChangeRoleOperation*)postProcessingOperationAsync_)->get_ReplicaRole(newReplicaRole);
}

//
// Cleanup
//
void CStatefulCompositeRollbackChangeRoleOperationAsync::OnRollbackCompleted()
{
    ASSERT_IF(NULL == owner_, "Unexpected owner");

    owner_->RemoveAllAtomicGroups();
}

//
// Constructor/Destructor.
//
CStatefulCompositeRollbackCloseOperationAsync::CStatefulCompositeRollbackCloseOperationAsync(
    __in CStatefulServiceGroup* owner,
    __in_opt CBaseAsyncOperation* parent, 
    __in FABRIC_PARTITION_ID partitionId)
    : CStatefulCompositeRollbackWithPostProcessingOperationAsync(
        owner, 
        parent, 
        partitionId, 
        TraceSubComponentStatefulCompositeRollbackCloseOperation, // const reference to global constant
        TraceOperationNameClose)                                  // const reference to global constant
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackCloseOperation, "this={0} - ctor", this);
}

CStatefulCompositeRollbackCloseOperationAsync::~CStatefulCompositeRollbackCloseOperationAsync()
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackCloseOperation, "this={0} - dtor", this);
}

//
// Cleanup
//
void CStatefulCompositeRollbackCloseOperationAsync::OnRollbackCompleted()
{
    ASSERT_IF(NULL == owner_, "Unexpected owner");

    owner_->RemoveAllAtomicGroups();
}

//
// Constructor/Destructor.
//
CStatefulCompositeRollbackWithPostProcessingOperationAsync::CStatefulCompositeRollbackWithPostProcessingOperationAsync(
    __in CStatefulServiceGroup* owner,
    __in_opt CBaseAsyncOperation* parent, 
    __in FABRIC_PARTITION_ID partitionId,
    __in Common::StringLiteral const & traceType,
    __in std::wstring const & operationName)
    : CCompositeAsyncOperationAsync(parent, partitionId)
    , traceType_(traceType)
    , operationName_(operationName)
{
    ASSERT_IF(owner == NULL, "Unexpected owner");

    WriteNoise(TraceSubComponentStatefulCompositeRollbackWithPostProcessingOperation, "this={0} - ctor", this);

    owner_ = owner;
    owner_->AddRef();

    postProcessingOperationAsync_ = NULL;
    rollbackOperationAsync_ = NULL;
    phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::Rollback;
}

CStatefulCompositeRollbackWithPostProcessingOperationAsync::~CStatefulCompositeRollbackWithPostProcessingOperationAsync(void)
{
    WriteNoise(TraceSubComponentStatefulCompositeRollbackWithPostProcessingOperation, "this={0} - dtor", this);

    if (NULL != rollbackOperationAsync_)
    {
        rollbackOperationAsync_->Release();
    }

    owner_->Release();
    postProcessingOperationAsync_->Release();
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CStatefulCompositeRollbackWithPostProcessingOperationAsync::Begin(void)
{
    ASSERT_IF(CStatefulCompositeRollbackWithPostProcessingOperationAsync::Rollback != phase_, "Unexpected phase");

    if (NULL != rollbackOperationAsync_)
    {
        return BeginRollback();
    }
    else
    {
        //
        // nothing to rollback, go to other operation
        //
        phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::PostProcessing;
        return BeginPostProcessing();
    }
}

HRESULT CStatefulCompositeRollbackWithPostProcessingOperationAsync::BeginRollback(void)
{
    ASSERT_IF(NULL == rollbackOperationAsync_, "Unexpected rollback operation");
    ASSERT_IF(CStatefulCompositeRollbackWithPostProcessingOperationAsync::Rollback != phase_, "Unexpected phase");

    count_ = 1;

    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulCompositeRollback(
        traceType_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    //
    // Begin must not fail; if it does, CAtomicGroupCommitRollbackOperationAsync will assert
    //
    return rollbackOperationAsync_->Begin();
}

HRESULT CStatefulCompositeRollbackWithPostProcessingOperationAsync::BeginPostProcessing(void)
{
    ASSERT_IF(NULL == postProcessingOperationAsync_, "Unexpected post proccesing operation");
    ASSERT_IF(CStatefulCompositeRollbackWithPostProcessingOperationAsync::PostProcessing != phase_, "Unexpected phase");

    HRESULT hr = S_OK;

    count_ = 1;

    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeRollback(
        traceType_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        operationName_
        );

    hr = postProcessingOperationAsync_->Begin();
    if (FAILED(hr) && !postProcessingOperationAsync_->HasStartedSuccessfully())
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulCompositeRollback(
            traceType_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            operationName_,
            hr
            );

        WriteNoise(
            traceType_, 
            "this={0} partitionId={1} - Invoking callback",
            this,
            partitionId_
            );

        phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::Done;

        errorCode_ = hr;

        CBaseAsyncOperation::set_IsCompleted();

        ASSERT_IF(NULL == state_, "Unexpected callback state");
        IFabricAsyncOperationCallback* callback = state_;
        state_ = NULL;
        callback->Invoke(this);
        callback->Release();
    }
    return S_OK;
}

STDMETHODIMP CStatefulCompositeRollbackWithPostProcessingOperationAsync::End(void)
{
    return CBaseAsyncOperation::End();
}

STDMETHODIMP CStatefulCompositeRollbackWithPostProcessingOperationAsync::set_IsCompleted(void)
{
    HRESULT hr = S_OK;
    BOOLEAN fDone = FALSE;

    if (0 == ::InterlockedDecrement64(&count_))
    {
        while (!fDone)
        {
            if (CStatefulCompositeRollbackWithPostProcessingOperationAsync::Rollback == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulCompositeRollback(
                    traceType_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );

                //
                // End must not fail; if it does, CAtomicGroupCommitRollbackOperationAsync will assert
                //
                hr = rollbackOperationAsync_->End();
                ASSERT_IF(FAILED(hr), "Unexpected error");

                ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulCompositeRollback(
                    traceType_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );

                OnRollbackCompleted();

                phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::PostProcessing;

                return BeginPostProcessing();
                break;
            }

            if (CStatefulCompositeRollbackWithPostProcessingOperationAsync::PostProcessing == phase_)
            {
                ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulCompositeRollback(
                    traceType_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    operationName_
                    );

                hr = postProcessingOperationAsync_->End();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeRollback(
                        traceType_,
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        operationName_,
                        hr
                        );

                    errorCode_ = hr;
                    phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::Done;
                    continue;
                }     
                else
                {
                    ServiceGroupStatefulEventSource::GetEvents().Info_5_StatefulCompositeRollback(
                        traceType_,
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        operationName_
                        );

                    phase_ = CStatefulCompositeRollbackWithPostProcessingOperationAsync::Done;
                    continue;
                }
                break;
            }

            if (CStatefulCompositeRollbackWithPostProcessingOperationAsync::Done == phase_)
            {
                WriteNoise(
                    traceType_, 
                    "this={0} partitionId={1} - Invoking callback",
                    this,
                    partitionId_
                    );

                CBaseAsyncOperation::set_IsCompleted();

                ASSERT_IF(NULL == state_, "Unexpected callback state");
                IFabricAsyncOperationCallback* callback = state_;
                state_ = NULL;
                callback->Invoke(this);
                callback->Release();

                break;
            }
        }
    }

    return S_OK;
}

//
// Addtional methods.
//
HRESULT CStatefulCompositeRollbackWithPostProcessingOperationAsync::Register(
    __in_opt CCompositeAsyncOperationAsync* rollbackOperationAsync,
    __in CCompositeAsyncOperationAsync* postProcessingOperationAsync
    )
{
    rollbackOperationAsync_ = rollbackOperationAsync;
    postProcessingOperationAsync_ = postProcessingOperationAsync;
    return S_OK;
}

//
// Constructor/Destructor.
//
CStatelessCompositeOpenCloseAsyncOperationAsync::CStatelessCompositeOpenCloseAsyncOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
    __in FABRIC_PARTITION_ID partitionId
    ) : CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatelessCompositeOpenCloseOperationAsync, "this={0} - ctor", this);

    openOperationAsync_ = NULL;
    closeOperationAsync_ = NULL;
    phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Open;
}

CStatelessCompositeOpenCloseAsyncOperationAsync::~CStatelessCompositeOpenCloseAsyncOperationAsync(void)
{
    WriteNoise(TraceSubComponentStatelessCompositeOpenCloseOperationAsync, "this={0} - dtor", this);

    openOperationAsync_->Release();
    closeOperationAsync_->Release();
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CStatelessCompositeOpenCloseAsyncOperationAsync::Begin(void)
{
    ASSERT_IF(NULL == openOperationAsync_, "Unexpected open operation");
    ASSERT_IF(NULL == closeOperationAsync_, "Unexpected close operation");
    ASSERT_IF(CStatelessCompositeOpenCloseAsyncOperationAsync::Open != phase_, "Unexpected phase");

    count_ = 1;
    HRESULT hr = S_OK;

    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessCompositeOpenCloseOperationAsync(
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    hr = openOperationAsync_->Begin();
    if (FAILED(hr) && !openOperationAsync_->HasStartedSuccessfully())
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessCompositeOpenCloseOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            hr
            );

        errorCode_ = hr;
        phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Close;
        count_ = 1;

        ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessCompositeOpenCloseOperationAsync(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );

        hr = closeOperationAsync_->Begin();
        if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                hr
                );
            ServiceGroupStatelessEventSource::GetEvents().Info_2_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                errorCode_
                );
            ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );

            phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Done;

            CBaseAsyncOperation::set_IsCompleted();
    
            ASSERT_IF(NULL == state_, "Unexpected callback state");
            IFabricAsyncOperationCallback* callback = state_;
            state_ = NULL;
            callback->Invoke(this);
            callback->Release();
        }
    }
    return S_OK;
}

STDMETHODIMP CStatelessCompositeOpenCloseAsyncOperationAsync::End(void)
{
    return CBaseAsyncOperation::End();
}

STDMETHODIMP CStatelessCompositeOpenCloseAsyncOperationAsync::set_IsCompleted(void)
{
    HRESULT hr = S_OK;
    BOOLEAN fDone = FALSE;
    while (!fDone)
    {
        if (CStatelessCompositeOpenCloseAsyncOperationAsync::Open == phase_)
        {
            ServiceGroupStatelessEventSource::GetEvents().Info_4_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );
    
            hr = openOperationAsync_->End();
            if (FAILED(hr))
            {
                ServiceGroupStatelessEventSource::GetEvents().Error_2_StatelessCompositeOpenCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    hr
                    );
    
                errorCode_ = hr;
                phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Close;
                count_ = 1;

                ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessCompositeOpenCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );

                hr = closeOperationAsync_->Begin();
                if (FAILED(hr) && !closeOperationAsync_->HasStartedSuccessfully())
                {
                    ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessCompositeOpenCloseOperationAsync(
                        reinterpret_cast<uintptr_t>(this),
                        partitionId_,
                        hr
                        );

                    phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Done;
                    continue;
                }     
                break;
            }     
            else
            {
                ServiceGroupStatelessEventSource::GetEvents().Info_5_StatelessCompositeOpenCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );

                phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Done;
                continue;
            }
            break;
        }

        if (CStatelessCompositeOpenCloseAsyncOperationAsync::Close == phase_)
        {
            ServiceGroupStatelessEventSource::GetEvents().Info_6_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );
        
            hr = closeOperationAsync_->End();
            if (FAILED(hr))
            {
                ServiceGroupStatelessEventSource::GetEvents().Error_3_StatelessCompositeOpenCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    hr
                    );
            }
            else
            {
                ServiceGroupStatelessEventSource::GetEvents().Info_7_StatelessCompositeOpenCloseOperationAsync(
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_
                    );
            }
        
            phase_ = CStatelessCompositeOpenCloseAsyncOperationAsync::Done;
            continue;
        }
        
        if (CStatelessCompositeOpenCloseAsyncOperationAsync::Done == phase_)
        {
            ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessCompositeOpenCloseOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );
        
            CBaseAsyncOperation::set_IsCompleted();
        
            ASSERT_IF(NULL == state_, "Unexpected callback state");
            IFabricAsyncOperationCallback* callback = state_;
            state_ = NULL;
            callback->Invoke(this);
            callback->Release();
        
            break;
        }
    }
    return S_OK;
}

//
// Addtional methods.
//
HRESULT CStatelessCompositeOpenCloseAsyncOperationAsync::get_ServiceEndpoint(__out IFabricStringResult** serviceEndpoint)
{
    return openOperationAsync_->get_ServiceEndpoint(serviceEndpoint);
}

HRESULT CStatelessCompositeOpenCloseAsyncOperationAsync::Register(
    __in CStatelessCompositeOpenOperation* openOperationAsync,
    __in CCompositeAsyncOperationAsync* closeOperationAsync
    )
{
    openOperationAsync_ = openOperationAsync;
    closeOperationAsync_ = closeOperationAsync;
    return S_OK;
}

//
// Constructor/Destructor.
//
CAtomicGroupCommitRollbackOperationAsync::CAtomicGroupCommitRollbackOperationAsync(
    __in_opt CBaseAsyncOperation* parent,
    __in IFabricAtomicGroupStateProvider* atomicGroupStateProvider,
    __in BOOLEAN isCommit,
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber
    ) : CStateProviderAsyncOperationAsync(parent, NULL)
{

    WriteNoise(TraceSubComponentSingleAtomicGroupCommitRollbackOperationAsync, "this={0} - ctor", this);

    if (NULL != atomicGroupStateProvider)
    {
        atomicGroupStateProvider->AddRef();
    }
    atomicGroupStateProvider_ = atomicGroupStateProvider;
    isCommit_ = isCommit;
    atomicGroupId_ = atomicGroupId;
    sequenceNumber_ = sequenceNumber;
}

CAtomicGroupCommitRollbackOperationAsync::~CAtomicGroupCommitRollbackOperationAsync(void)
{
    WriteNoise(TraceSubComponentSingleAtomicGroupCommitRollbackOperationAsync, "this={0} - dtor", this);

    if (NULL != atomicGroupStateProvider_)
    {
        atomicGroupStateProvider_->Release();
    }
}

//
// Overrides from CBaseAsyncOperation.
//
STDMETHODIMP CAtomicGroupCommitRollbackOperationAsync::Begin(void)
{
    ASSERT_IF(NULL == atomicGroupStateProvider_, "Unexpected atomic group state provider");

    IFabricAsyncOperationContext* context = NULL;
    if (isCommit_)
    {
        errorCode_ = atomicGroupStateProvider_->BeginAtomicGroupCommit(atomicGroupId_, sequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    }
    else
    {
        errorCode_ = atomicGroupStateProvider_->BeginAtomicGroupRollback(atomicGroupId_, sequenceNumber_, (IFabricAsyncOperationCallback*)this, &context);
    }
    ASSERT_IF(FAILED(errorCode_), SERVICE_REPLICA_DOWN);

    startedSuccessfully_ = TRUE;

    ASSERT_IF(NULL == context, "Unexpected context");
    HRESULT hr = S_OK;
    if (context->CompletedSynchronously())
    {
        hr = Complete(context);
    }
    context->Release();
    return hr;
}

//
// Overrides from CSingleAsyncOperation.
//
STDMETHODIMP CAtomicGroupCommitRollbackOperationAsync::Complete(__in_opt IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");
    if (isCommit_)
    {
        errorCode_ = atomicGroupStateProvider_->EndAtomicGroupCommit(context);
    }
    else
    {
        errorCode_ = atomicGroupStateProvider_->EndAtomicGroupRollback(context);
    }
    ASSERT_IF(FAILED(errorCode_), SERVICE_REPLICA_DOWN);
    return CStateProviderAsyncOperationAsync::Complete(context);
}

//
// Constructor/Destructor.
//
CCompositeAtomicGroupCommitRollbackOperationAsync::CCompositeAtomicGroupCommitRollbackOperationAsync(
    __in CStatefulServiceGroup* owner,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in BOOLEAN isCommit
    ) : CCompositeAsyncOperationAsync(NULL, partitionId)
{
    WriteNoise(TraceSubComponentCompositeAtomicGroupCommitRollbackOperationAsync, "this={0} - ctor", this);
    ASSERT_IF(NULL == owner, "Unexpected owner");

    owner_ = owner;
    owner_->AddRef();

    atomicGroupId_ = atomicGroupId;
    isCommit_ = isCommit;
}

CCompositeAtomicGroupCommitRollbackOperationAsync::~CCompositeAtomicGroupCommitRollbackOperationAsync(void)
{
    WriteNoise(TraceSubComponentCompositeAtomicGroupCommitRollbackOperationAsync, "this={0} - dtor", this);

    owner_->Release();
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CCompositeAtomicGroupCommitRollbackOperationAsync::Begin(void)
{
    count_ = asyncOperations_.size();
    ASSERT_IF(0 == count_, "Unexpected number of child operations");

    ServiceGroupReplicationEventSource::GetEvents().Info_0_CompositeAtomicGroupCommitRollbackOperationAsync(
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    HRESULT hr = E_FAIL;
    BaseAsyncOperation_Iterator iterateEach;
    AddRef();
    for (iterateEach = asyncOperations_.begin(); asyncOperations_.end() != iterateEach; iterateEach++)
    {
        hr = (*iterateEach)->Begin();
    }
    Release();
    return hr;
}
    
STDMETHODIMP CCompositeAtomicGroupCommitRollbackOperationAsync::set_IsCompleted(void)
{
    if (0 == ::InterlockedDecrement64(&count_))
    {
        if (isCommit_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_1_CompositeAtomicGroupCommitRollbackOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                atomicGroupId_
                );
        }
        else
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_2_CompositeAtomicGroupCommitRollbackOperationAsync(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                atomicGroupId_
                );
        }

        owner_->RemoveAtomicGroupOnCommitRollback(atomicGroupId_);

        Release();
    } 
    return S_OK;
}

