// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::TStore;
using namespace Data::Utilities;
using namespace Data::Interop;

extern "C" HRESULT StateProviderEnumerator_MoveNext(
    __in StateProviderEnumeratorHandle enumeratorHandle,
    __out BOOL* advanced,
    __out LPCWSTR* providerName,
    __out StateProviderHandle* provider)
{
    NTSTATUS status = STATUS_SUCCESS;
    IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>* enumerator = (IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>*)enumeratorHandle;
    EXCEPTION_TO_STATUS(*advanced = enumerator->MoveNext(), status);
    if (!NT_SUCCESS(status) || !(*advanced))
    {
        return StatusConverter::ToHResult(status);
    }
    KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>> kvPair = enumerator->Current();
    KUri::CSPtr uri = kvPair.Key;
    TxnReplicator::IStateProvider2::SPtr stateProviderSPtr = kvPair.Value;

    // Is this is wrapper state provider for compat distributed dictionary state provider
    if (dynamic_cast<CompatRDStateProvider*>(stateProviderSPtr.RawPtr()) != nullptr)
    {
        CompatRDStateProvider* pCompatRDStateProvider = dynamic_cast<CompatRDStateProvider*>(stateProviderSPtr.RawPtr());
        // Get actual data store state provider
        stateProviderSPtr = dynamic_cast<TxnReplicator::IStateProvider2*>(pCompatRDStateProvider->DataStore.RawPtr());
    }

    *providerName = static_cast<LPCWSTR>((KUriView)*uri);
    *provider = stateProviderSPtr.Detach();
    return StatusConverter::ToHResult(status);
}

extern "C" void StateProviderEnumerator_Release(
    __in StateProviderEnumeratorHandle enumerator)
{
    IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>::SPtr enumeratorSPtr;
    enumeratorSPtr.Attach((IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>*)enumerator);
}

extern "C" void StateProviderEnumerator_AddRef(
    __in StateProviderEnumeratorHandle enumerator)
{
    IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>::SPtr((IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>*)enumerator).Detach();
}
