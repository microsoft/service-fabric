// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComPropertyManager.h"

/*static*/
void ComPropertyManager::Create(
    __out ComPropertyManager::SPtr& spManager,
    __in KAllocator& allocator
)
{
    spManager = _new(TAG, allocator) ComPropertyManager();
    KInvariant(spManager != nullptr);
}

ComPropertyManager::ComPropertyManager()
{
}

ComPropertyManager::~ComPropertyManager()
{
}

HRESULT ComPropertyManager::BeginGetProperty(
    __in FABRIC_URI name,
    __in LPCWSTR propertyName,
    __in DWORD timeoutMilliseconds,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(propertyName);
    UNREFERENCED_PARAMETER(timeoutMilliseconds);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);

    callback->Invoke(nullptr);

    return S_OK;
}

HRESULT ComPropertyManager::EndGetProperty(
    __in IFabricAsyncOperationContext * context,
    __out IFabricPropertyValueResult ** result)
{
    UNREFERENCED_PARAMETER(context);

    if (_spResult != nullptr)
    {
        ComPointer<IFabricPropertyValueResult> sp = _spResult.RawPtr();
        *result = sp.Detach();
        return S_OK;
    }

    return E_FAIL;
}

HRESULT ComPropertyManager::BeginCreateName(
    __in FABRIC_URI,
    __in DWORD,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext **)
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndCreateName(
    __in IFabricAsyncOperationContext *)
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginDeleteName(
    __in FABRIC_URI ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndDeleteName(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginNameExists(
    __in FABRIC_URI ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndNameExists(
    __in IFabricAsyncOperationContext *,
    __out BOOLEAN * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginEnumerateSubNames(
    __in FABRIC_URI ,
    __in IFabricNameEnumerationResult * ,
    __in BOOLEAN,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndEnumerateSubNames(
    __in IFabricAsyncOperationContext * ,
    __out IFabricNameEnumerationResult ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginPutPropertyBinary(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in ULONG ,
    __in_bcount(dataLength) const BYTE * ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndPutPropertyBinary(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginPutPropertyInt64(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in LONGLONG ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndPutPropertyInt64(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginPutPropertyDouble(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in double ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndPutPropertyDouble(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginPutPropertyWString(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in LPCWSTR ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndPutPropertyWString(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginPutPropertyGuid(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in const GUID * ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndPutPropertyGuid(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginDeleteProperty(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndDeleteProperty(
    __in IFabricAsyncOperationContext * )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginGetPropertyMetadata(
    __in FABRIC_URI ,
    __in LPCWSTR ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndGetPropertyMetadata(
    __in IFabricAsyncOperationContext * ,
    __out IFabricPropertyMetadataResult ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginSubmitPropertyBatch(
    __in FABRIC_URI ,
    __in ULONG ,
    __in_ecount(operationCount) const FABRIC_PROPERTY_BATCH_OPERATION * ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndSubmitPropertyBatch(
    __in IFabricAsyncOperationContext * ,
    __out ULONG * ,
    __out IFabricPropertyBatchResult ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::BeginEnumerateProperties(
    __in FABRIC_URI ,
    __in BOOLEAN ,
    __in IFabricPropertyEnumerationResult * ,
    __in DWORD ,
    __in IFabricAsyncOperationCallback * ,
    __out IFabricAsyncOperationContext ** )
{
    return E_NOTIMPL;
}

HRESULT ComPropertyManager::EndEnumerateProperties(
    __in IFabricAsyncOperationContext * ,
    __out IFabricPropertyEnumerationResult ** )
{
    return E_NOTIMPL;
}
