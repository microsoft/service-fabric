// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"

class DictionaryChangeHandler 
    : public KObject<DictionaryChangeHandler>
    , public KShared<DictionaryChangeHandler>
    , public Data::TStore::IDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr>
{
    K_FORCE_SHARED(DictionaryChangeHandler);
    K_SHARED_INTERFACE_IMP(IDictionaryChangeHandler);

public:
    using KeyType = KString::SPtr;
    using ValueType = KBuffer::SPtr;

    static HRESULT Create(
        __in KAllocator& allocator,
        __in TxnReplicator::IStateProvider2 *stateProvider,
        __in fnNotifyStoreChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx,
        __out DictionaryChangeHandler::SPtr& result);

public :
    //
    // IDictionaryChangeHandler methods
    //
    ktl::Awaitable<void> OnAddedAsync(
        __in TxnReplicator::TransactionBase const& replicatorTransaction,
        __in KeyType key,
        __in ValueType value,
        __in LONG64 sequenceNumber,
        __in bool isPrimary) noexcept;

    ktl::Awaitable<void> OnUpdatedAsync(
        __in TxnReplicator::TransactionBase const& replicatorTransaction,
        __in KeyType key,
        __in ValueType value,
        __in LONG64 sequenceNumber,
        __in bool isPrimary) noexcept;

    ktl::Awaitable<void> OnRemovedAsync(
        __in TxnReplicator::TransactionBase const& replicatorTransaction,
        __in KeyType key,
        __in LONG64 sequenceNumber,
        __in bool isPrimary) noexcept;

    ktl::Awaitable<void> OnRebuiltAsync(__in Data::Utilities::IAsyncEnumerator<Data::KeyValuePair<KeyType, Data::KeyValuePair<LONG64, ValueType>>> & enumerableState) noexcept;

private:
    DictionaryChangeHandler(
        __in TxnReplicator::IStateProvider2 *stateProvider,
        __in fnNotifyStoreChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx);

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
    struct StoreChangeDataItem
    {
        DictionaryChangeHandler::SPtr Handler;
        StoreChangeKind Kind;
        void *Data;
    };

    static NTSTATUS AppendData(TxnReplicator::Transaction *txn, StoreChangeDataItem item);
    static NTSTATUS FlushData(TxnReplicator::Transaction *txn);
#endif

private:
    fnNotifyStoreChangeCallback notifyStoreChangeCallback_;
    fnCleanupContextCallback cleanupContextCallback_;
	TxnReplicator::IStateProvider2 *stateProvider_;        // weak reference to avoid cycles - the notification always come from store
    void* ctx_;
    StoreChangeData_Rebuild rebuildData_;

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
private:
    static NTSTATUS StaticInit(__in KAllocator &allocator);

private:
    static Common::atomic_bool s_initialized;
    static Data::TStore::ConcurrentDictionary2<Transaction *, KSharedArray<StoreChangeDataItem>::SPtr>::SPtr s_txnChangeDict;
    static KGlobalSpinLockStorage s_txnChangeDictLock;
#endif
};

