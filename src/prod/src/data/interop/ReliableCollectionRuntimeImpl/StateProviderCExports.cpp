// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::Utilities;

extern "C" HRESULT StateProvider_GetInfo(
    __in StateProviderHandle stateProviderHandle,
    __in LPCWSTR lang,
    __inout StateProvider_Info* stateProvider_Info)
{
    NTSTATUS status;

    if (lang == nullptr)
        return E_INVALIDARG;
    if (stateProviderHandle == nullptr)
        return E_INVALIDARG;
    if (stateProvider_Info == nullptr)
        return E_INVALIDARG;

    if (stateProvider_Info->Size < StateProvider_Info_V1_Size)
        return E_INVALIDARG;

    IStateProvider2* stateProviderPtr = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    Data::Interop::StateProviderInfo stateProviderInfo;

    status = Data::Interop::StateProviderInfo::FromStateProvider(stateProviderPtr, lang, stateProviderInfo);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    status = stateProviderInfo.ToPublicApi(*stateProvider_Info);
    return StatusConverter::ToHResult(status);
}

extern "C" void StateProvider_AddRef(
    __in StateProviderHandle stateProviderHandle)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStateProvider2::SPtr stateProviderSPtr(stateProvider);
    stateProviderSPtr.Detach();
}

extern "C" void StateProvider_Release(
    __in StateProviderHandle stateProviderHandle)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStateProvider2::SPtr stateProviderSPtr;
    stateProviderSPtr.Attach(stateProvider);
}
