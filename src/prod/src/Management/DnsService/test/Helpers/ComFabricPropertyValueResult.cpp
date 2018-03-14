// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComFabricPropertyValueResult.h"
#include "DnsPropertyValue.h"

/*static*/
void ComFabricPropertyValueResult::Create(
    __out ComFabricPropertyValueResult::SPtr& spResult,
    __in KAllocator& allocator,
    __in KBuffer& buffer
)
{
    spResult = _new(TAG, allocator) ComFabricPropertyValueResult(buffer);
    KInvariant(spResult != nullptr);
}

ComFabricPropertyValueResult::ComFabricPropertyValueResult(
    __in KBuffer& buffer
)
{
    _spData = &buffer;
}

ComFabricPropertyValueResult::~ComFabricPropertyValueResult()
{
}

HRESULT ComFabricPropertyValueResult::GetValueAsWString(
    /* [retval][out] */ LPCWSTR*)
{
    return E_NOTIMPL;
}

const FABRIC_NAMED_PROPERTY* ComFabricPropertyValueResult::get_Property(void)
{
    return nullptr;
}

HRESULT ComFabricPropertyValueResult::GetValueAsBinary(
    /* [out] */ ULONG * pSize,
    /* [retval][out] */ const BYTE** ppValue)
{
    *pSize = _spData->QuerySize();
    *ppValue = (BYTE*)_spData->GetBuffer();
    return S_OK;
}

HRESULT ComFabricPropertyValueResult::GetValueAsInt64(
    /* [retval][out] */ LONGLONG*)
{
    return E_NOTIMPL;
}

HRESULT ComFabricPropertyValueResult::GetValueAsDouble(
    /* [retval][out] */ double*)
{
    return E_NOTIMPL;
}

HRESULT ComFabricPropertyValueResult::GetValueAsGuid(
    /* [retval][out] */ GUID*)
{
    return E_NOTIMPL;
}
