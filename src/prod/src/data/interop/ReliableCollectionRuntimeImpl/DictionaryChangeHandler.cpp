// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Utilities;
using namespace TStore;
using namespace TxnReplicator;
using namespace ktl;

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
ConcurrentDictionary2<Transaction *, KSharedArray<DictionaryChangeHandler::StoreChangeDataItem>::SPtr>::SPtr DictionaryChangeHandler::s_txnChangeDict;
Common::atomic_bool DictionaryChangeHandler::s_initialized;
KGlobalSpinLockStorage DictionaryChangeHandler::s_txnChangeDictLock;
#endif

HRESULT DictionaryChangeHandler::Create(
    __in KAllocator& allocator,
    __in IStateProvider2 *stateProvider,
    __in fnNotifyStoreChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void* ctx,
    __out DictionaryChangeHandler::SPtr& result)
{
    DictionaryChangeHandler* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) DictionaryChangeHandler(stateProvider, callback, cleanupCallback, ctx);
    if (!pointer)
        return E_OUTOFMEMORY;

    result = pointer;
    return S_OK;
}

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING

//
// Experimental batching support
//
NTSTATUS DictionaryChangeHandler::AppendData(TxnReplicator::Transaction *txn, StoreChangeDataItem item)
{
    KSharedArray<StoreChangeDataItem>::SPtr arrayPtr;

    if (!s_txnChangeDict->TryGetValue(txn, arrayPtr))
    {
        arrayPtr = _new(1, item.Handler->GetThisAllocator()) KSharedArray<StoreChangeDataItem>();
        if (arrayPtr == nullptr)
            return STATUS_INSUFFICIENT_RESOURCES;

        s_txnChangeDict->Add(txn, arrayPtr);
    }

    return arrayPtr->Append(item);
}

NTSTATUS DictionaryChangeHandler::FlushData(TxnReplicator::Transaction *txn)
{
    KSharedArray<StoreChangeDataItem>::SPtr arrayPtr;
    if (!s_txnChangeDict->TryGetValue(txn, arrayPtr))
        return S_OK;

    s_txnChangeDict->Remove(txn);

    for (ULONG i = 0; i < arrayPtr->Count(); ++i)
    {
        StoreChangeDataItem &item = (*arrayPtr)[i];
        
        // @TODO - Change this to a iterator based batch interface
        item.Handler->notifyStoreChangeCallback_(item.Handler->ctx_, item.Handler->stateProvider_, item.Kind, item.Data);

        _delete(item.Data);
    }

    s_txnChangeDict = nullptr;

    return S_OK;
}

#endif

ktl::Awaitable<void> DictionaryChangeHandler::OnAddedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,
    __in KeyType key,
    __in ValueType value,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(isPrimary);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&replicatorTransaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
    KUniquePtr<StoreChangeData_Add> addData = _new(1, GetThisAllocator()) StoreChangeData_Add { sequenceNumber, txnHandle, (LPCWSTR) *key, (char *)value->GetBuffer(), value->QuerySize() };
    if (!addData)
    {
        // @TODO - There is no way to tell client that we can't send the notification - need to log
        return;
    }

    NTSTATUS status = AppendData(txn, { this, StoreChangeKind_Add, addData } );
    if (!NT_SUCCESS(status))
    {
        // @TODO - There is no way to tell client that we can't send the notification - need to log
        return;
    }
    addData.Detach();
#else
    StoreChangeData_Add addData = { sequenceNumber, txnHandle, (LPCWSTR) *key, (char *)value->GetBuffer(), value->QuerySize() };

    notifyStoreChangeCallback_(ctx_, stateProvider_, StoreChangeKind_Add, &addData); 
#endif

    co_return;
}

ktl::Awaitable<void> DictionaryChangeHandler::OnUpdatedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,
    __in KeyType key,
    __in ValueType value,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(isPrimary);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&replicatorTransaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);

    StoreChangeData_Update updateData = { sequenceNumber, txnHandle, (LPCWSTR) *key, (char *)value->GetBuffer(), value->QuerySize() };

    notifyStoreChangeCallback_(ctx_, stateProvider_, StoreChangeKind_Update, &updateData);

    co_return;
}

ktl::Awaitable<void> DictionaryChangeHandler::OnRemovedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,
    __in KeyType key,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(isPrimary);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&replicatorTransaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);

    StoreChangeData_Remove removeData = { sequenceNumber, txnHandle, (LPCWSTR) *key };

    notifyStoreChangeCallback_(ctx_, stateProvider_, StoreChangeKind_Remove, &removeData);

    co_return;
}

ktl::Awaitable<void> DictionaryChangeHandler::OnRebuiltAsync(
    __in Utilities::IAsyncEnumerator<KeyValuePair<KeyType, KeyValuePair<LONG64, ValueType>>> & enumerableState) noexcept
{
    ktl::Awaitable<void> awaitable;
    NTSTATUS status;

    KSharedPtr<AwaitableCompletionSource<void>> acsPtr;
    status = ktl::AwaitableCompletionSource<void>::Create(GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, acsPtr);
    if (!NT_SUCCESS(status)) return Awaitable<void>();
    
    awaitable = acsPtr->GetAwaitable();

    // NOTE: Even this function returns Awaitable<void>, given there is no co_await/co_return here, this function is
    // *NOT* a coroutine! This means its stack frame won't be preserved. We need to stash the StoreChangeData_Rebuild
    // struct on the DictionaryChangeHandler object instead.
    rebuildData_ = { 
        // Note this isn't AddRefed so language projection should not call Release on this, or need to call AddRef first
        (StoreKeyValueAsyncEnumeratorHandle) &enumerableState, 
        (fnAsyncCompletionCallback)[](void *ctx) {
            // Signal completion - need to be called from language projection callback
            KSharedPtr<AwaitableCompletionSource<void>> ptr;
            ptr.Attach((AwaitableCompletionSource<void> *)ctx);
            ptr->Set();
        },
        acsPtr.Detach(),
    };

    notifyStoreChangeCallback_(ctx_, stateProvider_, StoreChangeKind_Rebuild, &rebuildData_);

    return awaitable;
}

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
void DictionaryChangeHandler::OnUnlock(__in TxnReplicator::TransactionBase const& replicatorTransaction) noexcept
{
    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&replicatorTransaction));
    FlushData(txn);
}
#endif

DictionaryChangeHandler::DictionaryChangeHandler(
    __in IStateProvider2 *stateProvider,
    __in fnNotifyStoreChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
	stateProvider_ = stateProvider;
    notifyStoreChangeCallback_ = callback;
    cleanupContextCallback_ = cleanupCallback;
    ctx_ = ctx;

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
    NTSTATUS status = StaticInit(this->GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(STATUS_NO_MEMORY);
        return;
    }
#endif
}

#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING

NTSTATUS DictionaryChangeHandler::StaticInit(__in KAllocator &allocator)
{    
    if (s_initialized.load())
        return STATUS_SUCCESS;

    K_LOCK_BLOCK(s_txnChangeDictLock.Lock())
    {
        // Check again in case more than one thread try to initialize
        if (s_initialized.load())
            return STATUS_SUCCESS;

        KSharedPtr<IComparer<Transaction *>> comparer;
        NTSTATUS status = DefaultComparer<Transaction *>::Create(allocator, comparer);
        if (!NT_SUCCESS(status))
            return status;
 
        // Bridge type to cast lambda to function pointer then to KDelegate
        typedef ULONG(*HashFunc)(Transaction *const&);

        status = ConcurrentDictionary2<Transaction *, KSharedArray<StoreChangeDataItem>::SPtr>::Create(
            100,
            (HashFunc)[](Transaction *const& txn) { return K_DefaultHashFunction((ULONG64)txn); },
            *comparer,
            allocator,
            s_txnChangeDict);
        if (!NT_SUCCESS(status))
            return status;

        s_initialized.store(true);
    }

    return STATUS_SUCCESS;
}

#endif

DictionaryChangeHandler::~DictionaryChangeHandler()
{
    //
    // Make sure API caller gets a chance to destroy the context
    // Note that it is possible to have a race between removing the notification and store firing another notification
    // with the cached instance. So we destroy it when store release the cached instance on stack
    //
    if (cleanupContextCallback_)
        cleanupContextCallback_(ctx_);
}
