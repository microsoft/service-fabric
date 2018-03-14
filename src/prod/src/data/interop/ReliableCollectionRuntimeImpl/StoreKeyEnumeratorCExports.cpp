// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

extern "C" void StoreKeyEnumerator_Release(
    StoreKeyEnumeratorHandle enumerator)
{
    IEnumerator<KString::SPtr>::SPtr enumeratorSPtr;
    enumeratorSPtr.Attach((IEnumerator<KString::SPtr>*)enumerator);
}

extern "C" void StoreKeyEnumerator_AddRef(
    StoreKeyEnumeratorHandle enumerator)
{
    IEnumerator<KString::SPtr>::SPtr((IEnumerator<KString::SPtr>*)enumerator).Detach();
}

extern "C" HRESULT StoreKeyEnumerator_MoveNext(
    __in StoreKeyEnumeratorHandle enumeratorHandle,
    __out BOOL* advanced,
    __out LPCWSTR* key)
{
    NTSTATUS status = STATUS_SUCCESS;
    IEnumerator<KString::SPtr>* enumerator = (IEnumerator<KString::SPtr>*)enumeratorHandle;

    EXCEPTION_TO_STATUS(*advanced = enumerator->MoveNext(), status);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    if (key != nullptr && *advanced)
    {
        KString::SPtr current = enumerator->Current();
        // return value pointed by SPtr
        // Since we have a snapshot it is guranteed that memory pointed by SPtr will not be released
        *key = static_cast<LPCWSTR>(*current);
    }

    return StatusConverter::ToHResult(status);
}
