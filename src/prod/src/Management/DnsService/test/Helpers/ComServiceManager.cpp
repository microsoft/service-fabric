// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComServiceManager.h"

/*static*/
void ComServiceManager::Create(
    __out ComServiceManager::SPtr& spManager,
    __in KAllocator& allocator
)
{
    spManager = _new(TAG, allocator) ComServiceManager();
    KInvariant(spManager != nullptr);
}

ComServiceManager::ComServiceManager() :
    _htResults(64, K_DefaultHashFunction, GetThisAllocator())
{
}

ComServiceManager::~ComServiceManager()
{
}

void ComServiceManager::AddResult(
    __in LPCWSTR wsz,
    __in IFabricResolvedServicePartitionResult& result
)
{
    KWString str(GetThisAllocator(), wsz);
    ComPointer<IFabricResolvedServicePartitionResult> spResult = &result;
    _htResults.Put(str, spResult, TRUE);
}

HRESULT ComServiceManager::BeginCreateService(
    /* [in] */ const FABRIC_SERVICE_DESCRIPTION *,
    /* [in] */ DWORD,
    /* [in] */ IFabricAsyncOperationCallback *,
    /* [retval][out] */ IFabricAsyncOperationContext **)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::EndCreateService(
    /* [in] */ IFabricAsyncOperationContext *)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::BeginCreateServiceFromTemplate(
    /* [in] */ FABRIC_URI,
    /* [in] */ FABRIC_URI,
    /* [in] */ LPCWSTR,
    /* [in] */ ULONG,
    /* [size_is][in] */ BYTE *,
    /* [in] */ DWORD,
    /* [in] */ IFabricAsyncOperationCallback *,
    /* [retval][out] */ IFabricAsyncOperationContext **)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::EndCreateServiceFromTemplate(
    /* [in] */ IFabricAsyncOperationContext *)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::BeginDeleteService(
    /* [in] */ FABRIC_URI,
    /* [in] */ DWORD,
    /* [in] */ IFabricAsyncOperationCallback *,
    /* [retval][out] */ IFabricAsyncOperationContext **)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::EndDeleteService(
    /* [in] */ IFabricAsyncOperationContext *)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::BeginGetServiceDescription(
    /* [in] */ FABRIC_URI,
    /* [in] */ DWORD,
    /* [in] */ IFabricAsyncOperationCallback *,
    /* [retval][out] */ IFabricAsyncOperationContext **)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::EndGetServiceDescription(
    /* [in] */ IFabricAsyncOperationContext *,
    /* [retval][out] */ IFabricServiceDescriptionResult **)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::RegisterServicePartitionResolutionChangeHandler(
    /* [in] */ FABRIC_URI,
    /* [in] */ FABRIC_PARTITION_KEY_TYPE,
    /* [in] */ const void *,
    /* [in] */ IFabricServicePartitionResolutionChangeHandler *,
    /* [retval][out] */ LONGLONG *)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::UnregisterServicePartitionResolutionChangeHandler(
    /* [in] */ LONGLONG)
{
    return E_NOTIMPL;
}

HRESULT ComServiceManager::BeginResolveServicePartition(
    /* [in] */ FABRIC_URI name,
    /* [in] */ FABRIC_PARTITION_KEY_TYPE partitionKeyType,
    /* [in] */ const void *partitionKey,
    /* [in] */ IFabricResolvedServicePartitionResult *previousResult,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(timeoutMilliseconds);
    UNREFERENCED_PARAMETER(previousResult);
    UNREFERENCED_PARAMETER(partitionKey);
    UNREFERENCED_PARAMETER(partitionKeyType);
    UNREFERENCED_PARAMETER(name);
   IFabricAsyncOperationContext* pContext = (IFabricAsyncOperationContext*)(name);
    callback->Invoke(pContext);
    return S_OK;
}

HRESULT ComServiceManager::EndResolveServicePartition(
    /* [in] */ IFabricAsyncOperationContext* pContext,
    /* [retval][out] */ IFabricResolvedServicePartitionResult **result)
{
    LPCWSTR wsz = (LPCWSTR)pContext;
    KWString str(GetThisAllocator(), wsz);
    ComPointer<IFabricResolvedServicePartitionResult> spResult;
    _htResults.Get(str, /*out*/spResult);

    ComPointer<IFabricResolvedServicePartitionResult> spOut = spResult;
    *result = spOut.Detach();

    return (spResult.RawPtr() != nullptr) ? S_OK : E_FAIL;
}
