// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Data;
using namespace Data::TStore;
using namespace TxnReplicator;
using namespace Data::Interop;
using namespace Data::Utilities;
using Data::Utilities::IAsyncEnumerator;

extern "C" void Store_Release(
    __in StateProviderHandle stateProviderHandle)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStateProvider2::SPtr stateProviderSPtr;
    stateProviderSPtr.Attach(stateProvider);
}

extern "C" void Store_AddRef(
    __in StateProviderHandle stateProviderHandle)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStateProvider2::SPtr stateProviderSPtr(stateProvider);
    stateProviderSPtr.Detach();
}

ktl::Task StoreConditionalGetAsyncInternal(
    IStore<KString::SPtr, KBuffer::SPtr>* store,
    Transaction* txn,
    LPCWSTR key,
    int64 timeout,
    Store_LockMode lockMode,
    size_t* objectHandle,
    Buffer* value,
    LONG64* versionSequenceNumber,
    ktl::CancellationTokenSource** cts,
    BOOL* found,
    fnNotifyGetAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    KString::SPtr kstringkey;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource = nullptr;
    Data::KeyValuePair<LONG64, KBuffer::SPtr> kvpair(-1, nullptr);
    KSharedPtr<IStoreTransaction<KString::SPtr, KBuffer::SPtr>> storeTxn;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    status = KString::Create(kstringkey, txn->GetThisAllocator(), key);
    CO_RETURN_VOID_ON_FAILURE(status);

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    if (lockMode == Store_LockMode_Update)
    {
        storeTxn->set_LockingHints(LockingHints::UpdateLock);
    }

    storeTxn->ReadIsolationLevel = IsolationHelper::GetIsolationLevel(*txn, IsolationHelper::OperationType::SingleEntity);

    auto awaitable = store->ConditionalGetAsync(*storeTxn, kstringkey, Common::TimeSpan::FromTicks(timeout), kvpair, cancellationToken);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*found = co_await awaitable, status);
        CO_RETURN_VOID_ON_FAILURE(status);

        if (*found)
        {
            KBuffer::SPtr kBufferSptr;
            kBufferSptr = kvpair.Value;
            char* buffer = (char*)kBufferSptr->GetBuffer();
            uint32_t bufferLength = kBufferSptr->QuerySize();
#ifdef FEATURE_CACHE_OBJHANDLE
            *objectHandle = *(size_t*)buffer;
            buffer += sizeof(size_t);
            bufferLength -= sizeof(size_t);
#else 
            *objectHandle = 0;
#endif
            value->Bytes = buffer;
            value->Length = bufferLength;
            value->Handle = kBufferSptr.Detach();
            *versionSequenceNumber = kvpair.get_Key();
        }
        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();

    BOOL result = false;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    EXCEPTION_TO_STATUS(result = co_await awaitable, ntstatus);
    if (result)
    {
        size_t objHandle = 0;
        byte* buffer = (byte*)kvpair.get_Value()->GetBuffer();
        ULONG bufferLength = kvpair.get_Value()->QuerySize();
#ifdef FEATURE_CACHE_OBJHANDLE
        objHandle = *(size_t*)buffer;
        buffer += sizeof(size_t);
        bufferLength -= sizeof(size_t);
#endif
        callback(ctx, StatusConverter::ToHResult(ntstatus), result, objHandle, buffer, bufferLength, kvpair.get_Key());
    }
    else
        callback(ctx, StatusConverter::ToHResult(ntstatus), result, 0, nullptr, 0, 0);
}

ktl::Task StoreAddAsyncInternal(
    IStore<KString::SPtr, KBuffer::SPtr>* store, 
    Transaction* txn, 
    LPCWSTR key, 
    size_t objectHandle, 
    void* bytes, 
    uint32_t bytesLength, 
    int64 timeout, 
    ktl::CancellationTokenSource** cts,
    fnNotifyAsyncCompletion callback,
    void* ctx, 
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    ULONG kBufferLength;
    KString::SPtr kstringkey;
    KBuffer::SPtr bufferSptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource;
    KSharedPtr<IStoreTransaction<KString::SPtr, KBuffer::SPtr>> storeTxn;

    status = STATUS_SUCCESS;
    synchronousComplete = false;
    kBufferLength = bytesLength;

#ifdef FEATURE_CACHE_OBJHANDLE
    kBufferLength += sizeof(size_t);
#endif

    status = KString::Create(kstringkey, txn->GetThisAllocator(), key);
    CO_RETURN_VOID_ON_FAILURE(status);

    status = KBuffer::Create(kBufferLength, bufferSptr, txn->GetThisAllocator());
    CO_RETURN_VOID_ON_FAILURE(status);

    auto buffer = bufferSptr->GetBuffer();
#ifdef FEATURE_CACHE_OBJHANDLE
    *(size_t*)buffer = objectHandle;
    buffer = (byte*)buffer + sizeof(size_t);
#endif
    memcpy(buffer, bytes, bytesLength);

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    auto awaitable = store->AddAsync(*storeTxn, kstringkey, bufferSptr, Common::TimeSpan::FromTicks(timeout), cancellationToken);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(co_await awaitable, status);
        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();
    NTSTATUS ntstatus = STATUS_SUCCESS;

    EXCEPTION_TO_STATUS(co_await awaitable, ntstatus);

    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

ktl::Task StoreConditionalUpdateAsyncInternal(
    IStore<KString::SPtr, KBuffer::SPtr>* store,
    Transaction* txn,
    LPCWSTR key,
    size_t objectHandle,
    void* bytes,
    uint32_t bytesLength,
    int64 timeout,
    ktl::CancellationTokenSource** cts,
    LONG64 conditionalVersion,
    BOOL* updated,
    fnNotifyUpdateAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    ULONG kBufferLength;
    //TODO generic key types
    KString::SPtr kstringkey;
    //TODO define custom value type
    KBuffer::SPtr bufferSptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource = nullptr;
    KSharedPtr<IStoreTransaction<KString::SPtr, KBuffer::SPtr>> storeTxn;

    status = STATUS_SUCCESS;
    synchronousComplete = false;
    kBufferLength = bytesLength;

#ifdef FEATURE_CACHE_OBJHANDLE
    kBufferLength += sizeof(size_t);
#endif

    status = KString::Create(kstringkey, txn->GetThisAllocator(), key);
    CO_RETURN_VOID_ON_FAILURE(status);

    status = KBuffer::Create(kBufferLength, bufferSptr, txn->GetThisAllocator());
    CO_RETURN_VOID_ON_FAILURE(status);

    auto buffer = bufferSptr->GetBuffer();
#ifdef FEATURE_CACHE_OBJHANDLE
    *(size_t*)buffer = objectHandle;
    buffer = (byte*)buffer + sizeof(size_t);
#endif
    memcpy(buffer, bytes, bytesLength);

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    auto awaitable = store->ConditionalUpdateAsync(*storeTxn, kstringkey, bufferSptr, Common::TimeSpan::FromTicks(timeout), cancellationToken, conditionalVersion);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*updated = co_await awaitable, status);
        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();
    BOOL result = false;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    EXCEPTION_TO_STATUS(result = co_await awaitable, ntstatus);

    callback(ctx, StatusConverter::ToHResult(ntstatus), result);
}

ktl::Task StoreConditionalRemoveAsyncInternal(
    IStore<KString::SPtr, KBuffer::SPtr>* store,
    Transaction* txn,
    LPCWSTR key,
    int64 timeout,
    ktl::CancellationTokenSource** cts,
    LONG64 conditionalVersion,
    BOOL* removed,
    fnNotifyRemoveAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    KString::SPtr kstringKey;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    IStoreTransaction<KString::SPtr, KBuffer::SPtr>::SPtr storeTxn;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    status = KString::Create(kstringKey, txn->GetThisAllocator(), key);
    CO_RETURN_VOID_ON_FAILURE(status);

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    auto awaitable = store->ConditionalRemoveAsync(*storeTxn, kstringKey, Common::TimeSpan::FromTicks(timeout), cancellationToken, conditionalVersion);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*removed = co_await awaitable, status);
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = STATUS_SUCCESS;
    BOOL isRemoved = false;

    EXCEPTION_TO_STATUS(isRemoved = co_await awaitable, ntstatus);

    callback(ctx, StatusConverter::ToHResult(ntstatus), isRemoved);
}

ktl::Task StoreContainsKeyAsyncInternal(
    IStore<KString::SPtr, KBuffer::SPtr>* store,
    Transaction* txn,
    LPCWSTR key,
    int64 timeout,
    Store_LockMode lockMode,
    ktl::CancellationTokenSource** cts,
    BOOL* found,
    fnNotifyContainsKeyAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    KString::SPtr kstringKey;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    IStoreTransaction<KString::SPtr, KBuffer::SPtr>::SPtr storeTxn;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    status = KString::Create(kstringKey, txn->GetThisAllocator(), key);
    CO_RETURN_VOID_ON_FAILURE(status);

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (lockMode == Store_LockMode_Update)
    {
        storeTxn->set_LockingHints(LockingHints::UpdateLock);
    }

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    auto awaitable = store->ContainsKeyAsync(*storeTxn, kstringKey, Common::TimeSpan::FromTicks(timeout), cancellationToken);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*found = co_await awaitable, status);
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = STATUS_SUCCESS;
    BOOL isFound = false;

    EXCEPTION_TO_STATUS(isFound = co_await awaitable, ntstatus);

    callback(ctx, StatusConverter::ToHResult(ntstatus), isFound);
}

extern "C" HRESULT Store_ConditionalGetAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __in Store_LockMode lockMode,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out int64_t* versionSequenceNumber,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* found,
    __in fnNotifyGetAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreConditionalGetAsyncInternal(
        store,
        (Transaction*)txn, 
        key,
        timeout,
        lockMode,
        objectHandle,
        value,
        versionSequenceNumber, 
        (ktl::CancellationTokenSource**)cts, 
        found,
        callback,
        ctx, 
        status,
        *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_AddAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in size_t objectHandle,
    __in void* bytes,
    __in uint32_t bytesLength,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreAddAsyncInternal(
        store,
        (Transaction*)txn,
        key, objectHandle, bytes, bytesLength, timeout,
        (ktl::CancellationTokenSource**)cts, 
        callback, ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_ConditionalUpdateAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in size_t objectHandle,
    __in void* bytes,
    __in uint32_t bytesLength,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in int64_t conditionalVersion,
    __out BOOL* updated,
    __in fnNotifyUpdateAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreConditionalUpdateAsyncInternal(
        store,
        (Transaction*)txn,
        key, objectHandle, bytes, bytesLength, timeout,
        (ktl::CancellationTokenSource**)cts,
        conditionalVersion, updated, callback, 
        ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_ConditionalRemoveAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in int64_t conditionalVersion,
    __out BOOL* removed,
    __in fnNotifyRemoveAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreConditionalRemoveAsyncInternal(
        store,
        (Transaction*)txn, 
        key, timeout, 
        (ktl::CancellationTokenSource**)cts, 
        conditionalVersion, removed, callback, 
        ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_GetCount(
    __in StateProviderHandle stateProviderHandle,
    __out int64_t* count)
{
    NTSTATUS status = STATUS_SUCCESS;
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    EXCEPTION_TO_STATUS(*count = store->Count, status);

    return StatusConverter::ToHResult(status);
}



extern "C" HRESULT Store_SetNotifyStoreChangeCallback(
    __in StateProviderHandle stateProviderHandle, 
    __in fnNotifyStoreChangeCallback callback, 
    __in fnCleanupContextCallback cleanupCallback, 
    __in void *ctx)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    if (callback == nullptr)
    {
        // Unsets if nullptr is passed in
        store->set_DictionaryChangeHandler(nullptr);
        return S_OK;
    }
    
    DictionaryChangeHandler::SPtr dictionaryChangeHandlerPtr;
    NTSTATUS status = DictionaryChangeHandler::Create(store->Allocator, stateProvider, callback, cleanupCallback, ctx, dictionaryChangeHandlerPtr);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    store->set_DictionaryChangeHandler(dictionaryChangeHandlerPtr.RawPtr());

    return S_OK;
}

extern "C" HRESULT Store_SetNotifyStoreChangeCallbackMask(
    __in StateProviderHandle stateProviderHandle,
    __in NotifyStoreChangeCallbackMask mask)
{
    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;
   
    store->DictionaryChangeHandlerMask = (TStore::DictionaryChangeEventMask::Enum) mask;
    return S_OK;
}

ktl::Task StoreCreateKeyEnumeratorAsyncInternal(
    __in IStore<KString::SPtr, KBuffer::SPtr>* store,
    __in Transaction* txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete, 
    __out Data::IEnumerator<KString::SPtr>::SPtr& enumerator)
{
    KString::SPtr kstringFirstKey;
    KString::SPtr kstringLastKey;
    IStoreTransaction<KString::SPtr, KBuffer::SPtr>::SPtr storeTxn;
    ktl::Awaitable<Data::IEnumerator<KString::SPtr>::SPtr> awaitable;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    // Set Isolation level to Snapshot
    storeTxn->ReadIsolationLevel = IsolationHelper::GetIsolationLevel(*txn, IsolationHelper::OperationType::MultiEntity);

    if (firstKey != nullptr)
    {
        status = KString::Create(kstringFirstKey, txn->GetThisAllocator(), firstKey);
        CO_RETURN_VOID_ON_FAILURE(status);
    }

    if (lastKey != nullptr)
    {
        status = KString::Create(kstringLastKey, txn->GetThisAllocator(), lastKey);
        CO_RETURN_VOID_ON_FAILURE(status);
    }

    ASSERT_IF(lastKey != nullptr && firstKey == nullptr, "lastKey requires firstKey to be specified");

    if (lastKey != nullptr)
        awaitable = store->CreateKeyEnumeratorAsync(*storeTxn, kstringFirstKey, kstringLastKey);
    else if (firstKey != nullptr)
        awaitable = store->CreateKeyEnumeratorAsync(*storeTxn, kstringFirstKey);
    else
        awaitable = store->CreateKeyEnumeratorAsync(*storeTxn);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(enumerator = co_await awaitable, status);
        co_return;
    }

    NTSTATUS ntstatus = STATUS_SUCCESS;
    Data::IEnumerator<KString::SPtr>::SPtr result;
    EXCEPTION_TO_STATUS(result = co_await awaitable, ntstatus);
    callback(ctx, StatusConverter::ToHResult(ntstatus), result.Detach());
}

extern "C" HRESULT Store_CreateKeyEnumeratorAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyEnumeratorHandle* enumerator,
    __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    Data::IEnumerator<KString::SPtr>::SPtr retval;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreCreateKeyEnumeratorAsyncInternal(
        store,
        (Transaction*)txn, 
        firstKey, lastKey, callback, 
        ctx, status, *synchronousComplete, retval);

    *enumerator = retval.Detach();
    return StatusConverter::ToHResult(status);
}

ktl::Task StoreCreateEnumeratorAsyncInternal(
    __in IStore<KString::SPtr, KBuffer::SPtr>* store,
    __in Transaction* txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete,
    __out IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>::SPtr& retval)
{
    KString::SPtr kstringFirstKey;
    KString::SPtr kstringLastKey;
    TStore::IStoreTransaction<KString::SPtr, KBuffer::SPtr>::SPtr storeTxn;
    ktl::Awaitable<KSharedPtr<Data::Utilities::IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>>> awaitable;
    status = STATUS_SUCCESS;
    synchronousComplete = false;

    EXCEPTION_TO_STATUS(store->CreateOrFindTransaction(*txn, storeTxn), status);
    CO_RETURN_VOID_ON_FAILURE(status);

    storeTxn->ReadIsolationLevel = IsolationHelper::GetIsolationLevel(*txn, IsolationHelper::OperationType::MultiEntity);

    if (firstKey != nullptr)
    {
        status = KString::Create(kstringFirstKey, txn->GetThisAllocator(), firstKey);
        CO_RETURN_VOID_ON_FAILURE(status);
    }

    if (lastKey != nullptr)
    {
        status = KString::Create(kstringLastKey, txn->GetThisAllocator(), lastKey);
        CO_RETURN_VOID_ON_FAILURE(status);
    }

    awaitable = store->CreateEnumeratorAsync(*storeTxn, kstringFirstKey, firstKey != nullptr, kstringLastKey, lastKey != nullptr);

    NTSTATUS ntstatus = STATUS_SUCCESS;
    IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>::SPtr enumeratorSPtr;
    EXCEPTION_TO_STATUS(enumeratorSPtr = co_await awaitable, ntstatus);
    callback(ctx, StatusConverter::ToHResult(ntstatus), enumeratorSPtr.Detach());
}

extern "C" HRESULT Store_CreateEnumeratorAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>::SPtr retval;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreCreateEnumeratorAsyncInternal(
        store, (Transaction*)txn, nullptr, nullptr,
        callback, ctx,
        status, *synchronousComplete, retval);

    *enumerator = retval.Detach();
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_CreateRangedEnumeratorAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>::SPtr retval;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreCreateEnumeratorAsyncInternal(
        store, (Transaction*)txn, firstKey, lastKey,
        callback, ctx, status, *synchronousComplete, retval);


    *enumerator = retval.Detach();
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT Store_ContainsKeyAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __in Store_LockMode lockMode,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* found,
    __in fnNotifyRemoveAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;

    IStateProvider2* stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    IStore<KString::SPtr, KBuffer::SPtr>* store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);
    if (store == nullptr)
        return E_INVALIDARG;

    StoreContainsKeyAsyncInternal(
        store,
        (Transaction*)txn,
        key, timeout, lockMode,
        (ktl::CancellationTokenSource**)cts,
        found, callback,
        ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}
