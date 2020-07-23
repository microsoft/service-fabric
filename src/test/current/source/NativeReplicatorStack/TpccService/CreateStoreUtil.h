// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    struct CreateStoreUtil
    {
        template <class TKey,
                  class TVal,
                  class KeyComparer,
                  class KeySerializer,
                  class ValSerializer,
                  class KeyHashFunc>
        static void CreateStore(
            __in FABRIC_REPLICA_ID replicaId,
            __in KeyHashFunc keyHashFunc,
            __in KUriView& stateProviderName,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in KAllocator& allocator,
            __out TxnReplicator::IStateProvider2::SPtr& stateProvider);
    };

    template <class TKey,
              class TVal,
              class KeyComparer,
              class KeySerializer,
              class ValSerializer,
              class KeyHashFunc>
    void CreateStoreUtil::CreateStore(
        __in FABRIC_REPLICA_ID replicaId,
        __in KeyHashFunc keyHashFunc,
        __in KUriView& stateProviderName,
        __in FABRIC_STATE_PROVIDER_ID stateProviderId,
        __in KAllocator& allocator,
        __out TxnReplicator::IStateProvider2::SPtr& stateProvider)
    {
        KSharedPtr<KeyComparer> comparerSPtr = nullptr;
        NTSTATUS status = KeyComparer::Create(allocator, comparerSPtr);
        KInvariant(NT_SUCCESS(status));

        KSharedPtr<KeySerializer> keyStateSerializer = nullptr;
        KSharedPtr<ValSerializer> valStateSerializer = nullptr;

        status = KeySerializer::Create(allocator, keyStateSerializer);
        KInvariant(NT_SUCCESS(status));
        status = ValSerializer::Create(allocator, valStateSerializer);
        KInvariant(NT_SUCCESS(status));

        KSharedPtr<Data::TStore::Store<TKey, TVal>> storeSPtr = nullptr;

        KGuid guid;
        guid.CreateNew();

        status = Data::TStore::Store<TKey, TVal>::Create(
            *PartitionedReplicaId::Create(guid, replicaId, allocator),
            *comparerSPtr,
            keyHashFunc,
            allocator,
            stateProviderName,
            stateProviderId,
            *keyStateSerializer,
            *valStateSerializer,
            storeSPtr);
        KInvariant(NT_SUCCESS(status));

        // Enable enumeration when ReadIsolationLevel = Repeatable
        storeSPtr->EnableEnumerationWithRepeatableRead = true;

        stateProvider = storeSPtr.RawPtr();
    }
}
