// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Store.h"

using namespace ktl;
using namespace Data;
using namespace Data::TStore;
using namespace Data::Utilities;

NTSTATUS Factory::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in IComparer<KString::SPtr>& keyComparer,
    __in KDelegate<ULONG(const KString::SPtr&Key)> func,
    __in KAllocator & allocator, 
    __in KUriView & name,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in Data::StateManager::IStateSerializer<KString::SPtr>& keySerializer,
    __in Data::StateManager::IStateSerializer<KBuffer::SPtr>& valueSerializer,
    __out IStore<KString::SPtr, KBuffer::SPtr>::SPtr& result)
{
    Store<KString::SPtr, KBuffer::SPtr>::SPtr storeSPtr = nullptr;
    NTSTATUS status = Store<KString::SPtr, KBuffer::SPtr>::Create(traceId, keyComparer, func, allocator, name, stateProviderId, keySerializer, valueSerializer, storeSPtr);
    Diagnostics::Validate(status);
    result = IStore<KString::SPtr, KBuffer::SPtr>::SPtr(storeSPtr.RawPtr());

    return status;
}
