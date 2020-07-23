// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Data;
using namespace Data::TStore;
using namespace TxnReplicator;
using namespace Data::Interop;
using namespace Data::Utilities;

extern "C" HRESULT TxnReplicator_CreateTransaction(
    __in TxnReplicatorHandle txnReplicator,
    __out TransactionHandle* txn) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    Transaction::SPtr txnSPtr;

    status = ((ITransactionalReplicator*)txnReplicator)->CreateTransaction(txnSPtr);
    if (NT_SUCCESS(status))
        *txn = txnSPtr.Detach();

    return StatusConverter::ToHResult(status);
}

// default stateProviderType. This should go away when we remove
// TxnReplicator_GetOrAddStateProvider && TxnReplicator_AddStateProvider
// 0 - Marker for new encoding
// /1 - Version
// /1 - Store
// \n -> no language metadata
KStringView const stateProviderTypeName = L"0\1\1\n";

ktl::Awaitable<NTSTATUS> __GetOrAddStateProviderAsync(
    TxnReplicator::ITransactionalReplicator* txnReplicator,
    TxnReplicator::Transaction* transaction,
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout,
    ktl::CancellationToken const & cancellationToken,
    IStateProvider2::SPtr& stateProvider,
    BOOL& alreadyExist) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KUriView stateProviderName(name);
    bool exists = false;
    if (stateProviderInfo == nullptr)
    {
        status = co_await txnReplicator->GetOrAddAsync(
            *transaction,
            stateProviderName,
            stateProviderTypeName,
            stateProvider,
            exists,
            nullptr, // OperationData const *
            Common::TimeSpan::FromTicks(timeout),
            cancellationToken);
    }
    else
    {
        KString::SPtr encodedStateProviderTypeName;
        Data::Interop::StateProviderInfo::SPtr stateProviderInfoSPtr;

        status = Data::Interop::StateProviderInfo::FromPublicApi(
            txnReplicator->StatefulPartition->GetThisAllocator(),
            lang,
            *stateProviderInfo,
            stateProviderInfoSPtr);
        CO_RETURN_ON_FAILURE(status);

        status = stateProviderInfoSPtr->Encode(encodedStateProviderTypeName);
        CO_RETURN_ON_FAILURE(status);

        status = co_await txnReplicator->GetOrAddAsync(
            *transaction,
            stateProviderName,
            *encodedStateProviderTypeName,
            stateProvider,
            exists,
            nullptr, // OperationData const *
            Common::TimeSpan::FromTicks(timeout),
            cancellationToken);
    }

    CO_RETURN_ON_FAILURE(status);

    alreadyExist = exists;
    co_return status;
}

ktl::Awaitable<NTSTATUS> __GetOrAddStateProviderAsync(
    ITransactionalReplicator* txnReplicator,
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout,
    ktl::CancellationToken const & cancellationToken,
    IStateProvider2::SPtr& stateProvider,
    BOOL& alreadyExist) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    Transaction::SPtr transaction;
    StateProvider_Kind stateProviderKind = StateProvider_Kind_Invalid;
    if (stateProviderInfo != nullptr)
        stateProviderKind = stateProviderInfo->Kind;

    status = txnReplicator->CreateTransaction(transaction);
    CO_RETURN_ON_FAILURE(status);
    KFinally([&] {transaction->Dispose(); });

    status = co_await __GetOrAddStateProviderAsync(
        txnReplicator,
        transaction.RawPtr(),
        name,
        lang,
        stateProviderInfo,
        timeout,
        cancellationToken,
        stateProvider,
        alreadyExist);

    CO_RETURN_ON_FAILURE(status);

    status = co_await transaction->CommitAsync();
    CO_RETURN_ON_FAILURE(status);

    // Is this for compat reliable dictionary state provider
    if (stateProviderKind == StateProvider_Kind_ReliableDictionary_Compat)
    {
        // For compat reliable dictionary state provider get the actual data store
        CompatRDStateProvider* pCompatRDStateProvider = dynamic_cast<CompatRDStateProvider*>(stateProvider.RawPtr());
        if (pCompatRDStateProvider == nullptr)
        {
            // Trace.WriteInfo(
            //    "TransactionalReplicatorCExports",
            //    "__GetOrAddStateProviderAsync: Dynamic casting to CompatRDStateProvider failed for {0}",
            //    ToStringLiteral(stateProviderName));
            co_return STATUS_OBJECT_TYPE_MISMATCH;
        }
        stateProvider = dynamic_cast<IStateProvider2*>(pCompatRDStateProvider->DataStore.RawPtr());
        if (stateProvider == nullptr)
        {
            // Trace.WriteInfo(
            //    "TransactionalReplicatorCExports",
            //    "__GetOrAddStateProviderAsync: DataStore in CompatRDStateProvider {0} is null",
            //    ToStringLiteral(stateProviderName));
            co_return STATUS_OBJECT_TYPE_MISMATCH;
        }
    }

    co_return status;
}

ktl::Task GetOrAddStateProviderAsyncInternal(
    ITransactionalReplicator* txnReplicator, 
    Transaction *txn,
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout, 
    ktl::CancellationTokenSource** cts,
    fnNotifyGetOrAddStateProviderAsyncCompletion callback,
    void* ctx, 
    NTSTATUS& status,
    BOOL& synchronousComplete, 
    IStateProvider2::SPtr& result,
    BOOL& alreadyExist)
{
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    // Create local var as lifetime of param passed by ref may not be throughout co-routine call
    BOOL exist = false;
    IStateProvider2::SPtr stateProviderSPtr;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    ktl::Awaitable<NTSTATUS> awaitable;
    if (txn == nullptr)
    {
        awaitable = __GetOrAddStateProviderAsync(txnReplicator, name, lang, stateProviderInfo, timeout, cancellationToken, stateProviderSPtr, exist);
    }
    else
    {
        awaitable = __GetOrAddStateProviderAsync(txnReplicator, txn, name, lang, stateProviderInfo, timeout, cancellationToken, stateProviderSPtr, exist);
    }

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        CO_RETURN_VOID_ON_FAILURE(status);
        result = stateProviderSPtr;
        alreadyExist = exist;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus), stateProviderSPtr.Detach(), exist);
}

extern "C" HRESULT TxnReplicator_GetOrAddStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in LPCWSTR lang,
    __in StateProvider_Info* stateProviderInfo,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __out StateProviderHandle* stateProviderHandle,
    __out BOOL* alreadyExist,
    __in fnNotifyGetOrAddStateProviderAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete) noexcept
{
    NTSTATUS status;
    IStateProvider2::SPtr stateProviderSPtr;
    GetOrAddStateProviderAsyncInternal(
        (ITransactionalReplicator*)txnReplicator,
        (Transaction*)txn,
        name,
        lang,
        stateProviderInfo,
        timeout,
        (ktl::CancellationTokenSource**)cts,
        callback,
        ctx,
        status,
        *synchronousComplete,
        stateProviderSPtr,
        *alreadyExist);
    *stateProviderHandle = stateProviderSPtr.Detach();
    return StatusConverter::ToHResult(status);
}

ktl::Awaitable<NTSTATUS> __AddStateProviderAsync(
    ITransactionalReplicator* txnReplicator,
    Transaction& transaction,
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout,
    ktl::CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KUriView stateProviderName(name);

    if (stateProviderInfo == nullptr)
    {
        status = co_await txnReplicator->AddAsync(
            transaction,
            stateProviderName,
            stateProviderTypeName,
            nullptr, // OperationData const *
            Common::TimeSpan::FromTicks(timeout),
            cancellationToken);
    }
    else
    {
        KString::SPtr encodedStateProviderTypeName;
        Data::Interop::StateProviderInfo::SPtr stateProviderInfoSPtr;

        status = Data::Interop::StateProviderInfo::FromPublicApi(
            txnReplicator->StatefulPartition->GetThisAllocator(),
            lang,
            *stateProviderInfo,
            stateProviderInfoSPtr);
        CO_RETURN_ON_FAILURE(status);

        status = stateProviderInfoSPtr->Encode(encodedStateProviderTypeName);
        CO_RETURN_ON_FAILURE(status);

        status = co_await txnReplicator->AddAsync(
            transaction,
            stateProviderName,
            *encodedStateProviderTypeName,
            nullptr, // OperationData const *
            Common::TimeSpan::FromTicks(timeout),
            cancellationToken);
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> __AddStateProviderAsync(
    ITransactionalReplicator* txnReplicator,
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout,
    ktl::CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    Transaction::SPtr transaction;

    status = txnReplicator->CreateTransaction(transaction);
    CO_RETURN_ON_FAILURE(status);

    KFinally([&] {transaction->Dispose(); });

    status = co_await __AddStateProviderAsync(
        txnReplicator,
        *transaction,
        name,
        lang,
        stateProviderInfo,
        timeout,
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    status = co_await transaction->CommitAsync();

    co_return status;
}

ktl::Task AddStateProviderAsyncInternal(
    ITransactionalReplicator* txnReplicator, 
    Transaction *txn, 
    LPCWSTR name,
    LPCWSTR lang,
    StateProvider_Info* stateProviderInfo,
    int64 timeout, 
    ktl::CancellationTokenSource** cts,
    fnNotifyAsyncCompletion callback,
    void* ctx, 
    NTSTATUS& status,
    BOOL& synchronousComplete) noexcept
{
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    ktl::Awaitable<NTSTATUS> awaitable;
    if (txn == nullptr)
    {
        awaitable = __AddStateProviderAsync(txnReplicator, name, lang, stateProviderInfo, timeout, cancellationToken);
    }
    else
    {
        awaitable = __AddStateProviderAsync(txnReplicator, *txn, name, lang, stateProviderInfo, timeout, cancellationToken);
    }
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();
    
    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT TxnReplicator_AddStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in LPCWSTR lang,
    __in StateProvider_Info* stateProviderInfo,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete) noexcept
{
    NTSTATUS status;

    AddStateProviderAsyncInternal(
        (ITransactionalReplicator*)txnReplicator,
        (Transaction*)txn,
        name,
        lang,
        stateProviderInfo,
        timeout,
        (ktl::CancellationTokenSource**)cts,
        callback, ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

ktl::Awaitable<NTSTATUS> __RemoveStateProviderAsync(
    ITransactionalReplicator* txnReplicator, 
    LPCWSTR name, 
    int64 timeout, 
    ktl::CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KUriView stateProviderName(name);
    TxnReplicator::Transaction::SPtr transaction;

    status = txnReplicator->CreateTransaction(transaction);
    CO_RETURN_ON_FAILURE(status);

    KFinally([&] {transaction->Dispose(); });

    status = co_await txnReplicator->RemoveAsync(
        *transaction,
        stateProviderName,
        Common::TimeSpan::FromTicks(timeout),
        cancellationToken);
    CO_RETURN_ON_FAILURE(status);

    status = co_await transaction->CommitAsync();

    co_return status;
}

ktl::Awaitable<NTSTATUS> __RemoveStateProviderAsync(
    ITransactionalReplicator* txnReplicator,
    Transaction *transaction,
    LPCWSTR name,
    int64 timeout,
    ktl::CancellationToken const & cancellationToken) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KUriView stateProviderName(name);

    status = co_await txnReplicator->RemoveAsync(
        *transaction,
        stateProviderName,
        Common::TimeSpan::FromTicks(timeout),
        cancellationToken);

    co_return status;
}

ktl::Task RemoveStateProviderAsyncInternal(
    ITransactionalReplicator* txnReplicator, 
    Transaction *txn, 
    LPCWSTR name,
    int64 timeout, 
    ktl::CancellationTokenSource** cts,
    fnNotifyAsyncCompletion callback,
    void* ctx, 
    NTSTATUS& status,
    BOOL& synchronousComplete) noexcept
{
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    ktl::Awaitable<NTSTATUS> awaitable;
    if (txn == nullptr) 
    {
        awaitable = __RemoveStateProviderAsync(txnReplicator, name, timeout, cancellationToken);
    }
    else
    {
        awaitable = __RemoveStateProviderAsync(txnReplicator, txn, name, timeout, cancellationToken);
    }
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT TxnReplicator_RemoveStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL*synchronousComplete) noexcept
{
    NTSTATUS status;

    RemoveStateProviderAsyncInternal(
        (ITransactionalReplicator*)txnReplicator, 
        (Transaction*)txn, 
        name, timeout, 
        (ktl::CancellationTokenSource**)cts, 
        callback, ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT TxnReplicator_CreateEnumerator(
    __in TxnReplicatorHandle txnReplicator,
    __in BOOL parentsOnly,
    __out StateProviderEnumeratorHandle* enumerator) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KSharedPtr<IEnumerator<KeyValuePair<KUri::CSPtr, KSharedPtr<IStateProvider2>>>> enumeratorSPtr;

    status = ((ITransactionalReplicator*)txnReplicator)->CreateEnumerator(parentsOnly, enumeratorSPtr);
    *enumerator = enumeratorSPtr.Detach();
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT TxnReplicator_GetStateProvider(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR name,
    __out StateProviderHandle* stateProviderHandle) noexcept
{
    NTSTATUS status = STATUS_SUCCESS;
    KUriView stateProviderName(name);
    IStateProvider2::SPtr stateProvider = nullptr;

    status = ((ITransactionalReplicator*)txnReplicator)->Get(stateProviderName, stateProvider);
    if (NT_SUCCESS(status))
    {
        // Is this compat reliable dictionary state provider
        if (dynamic_cast<CompatRDStateProvider*>(stateProvider.RawPtr()) != nullptr)
        {
            // return the actual data store state provider instead of the wrapper object
            CompatRDStateProvider* pCompatRDStateProvider = dynamic_cast<CompatRDStateProvider*>(stateProvider.RawPtr());
            stateProvider = dynamic_cast<IStateProvider2*>(pCompatRDStateProvider->DataStore.RawPtr());
        }
        *stateProviderHandle = stateProvider.Detach();
    }

    return StatusConverter::ToHResult(status);
}

extern "C" void TxnReplicator_Release(
    __in TxnReplicatorHandle txnReplicator) noexcept
{
    ITransactionalReplicator::SPtr txReplicatorSPtr;
    txReplicatorSPtr.Attach((ITransactionalReplicator*)txnReplicator);
}

ktl::Task TxnReplicator_BackupAsyncInternal(
    __in ITransactionalReplicator* txnReplicator,
    __in fnUploadAsync uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete)
{
    BackupCallbackHandler::SPtr backupCallbackHandler;
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    BackupInfo result;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    status = BackupCallbackHandler::Create(txnReplicator->StatefulPartition->GetThisAllocator(), uploadAsyncCallback, ctx, backupCallbackHandler);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    auto awaitable = txnReplicator->BackupAsync(
        *backupCallbackHandler,
        (FABRIC_BACKUP_OPTION)backupOption,
        Common::TimeSpan::FromTicks(timeout),
        cancellationToken,
        result);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

ktl::Task TxnReplicator_BackupAsyncInternal2(
    __in ITransactionalReplicator* txnReplicator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete)
{
    BackupCallbackHandler::SPtr backupCallbackHandler;
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    BackupInfo result;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    status = BackupCallbackHandler::Create(txnReplicator->StatefulPartition->GetThisAllocator(), uploadAsyncCallback, ctx, backupCallbackHandler);
    CO_RETURN_VOID_ON_FAILURE(status);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    auto awaitable = txnReplicator->BackupAsync(
        *backupCallbackHandler,
        (FABRIC_BACKUP_OPTION)backupOption,
        Common::TimeSpan::FromTicks(timeout),
        cancellationToken,
        result);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT TxnReplicator_BackupAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    TxnReplicator_BackupAsyncInternal(
        (ITransactionalReplicator*)txnReplicator,
        uploadAsyncCallback,
        backupOption,
        timeout,
        cts,
        callback,
        ctx,
        status,
        *synchronousComplete);
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT TxnReplicator_BackupAsync2(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    TxnReplicator_BackupAsyncInternal2(
        (ITransactionalReplicator*)txnReplicator,
        uploadAsyncCallback,
        backupOption,
        timeout,
        cts,
        callback,
        ctx,
        status,
        *synchronousComplete);
    return StatusConverter::ToHResult(status);
}

ktl::Task TxnReplicator_RestoreAsyncInternal(
    __in ITransactionalReplicator* txnReplicator,
    __in LPCWSTR backupFolder,
    __in Restore_Policy restorePolicy,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle*  cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete)
{
    KString::SPtr kstringBackupFolder;
    ktl::CancellationTokenSource::SPtr ctsSPtr = nullptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txnReplicator->StatefulPartition->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, ctsSPtr);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = ctsSPtr->Token;
    }

    status = KString::Create(kstringBackupFolder, txnReplicator->StatefulPartition->GetThisAllocator(), backupFolder);
    CO_RETURN_VOID_ON_FAILURE(status);

    auto awaitable = txnReplicator->RestoreAsync(
        *kstringBackupFolder,
        (FABRIC_RESTORE_POLICY)restorePolicy,
        Common::TimeSpan::FromTicks(timeout),
        cancellationToken);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        status = co_await awaitable;
        co_return;
    }

    if (cts != nullptr)
        *cts = ctsSPtr.Detach();

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT TxnReplicator_RestoreAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR backupFolder,
    __in Restore_Policy restorePolicy,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle*  cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    TxnReplicator_RestoreAsyncInternal(
        (ITransactionalReplicator*)txnReplicator,
        backupFolder,
        restorePolicy,
        timeout,
        cts,
        callback,
        ctx,
        status,
        *synchronousComplete);
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT TxnReplicator_SetNotifyStateManagerChangeCallback(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback, 
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    auto replicator = reinterpret_cast<ITransactionalReplicator*>(txnReplicator);

    if (callback == nullptr)
    {
        // Unsets if nullptr is passed in
        replicator->UnRegisterStateManagerChangeHandler();
        return S_OK;
    }

    StateManagerChangeHandler::SPtr stateManagerChangeHandler;
    NTSTATUS status = StateManagerChangeHandler::Create(replicator->StatefulPartition->GetThisAllocator(), replicator, callback, cleanupCallback, ctx, stateManagerChangeHandler);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    status = replicator->RegisterStateManagerChangeHandler(*stateManagerChangeHandler);
    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT TxnReplicator_SetNotifyTransactionChangeCallback(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyTransactionChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    auto replicator = reinterpret_cast<ITransactionalReplicator*>(txnReplicator);

    if (callback == nullptr)
    {
        // Unsets if nullptr is passed in
        replicator->UnRegisterTransactionChangeHandler();
        return S_OK;
    }

    TransactionChangeHandler::SPtr transactionChangeHandler;
    NTSTATUS status = TransactionChangeHandler::Create(replicator->StatefulPartition->GetThisAllocator(), replicator, callback, cleanupCallback, ctx, transactionChangeHandler);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    status = replicator->RegisterTransactionChangeHandler(*transactionChangeHandler);
    return StatusConverter::ToHResult(status);
}

struct TxnReplicator_Info_v1
{
    uint32_t Size;                  // For versioning
    int64_t LastStableSequenceNumber;
    int64_t LastCommittedSequenceNumber;
    Epoch CurrentEpoch;
};

extern "C" HRESULT TxnReplicator_GetInfo(
    __in TxnReplicatorHandle txnReplicator,
    __inout TxnReplicator_Info* info)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (info == nullptr || txnReplicator == nullptr)
        return E_INVALIDARG;

    auto replicator = reinterpret_cast<ITransactionalReplicator*>(txnReplicator);

    if (info->Size > sizeof(TxnReplicator_Info_v1))
        return E_INVALIDARG;

    status = replicator->GetLastStableSequenceNumber(info->LastStableSequenceNumber);
    ON_FAIL_RETURN_HR(status);

    status = replicator->GetLastCommittedSequenceNumber(info->LastCommittedSequenceNumber);
    ON_FAIL_RETURN_HR(status);

    FABRIC_EPOCH  epoch;
    status = replicator->GetCurrentEpoch(epoch);
    ON_FAIL_RETURN_HR(status);

    info->CurrentEpoch.DataLossNumber = epoch.DataLossNumber;
    info->CurrentEpoch.ConfigurationNumber = epoch.ConfigurationNumber;
    info->CurrentEpoch.Reserved = nullptr;

    return S_OK;
}

HRESULT GetStateProviderFactory(IFabricInternalStatefulServicePartition *internalPartition, Common::ComPointer<IFabricStateProvider2Factory>& comStateProviderFactory)
{
    HANDLE ktlSystem = nullptr;

    HRESULT hr = internalPartition->GetKtlSystem(&ktlSystem);
    if (!SUCCEEDED(hr))
        return hr;

    KtlSystem* ktlSystem_ = (KtlSystem*)ktlSystem;

    KAllocator & allocator = ktlSystem_->PagedAllocator();
    hr = ComStateProviderFactory::Create(allocator, comStateProviderFactory);
    return hr;
}

HRESULT GetTxnReplicatorInternal(
    __in void* statefulServicePartition,
    __in void* dataLossHandler,
    __in void const* fabricReplicatorSettings,
    __in void const* txReplicatorSettings,
    __in void const* ktlloggerSharedSettings,
    __out void** primaryReplicator,
    __out TxnReplicatorHandle* txnReplicatorHandle) noexcept
{
    IFabricStatefulServicePartition* _statefulServicePartition = reinterpret_cast<IFabricStatefulServicePartition*>(statefulServicePartition);
    IFabricDataLossHandler* _dataLossHandler = reinterpret_cast<IFabricDataLossHandler*>(dataLossHandler);
    FABRIC_REPLICATOR_SETTINGS const* _fabricReplicatorSettings = reinterpret_cast<FABRIC_REPLICATOR_SETTINGS const*>(fabricReplicatorSettings);
    TRANSACTIONAL_REPLICATOR_SETTINGS const* _txReplicatorSettings = reinterpret_cast<TRANSACTIONAL_REPLICATOR_SETTINGS const*>(txReplicatorSettings);
    KTLLOGGER_SHARED_LOG_SETTINGS const* _ktlloggerSharedSettings = reinterpret_cast<KTLLOGGER_SHARED_LOG_SETTINGS const*>(ktlloggerSharedSettings);

    Common::ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    HRESULT hr = _statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());
    if (!SUCCEEDED(hr))
        return hr;

    Common::ComPointer<IFabricStateProvider2Factory> spFactory;
    hr = GetStateProviderFactory(internalPartition.GetRawPointer(), spFactory);
    if (!SUCCEEDED(hr))
        return hr;

    hr = internalPartition->CreateTransactionalReplicator(
        spFactory.GetRawPointer(),
        _dataLossHandler,
        _fabricReplicatorSettings,
        _txReplicatorSettings,
        _ktlloggerSharedSettings,
        (IFabricPrimaryReplicator**)primaryReplicator,
        (void **)txnReplicatorHandle); // ref count of returned handle is incremented by 1

    return hr;
}

HRESULT GetEndpointResourceFromManifest(
    __in PartitionedReplicaId const & prId,
    __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in LPCWSTR endpointName,
    __out ULONG & port)
{
    ASSERT_IF(NULL == codePackageActivationContextCPtr.GetRawPointer(), "activationContext is null");

    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = codePackageActivationContextCPtr->get_ServiceEndpointResources();

    if (endpoints == NULL ||
        endpoints->Count == 0 ||
        endpoints->Items == NULL)
    {
        RCREventSource::Events->ExceptionError(
            prId.TracePartitionId,
            prId.ReplicaId,
            L"[GetEndpointResourceFromManifest] No endpoints found",
            E_FAIL);
        return E_FAIL;
    }

    wstring text;

    // find endpoint by name
    for (ULONG i = 0; i < endpoints->Count; ++i)
    {
        const auto & endpoint = endpoints->Items[i];

        text.append(wformatString(
            "Endpoint{0}:{1};{2};{3};{4} \n\r",
            i,
            endpoint.Name,
            endpoint.Protocol,
            endpoint.Type,
            endpoint.Port));

        ASSERT_IF(endpoint.Name == NULL, "FABRIC_ENDPOINT_RESOURCE_DESCRIPTION with Name == NULL");

        if (StringUtility::AreEqualCaseInsensitive(endpointName, endpoint.Name))
        {
            port = endpoint.Port;

            return S_OK;
        }
    }

    wstring errorMessage;
    StringWriter messageWriter(errorMessage);
    messageWriter.Write("[StatefulServiceBase::GetEndpointResourceFromManifest] No matching Endpoint Resources found in service manifest. Endpoint found are: {0}", text);

    RCREventSource::Events->ExceptionError(
        prId.TracePartitionId,
        prId.ReplicaId,
        errorMessage,
        E_FAIL);

    return E_FAIL;
}

Common::WStringLiteral const DefaultReplicatorEndpointName(L"ReplicatorEndpoint");

HRESULT Data::Interop::LoadReplicatorSettingsFromConfigPackage(
    __in PartitionedReplicaId const & prId,
    __in Common::ScopedHeap& heap,
    __in ReplicatorConfigSettingsResult& configSettingsResult,
    __in wstring& configPackageName,
    __in wstring& replicatorSettingsSectionName,
    __in wstring& replicatorSecuritySectionName,
    __out FABRIC_REPLICATOR_SETTINGS const ** fabricReplicatorSettings,
    __out TRANSACTIONAL_REPLICATOR_SETTINGS const ** txnReplicatorSettings,
    __out KTLLOGGER_SHARED_LOG_SETTINGS const ** ktlLoggerSharedLogSettings)
{
    HRESULT hr = S_OK;
    Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr;

    // Initialize out params to nullptr
    if (fabricReplicatorSettings != nullptr)
        *fabricReplicatorSettings = nullptr;

    if (txnReplicatorSettings != nullptr)
        *txnReplicatorSettings = nullptr;

    if (ktlLoggerSharedLogSettings != nullptr)
        *ktlLoggerSharedLogSettings = nullptr;

    if (configPackageName.empty())
    {
        RCREventSource::Events->Warning(
            prId.TracePartitionId,
            prId.ReplicaId,
            L"LoadReplicatorSettingsFromConfigPackage - Replicator Config not provided");
        return hr;
    }
   
    hr = ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, codePackageActivationContextCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            prId.TracePartitionId,
            prId.ReplicaId,
            L"LoadReplicatorSettingsFromConfigPackage - FabricGetActivationContext failed",
            hr);
        return hr;
    }

    Common::ComPointer<IFabricCodePackageActivationContext6> codePackageActivationContext6CPtr;
    hr = codePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext6,
        codePackageActivationContext6CPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            prId.TracePartitionId,
            prId.ReplicaId,
            L"LoadReplicatorSettingsFromConfigPackage - QueryInterface(IID_IFabricCodePackageActivationContext6) failed",
            hr);
        return hr;
    }

    IFabricConfigurationPackage *configPackage = nullptr;
    hr = codePackageActivationContextCPtr->GetConfigurationPackage(configPackageName.c_str(), &configPackage);
    if (FAILED(hr))
    {
        if (hr == FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND)
        {
            //
            // Fill in default value in FabricReplicatorSetting using default endpoint
            //
            ULONG replicatorPort;
            hr = GetEndpointResourceFromManifest(
                prId,
                codePackageActivationContextCPtr,
                DefaultReplicatorEndpointName.cbegin(),
                replicatorPort);
            if (FAILED(hr))
            {
                RCREventSource::Events->ExceptionError(
                    prId.TracePartitionId,
                    prId.ReplicaId,
                    L"LoadReplicatorSettingsFromConfigPackage - GetEndpointResourceFromManifest failed",
                    hr);
                return hr;
            }

            // We should use get_ServiceListenAddress to get hostname for replicator instead of get_ServicePublishAddress.
            // get_ServicePublishAddress will return host machine FQDN in case of NAT, using this as host name, would fail with Containerized apps, because
            // Transport has check for FQDNs, where it verifies that name being used is local to the container.
            // See RDBug 13677805 for more details. http://vstfrd:8080/Azure/RD/_workitems?id=13677805&_a=edit&fullScreen=false
            wstring serviceListenAddress = codePackageActivationContext6CPtr->get_ServiceListenAddress();

            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> heapFabricReplicatorSettings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS>();
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> heapFabricReplicatorSettingsEx1 = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX2> heapFabricReplicatorSettingsEx2 = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX3> heapFabricReplicatorSettingsEx3 = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX4> heapFabricReplicatorSettingsEx4 = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX4>();
            heapFabricReplicatorSettings->Reserved = heapFabricReplicatorSettingsEx1.GetRawPointer();
            heapFabricReplicatorSettingsEx1->Reserved = heapFabricReplicatorSettingsEx2.GetRawPointer();
            heapFabricReplicatorSettingsEx2->Reserved = heapFabricReplicatorSettingsEx3.GetRawPointer();
            heapFabricReplicatorSettingsEx3->Reserved = heapFabricReplicatorSettingsEx4.GetRawPointer();
            
            wstring listenHostAndPort;
            StringWriter(listenHostAndPort).Write("{0}:{1}", serviceListenAddress, replicatorPort);

            LPCWSTR replicatorListenAddress = heap.AddString(listenHostAndPort);
            heapFabricReplicatorSettingsEx4->ReplicatorListenAddress = replicatorListenAddress;
            heapFabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_LISTEN_ADDRESS;

            wstring servicePublishAddress = codePackageActivationContext6CPtr->get_ServicePublishAddress();
            wstring publishHostAndPort;
            StringWriter(publishHostAndPort).Write("{0}:{1}", servicePublishAddress, replicatorPort);
            LPCWSTR replicatorPublishAddress = heap.AddString(publishHostAndPort);            
            heapFabricReplicatorSettingsEx4->ReplicatorPublishAddress = replicatorPublishAddress;
            heapFabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_PUBLISH_ADDRESS;
            *fabricReplicatorSettings = heapFabricReplicatorSettings.GetRawPointer();

            wstring replicatorAddressTrace;
            replicatorAddressTrace.append(wformatString(
                "Using ReplicatorListenAddress {0} and ReplicatorPublishAddress {1}",
                replicatorListenAddress,
                replicatorPublishAddress
            ));

            RCREventSource::Events->Info(
                prId.TracePartitionId,
                prId.ReplicaId,
                replicatorAddressTrace);

            return S_OK;
        }
        else
        {
            RCREventSource::Events->ExceptionError(
                prId.TracePartitionId,
                prId.ReplicaId,
                L"LoadReplicatorSettingsFromConfigPackage - GetConfigurationPackage failed",
                hr);
            return hr;
        }
    }

    if (!replicatorSettingsSectionName.empty())
    {
        const FABRIC_CONFIGURATION_SECTION* section = nullptr;
        hr = configPackage->GetSection(replicatorSettingsSectionName.c_str(), &section);
        if (hr != FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND && FAILED(hr))
        {
            RCREventSource::Events->ExceptionError(
                prId.TracePartitionId,
                prId.ReplicaId,
                L"LoadReplicatorSettingsFromConfigPackage - GetSection failed",
                hr);
            return hr;
        }

        if (hr != FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND && section != nullptr)
        {
            if (fabricReplicatorSettings != nullptr)
            {
                hr = ::FabricLoadReplicatorSettings(
                    codePackageActivationContextCPtr.GetRawPointer(),
                    configPackageName.c_str(),
                    replicatorSettingsSectionName.c_str(),
                    configSettingsResult.v1ReplicatorSettingsResult.InitializationAddress());
                if (FAILED(hr))
                {
                    RCREventSource::Events->ExceptionError(
                        prId.TracePartitionId,
                        prId.ReplicaId,
                        L"LoadReplicatorSettingsFromConfigPackage - FabricLoadReplicatorSettings failed",
                        hr);
                    return hr;
                }

                *fabricReplicatorSettings = configSettingsResult.v1ReplicatorSettingsResult->get_ReplicatorSettings();
            }

            if (txnReplicatorSettings != nullptr)
            {
                hr = TxnReplicator::TransactionalReplicatorSettings::FromConfig(
                    codePackageActivationContextCPtr,
                    configPackageName,
                    replicatorSettingsSectionName,
                    configSettingsResult.txnReplicatorSettingsResult.InitializationAddress());
                if (FAILED(hr))
                {
                    RCREventSource::Events->ExceptionError(
                        prId.TracePartitionId,
                        prId.ReplicaId,
                        L"LoadReplicatorSettingsFromConfigPackage - TransactionalReplicatorSettings::FromConfig failed",
                        hr);
                    return hr;
                }

                *txnReplicatorSettings = configSettingsResult.txnReplicatorSettingsResult->get_TransactionalReplicatorSettings();
            }

            if (ktlLoggerSharedLogSettings != nullptr)
            {
                hr = TxnReplicator::KtlLoggerSharedLogSettings::FromConfig(
                    codePackageActivationContextCPtr,
                    configPackageName,
                    replicatorSettingsSectionName,
                    configSettingsResult.sharedLogSettingsResult.InitializationAddress());
                if (FAILED(hr))
                {
                    RCREventSource::Events->ExceptionError(
                        prId.TracePartitionId,
                        prId.ReplicaId,
                        L"LoadReplicatorSettingsFromConfigPackage - KtlLoggerSharedLogSettings::FromConfig failed",
                        hr);
                    return hr;
                }

                *ktlLoggerSharedLogSettings = configSettingsResult.sharedLogSettingsResult->get_Settings();
            }
        }
        else
        {
            // If replication settings section not found then trace warning and return hresult should be success
            RCREventSource::Events->Warning(
                prId.TracePartitionId,
                prId.ReplicaId,
                L"LoadReplicatorSettingsFromConfigPackage - Replication settings config section does not exists");
            hr = S_OK;
        }
    }

    if (!replicatorSecuritySectionName.empty() && fabricReplicatorSettings != nullptr)
    {
        const FABRIC_CONFIGURATION_SECTION* section = nullptr;;
        hr = configPackage->GetSection(replicatorSecuritySectionName.c_str(), &section);
        if (hr != FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND && FAILED(hr))
        {
            RCREventSource::Events->ExceptionError(
                prId.TracePartitionId,
                prId.ReplicaId,
                L"LoadReplicatorSettingsFromConfigPackage - GetSection failed for replicator security settings",
                hr);
            return hr;
        }

        if (hr != FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND && section != nullptr)
        {
            hr = ::FabricLoadSecurityCredentials(
                codePackageActivationContextCPtr.GetRawPointer(),
                configPackageName.c_str(),
                replicatorSecuritySectionName.c_str(),
                configSettingsResult.replicatorSecurityCredentialsResult.InitializationAddress());
            if (FAILED(hr))
            {
                RCREventSource::Events->ExceptionError(
                    prId.TracePartitionId,
                    prId.ReplicaId,
                    L"LoadReplicatorSettingsFromConfigPackage - FabricLoadSecurityCredentials failed",
                    hr);
                return hr;
            }

            FABRIC_SECURITY_CREDENTIALS const* fabricSecurityCredentials = nullptr;
            fabricSecurityCredentials = configSettingsResult.replicatorSecurityCredentialsResult->get_SecurityCredentials();

            // Create new FABRIC_REPLICATOR_SETTINGS instance;
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> heapFabricReplicatorSettings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS>();
            // If we already have valid fabric replication setting object then update it
            if (*fabricReplicatorSettings != nullptr)
            {
                // cannot modify contents of const struct
                // perform a shallow copy of the structure and update SecurityCredentials field
                *heapFabricReplicatorSettings = *(*fabricReplicatorSettings);
                heapFabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECURITY;
                heapFabricReplicatorSettings->SecurityCredentials = fabricSecurityCredentials;
            }
            else
            {
                // Initialize flags to only have security flag set
                heapFabricReplicatorSettings->Flags = FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECURITY;
                heapFabricReplicatorSettings->SecurityCredentials = fabricSecurityCredentials;
            }
            *fabricReplicatorSettings = heapFabricReplicatorSettings.GetRawPointer();
        }
        else
        {
            // When section is not found trace warning and return success
            RCREventSource::Events->Warning(
                prId.TracePartitionId,
                prId.ReplicaId,
                L"LoadReplicatorSettingsFromConfigPackage - Replication security config section does not exists");
        }
    }

    return S_OK;
}

// Assumes the out params are already initialized to default values from cluster config
// Only overrides the new settings passed in from code
void ReplicatorSettingsToPublicApiSettings(
    __in Common::ScopedHeap& heap,
    __in TxnReplicator_Settings const* replicatorSettings,
    __inout FABRIC_REPLICATOR_SETTINGS* fabricReplicatorSettings,
    __inout TRANSACTIONAL_REPLICATOR_SETTINGS* transactionalReplicatorSettings,
    __inout KTLLOGGER_SHARED_LOG_SETTINGS* ktlLoggerSharedLogSettings)
{
    Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> ex1Settings;
    Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX2> ex2Settings;
    Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX3> ex3Settings;
    Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX4> ex4Settings;
    Common::ReferencePointer<FABRIC_SECURITY_CREDENTIALS> securitySettings;

    if (replicatorSettings == nullptr)
        return;

    if (replicatorSettings->Flags == TxnReplicator_Settings_Flags::None)
        return;

    if (fabricReplicatorSettings != nullptr)
    {
        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::RetryInterval)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_RETRY_INTERVAL;
            fabricReplicatorSettings->RetryIntervalMilliseconds = replicatorSettings->RetryIntervalMilliseconds;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::BatchAcknowledgementInterval)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
            fabricReplicatorSettings->BatchAcknowledgementIntervalMilliseconds = replicatorSettings->BatchAcknowledgementIntervalMilliseconds;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::ReplicatorAddress)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_ADDRESS;
            // not allocating new memory for string as lifetime of fabricReplicatorSetting is not beyond replicatorSettings
            fabricReplicatorSettings->ReplicatorAddress = (LPCWSTR)replicatorSettings->ReplicatorAddress;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::InitialCopyQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            fabricReplicatorSettings->InitialCopyQueueSize = replicatorSettings->InitialCopyQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxCopyQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            fabricReplicatorSettings->MaxCopyQueueSize = replicatorSettings->MaxCopyQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::SecurityCredentials)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECURITY;
            // not allocating new memory for SecurityCredentials as lifetime of fabricReplicatorSetting is not beyond replicatorSettings
            fabricReplicatorSettings->SecurityCredentials = (FABRIC_SECURITY_CREDENTIALS*)replicatorSettings->SecurityCredentials;
        }

        fabricReplicatorSettings->Reserved = nullptr;

        if (replicatorSettings->Flags >= TxnReplicator_Settings_Flags::SecondaryClearAcknowledgedOperations)
        {
            ex1Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
            ex1Settings->Reserved = nullptr;
            fabricReplicatorSettings->Reserved = ex1Settings.GetRawPointer();
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::SecondaryClearAcknowledgedOperations)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
            ex1Settings->SecondaryClearAcknowledgedOperations = (BOOLEAN)replicatorSettings->SecondaryClearAcknowledgedOperations;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxReplicationMessageSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
            ex1Settings->MaxReplicationMessageSize = replicatorSettings->MaxReplicationMessageSize;
        }

        if (replicatorSettings->Flags >= TxnReplicator_Settings_Flags::InitialPrimaryReplicationQueueSize)
        {
            ex2Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
            ex1Settings->Reserved = ex2Settings.GetRawPointer();
            ex3Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
            ex2Settings->Reserved = ex3Settings.GetRawPointer();
            ex3Settings->Reserved = nullptr;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::InitialPrimaryReplicationQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            ex3Settings->InitialPrimaryReplicationQueueSize = replicatorSettings->InitialPrimaryReplicationQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxPrimaryReplicationQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            ex3Settings->MaxPrimaryReplicationQueueSize = replicatorSettings->MaxPrimaryReplicationQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxPrimaryReplicationQueueMemorySize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            ex3Settings->MaxPrimaryReplicationQueueMemorySize = replicatorSettings->MaxPrimaryReplicationQueueMemorySize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::InitialSecondaryReplicationQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            ex3Settings->InitialSecondaryReplicationQueueSize = replicatorSettings->InitialSecondaryReplicationQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxSecondaryReplicationQueueSize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            ex3Settings->MaxSecondaryReplicationQueueSize = replicatorSettings->MaxSecondaryReplicationQueueSize;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxSecondaryReplicationQueueMemorySize)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            ex3Settings->MaxSecondaryReplicationQueueMemorySize = replicatorSettings->MaxSecondaryReplicationQueueMemorySize;
        }

        if (replicatorSettings->Flags >= TxnReplicator_Settings_Flags::ReplicatorListenAddress)
        {
            ex4Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX4>();
            ex4Settings->Reserved = nullptr;
            ex3Settings->Reserved = ex4Settings.GetRawPointer();
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::ReplicatorListenAddress)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_LISTEN_ADDRESS;
            // not allocating new memory for string as lifetime of fabricReplicatorSetting is not beyond replicatorSettings
            ex4Settings->ReplicatorListenAddress = (LPCWSTR)replicatorSettings->ReplicatorListenAddress;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::ReplicatorPublishAddress)
        {
            fabricReplicatorSettings->Flags |= FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_PUBLISH_ADDRESS;
            // not allocating new memory for string as lifetime of fabricReplicatorSetting is not beyond replicatorSettings
            ex4Settings->ReplicatorPublishAddress = (LPCWSTR)replicatorSettings->ReplicatorPublishAddress;
        }
    }

    if (transactionalReplicatorSettings != nullptr)
    {
        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxStreamSize)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;
            transactionalReplicatorSettings->MaxStreamSizeInMB = replicatorSettings->MaxStreamSizeInMB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxMetadataSize)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB;
            transactionalReplicatorSettings->MaxMetadataSizeInKB = replicatorSettings->MaxMetadataSizeInKB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxRecordSize)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB;
            transactionalReplicatorSettings->MaxRecordSizeInKB = replicatorSettings->MaxRecordSizeInKB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxWriteQueueDepth)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB;
            transactionalReplicatorSettings->MaxWriteQueueDepthInKB = replicatorSettings->MaxWriteQueueDepthInKB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::CheckpointThreshold)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
            transactionalReplicatorSettings->CheckpointThresholdInMB = replicatorSettings->CheckpointThresholdInMB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MaxAccumulatedBackupLogSize)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB;
            transactionalReplicatorSettings->MaxAccumulatedBackupLogSizeInMB = replicatorSettings->MaxAccumulatedBackupLogSizeInMB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::OptimizeForLocalSSD)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD;
            transactionalReplicatorSettings->OptimizeForLocalSSD = (BOOLEAN)replicatorSettings->OptimizeForLocalSSD;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::OptimizeLogForLowerDiskUsage)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE;
            transactionalReplicatorSettings->OptimizeLogForLowerDiskUsage = (BOOLEAN)replicatorSettings->OptimizeLogForLowerDiskUsage;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::SlowApiMonitoringDuration)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;
            transactionalReplicatorSettings->SlowApiMonitoringDurationSeconds = replicatorSettings->SlowApiMonitoringDurationSeconds;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::MinLogSize)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
            transactionalReplicatorSettings->MinLogSizeInMB = replicatorSettings->MinLogSizeInMB;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::TruncationThresholdFactor)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
            transactionalReplicatorSettings->TruncationThresholdFactor = replicatorSettings->TruncationThresholdFactor;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::ThrottlingThresholdFactor)
        {
            transactionalReplicatorSettings->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;
            transactionalReplicatorSettings->ThrottlingThresholdFactor = replicatorSettings->ThrottlingThresholdFactor;
        }
    }

    if (ktlLoggerSharedLogSettings != nullptr)
    {
        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::SharedLogId)
        {
            ktlLoggerSharedLogSettings->ContainerId = (LPCWSTR)replicatorSettings->SharedLogId;
        }

        if (replicatorSettings->Flags & TxnReplicator_Settings_Flags::SharedLogPath)
        {
            ktlLoggerSharedLogSettings->ContainerPath = (LPCWSTR)replicatorSettings->SharedLogPath;
        }
    }
}


HRESULT GetKtlSystem(
    IFabricStatefulServicePartition* statefulServicePartition,
    KtlSystem** ktlSystem)
{
    Common::ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    HRESULT hr = statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());
    if (!SUCCEEDED(hr))
        return hr;

    hr = internalPartition->GetKtlSystem((HANDLE*)ktlSystem);
    if (!SUCCEEDED(hr))
        return hr;

    return S_OK;
}


extern "C" HRESULT GetTxnReplicator(
    __in int64_t replicaId,
    __in void* statefulServicePartition,
    __in void* dataLossHandler,
    __in TxnReplicator_Settings const* replicatorSettings,
    __in LPCWSTR configPackageName,
    __in LPCWSTR replicatorSettingsSectionName,
    __in LPCWSTR replicatorSecuritySectionName,
    __out PrimaryReplicatorHandle* primaryReplicator,
    __out TxnReplicatorHandle* txnReplicatorHandle) noexcept
{
    HRESULT hr = S_OK;
    Common::ScopedHeap heap;
    FABRIC_REPLICATOR_SETTINGS const * fabricReplicatorSettings = nullptr;
    TRANSACTIONAL_REPLICATOR_SETTINGS const* transactionalReplicatorSettings = nullptr;
    KTLLOGGER_SHARED_LOG_SETTINGS const* ktlLoggerSharedLogSettings = nullptr;
    ReplicatorConfigSettingsResult configSettingsResult;
    Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr;

    FABRIC_SERVICE_PARTITION_INFORMATION const * partitionInfo;
    hr = reinterpret_cast<IFabricStatefulServicePartition*>(statefulServicePartition)->GetPartitionInfo(&partitionInfo);
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            Common::Guid::Empty(),
            0,
            L"Failed to GetPartitionInfo in GetTransactionalReplicator",
            hr);

        return hr;
    }

    KGuid pId(Common::ServiceInfoUtility::GetPartitionId(partitionInfo));
    KtlSystem* ktlSystem;
    hr = GetKtlSystem((IFabricStatefulServicePartition*)statefulServicePartition, &ktlSystem);
    if (FAILED(hr))
        return hr;

    KAllocator& allocator = ktlSystem->PagedAllocator();
    PartitionedReplicaId::SPtr prId = PartitionedReplicaId::Create(
        pId,
        replicaId,
        allocator);
    if (prId == nullptr)
    {
        RCREventSource::Events->ExceptionError(
            Common::Guid(pId),
            replicaId,
            L"Failed to Create PartitionedReplicaId due to insufficient memory in GetTransactionalReplicator",
            E_OUTOFMEMORY);

        return E_OUTOFMEMORY;
    }

    TRY_COM_PARSE_PUBLIC_STRING_OUT(configPackageName, configPackage, true) // AllowNull
    TRY_COM_PARSE_PUBLIC_STRING_OUT(replicatorSettingsSectionName, replicatorSettingsSection, true) // AllowNull
    TRY_COM_PARSE_PUBLIC_STRING_OUT(replicatorSecuritySectionName, replicatorSecuritySection, true) // AllowNull

    RCREventSource::Events->CreateTransactionalReplicator(prId->TracePartitionId, prId->ReplicaId, replicatorSettings != nullptr, configPackage, replicatorSettingsSection, replicatorSecuritySection);

    hr = ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, codePackageActivationContextCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"Failed to get activation context in GetTransactionalReplicator",
            hr);
        return hr;
    }

    if (replicatorSettings == nullptr)
    {
        hr = LoadReplicatorSettingsFromConfigPackage(
            const_cast<PartitionedReplicaId const &>(*prId),
            heap,
            configSettingsResult,
            configPackage,
            replicatorSettingsSection,
            replicatorSecuritySection,
            &fabricReplicatorSettings,
            &transactionalReplicatorSettings,
            &ktlLoggerSharedLogSettings);

        if (FAILED(hr))
        {
            RCREventSource::Events->ConfigPackageLoadFailed(prId->TracePartitionId, prId->ReplicaId, configPackage, replicatorSettingsSection, replicatorSecuritySection, hr);
            return hr;
        }
    } 
    else 
    { 
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> _fabricReplicatorSettings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS>();
        Common::ReferencePointer<TRANSACTIONAL_REPLICATOR_SETTINGS> _transactionReplicatorSettings = heap.AddItem<TRANSACTIONAL_REPLICATOR_SETTINGS>();
        Common::ReferencePointer<KTLLOGGER_SHARED_LOG_SETTINGS> _ktlLoggerSharedLogSettings = heap.AddItem<KTLLOGGER_SHARED_LOG_SETTINGS>();

        // Initialize before calling ReplicatorSettingsToPublicApiSettings
        _fabricReplicatorSettings->Flags = FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SETTINGS_NONE;
        _transactionReplicatorSettings->Flags = FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_FLAGS::FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_NONE;
        auto sharedLogSettings = KtlLoggerSharedLogSettings::GetDefaultReliableServiceSettings();
        sharedLogSettings->ToPublicApi(heap, *_ktlLoggerSharedLogSettings);

        ReplicatorSettingsToPublicApiSettings(
            heap, 
            replicatorSettings, 
            _fabricReplicatorSettings.GetRawPointer(), 
            _transactionReplicatorSettings.GetRawPointer(), 
            _ktlLoggerSharedLogSettings.GetRawPointer());

        fabricReplicatorSettings = _fabricReplicatorSettings.GetRawPointer();
        transactionalReplicatorSettings = _transactionReplicatorSettings.GetRawPointer();
        ktlLoggerSharedLogSettings = _ktlLoggerSharedLogSettings.GetRawPointer();
    }

    hr = GetTxnReplicatorInternal(
        statefulServicePartition,
        dataLossHandler,
        fabricReplicatorSettings,
        transactionalReplicatorSettings,
        ktlLoggerSharedLogSettings,
        primaryReplicator,
        txnReplicatorHandle);
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"GetTxnReplicator api failed in GetTransactionalReplicator",
            hr);
        return hr;
    }

    // If valid config package name provided then hookup config package change handler
    if (!configPackage.empty())
    {
        Common::ComPointer<IFabricConfigurationPackageChangeHandler> comConfigurationPackageChangeHandler;
        LONGLONG callbackHandle;
        hr = ConfigurationPackageChangeHandler::Create(
            *prId,
            reinterpret_cast<IFabricStatefulServicePartition*>(statefulServicePartition),
            reinterpret_cast<IFabricPrimaryReplicator*>(*primaryReplicator),
            configPackage,
            replicatorSettingsSection,
            replicatorSecuritySection,
            allocator, 
            comConfigurationPackageChangeHandler);
        if (FAILED(hr))
        {
            RCREventSource::Events->ExceptionError(
                prId->TracePartitionId,
                prId->ReplicaId,
                L"Failed to create ConfigurationPackageChangeHandler in GetTransactionalReplicator",
                hr);
            return hr;
        }

        hr = codePackageActivationContextCPtr->RegisterConfigurationPackageChangeHandler(
            comConfigurationPackageChangeHandler.GetRawPointer(),
            &callbackHandle);
        if (FAILED(hr))
        {
            RCREventSource::Events->ExceptionError(
                prId->TracePartitionId,
                prId->ReplicaId,
                L"RegisterConfigurationPackageChangeHandler failed in GetTransactionalReplicator",
                hr);
            return hr;
        }
    }
    return hr;
}

extern "C" HRESULT PrimaryReplicator_UpdateReplicatorSettings(
    __in PrimaryReplicatorHandle primaryReplicator,
    __in TxnReplicator_Settings const* replicatorSettings)
{
    Common::ScopedHeap heap;
    IFabricPrimaryReplicator* fabricPrimaryReplicator = reinterpret_cast<IFabricPrimaryReplicator*>(primaryReplicator);
    IFabricTransactionalReplicator* fabricTransactionalReplicator = dynamic_cast<IFabricTransactionalReplicator*>(fabricPrimaryReplicator);
    if (fabricTransactionalReplicator == nullptr)
        return E_INVALIDARG;

    Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> fabricReplicatorSettings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS>();
    // Initialize before calling ReplicatorSettingsToPublicApiSettings
    fabricReplicatorSettings->Flags = FABRIC_REPLICATOR_SETTINGS_FLAGS::FABRIC_REPLICATOR_SETTINGS_NONE;

    ReplicatorSettingsToPublicApiSettings(
        heap,
        replicatorSettings,
        fabricReplicatorSettings.GetRawPointer(),
        nullptr,
        nullptr);

    return fabricTransactionalReplicator->UpdateReplicatorSettings(fabricReplicatorSettings.GetRawPointer());
}
