// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

#ifdef _WIN32
#define RELIABLECOLLECTION_CLUSTER_DLL L"FabricRuntime.dll"
#define RELIABLECOLLECTION_STANDALONE_DLL L"ReliableCollectionServiceStandalone.dll"
#else 
#define RELIABLECOLLECTION_CLUSTER_DLL L"libFabricRuntime.so"
#define RELIABLECOLLECTION_STANDALONE_DLL L"libReliableCollectionServiceStandalone.so"
#endif

static ReliableCollectionApis g_reliableCollectionApis;

#ifdef _WIN32
extern "C" HRESULT ReliableCollectionRuntime_Initialize(uint16_t apiVersion)
{
    HMODULE module;
    wstring env;
    std::wstring currentModuleFullName;
    std::wstring fullName;
    Environment::GetCurrentModuleFileName(currentModuleFullName);
    std::wstring directoryName = Path::GetDirectoryName(currentModuleFullName);

    if (Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_STANDALONE", env, NOTHROW()))
        fullName = Path::Combine(directoryName, RELIABLECOLLECTION_STANDALONE_DLL);
    else
        fullName = Path::Combine(directoryName, RELIABLECOLLECTION_CLUSTER_DLL);

    module = ::LoadLibrary(fullName.c_str());
    if (module == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    pfnFabricGetReliableCollectionApiTable pfnGetReliableCollectionApiTable = (pfnFabricGetReliableCollectionApiTable)GetProcAddress(module, "FabricGetReliableCollectionApiTable");
    if (pfnGetReliableCollectionApiTable == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    pfnGetReliableCollectionApiTable(apiVersion, &g_reliableCollectionApis);
    return S_OK;
}
#else
#include <dlfcn.h>
extern "C" HRESULT ReliableCollectionRuntime_Initialize(uint16_t apiVersion)
{
    void* module = nullptr;
    wstring env;
    std::wstring currentModuleFullName;
    std::wstring fullName;
    Environment::GetCurrentModuleFileName(currentModuleFullName);
    std::wstring directoryName = Path::GetDirectoryName(currentModuleFullName);

    if (Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_STANDALONE", env, NOTHROW()))
        // shared library is already loaded in the process
        fullName = wstring(RELIABLECOLLECTION_STANDALONE_DLL);
    else
        fullName = Path::Combine(directoryName, RELIABLECOLLECTION_CLUSTER_DLL);

    string name = Utf16To8(fullName.c_str());
    module = dlopen(name.c_str(), RTLD_NOW);
    if (module == nullptr)
        return HRESULT_FROM_WIN32(GetLastError());

    pfnFabricGetReliableCollectionApiTable pfnGetReliableCollectionApiTable = (pfnFabricGetReliableCollectionApiTable)dlsym(module, "FabricGetReliableCollectionApiTable");
    if (pfnGetReliableCollectionApiTable == nullptr)
        return HRESULT_FROM_WIN32(GetLastError());

    pfnGetReliableCollectionApiTable(apiVersion, &g_reliableCollectionApis);
    return S_OK;
}
#endif

extern "C" void Store_Release(
        __in StateProviderHandle stateProviderHandle)
{
    g_reliableCollectionApis.Store_Release(stateProviderHandle);
}

extern "C" void Store_AddRef(
        __in StateProviderHandle stateProviderHandle)
{
    g_reliableCollectionApis.Store_AddRef(stateProviderHandle);
}

extern "C" HRESULT Store_ConditionalGetAsync(
    __in StateProviderHandle stateProvider,
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
    return g_reliableCollectionApis.Store_ConditionalGetAsync(
        stateProvider,
        txn,
        key,
        timeout,
        lockMode,
        objectHandle,
        value,
        versionSequenceNumber,
        cts,
        found,
        callback,
        ctx,
        synchronousComplete);
}


extern "C" HRESULT Store_AddAsync(
    __in StateProviderHandle stateProvider,
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
    return g_reliableCollectionApis.Store_AddAsync(
        stateProvider,
        txn,
        key,
        objectHandle,
        bytes,
        bytesLength,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_ConditionalUpdateAsync(
    __in StateProviderHandle stateProvider,
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
    return g_reliableCollectionApis.Store_ConditionalUpdateAsync(
        stateProvider,
        txn,
        key,
        objectHandle,
        bytes,
        bytesLength,
        timeout,
        cts,
        conditionalVersion,
        updated,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_ConditionalRemoveAsync(
    __in StateProviderHandle stateProvider,
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
    return g_reliableCollectionApis.Store_ConditionalRemoveAsync(
        stateProvider,
        txn,
        key,
        timeout,
        cts,
        conditionalVersion,
        removed,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_GetCount(
    __in StateProviderHandle stateProvider,
    __out int64_t* count)
{
    return g_reliableCollectionApis.Store_GetCount(stateProvider, count);
}

extern "C" HRESULT Store_CreateKeyEnumeratorAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyEnumeratorHandle* enumerator,
    __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Store_CreateKeyEnumeratorAsync(
        stateProvider,
        txn,
        firstKey,
        lastKey,
        enumerator,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_CreateEnumeratorAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Store_CreateEnumeratorAsync(
        stateProvider,
        txn,
        enumerator,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_CreateRangedEnumeratorAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Store_CreateRangedEnumeratorAsync(
        stateProvider,
        txn,
        firstKey,
        lastKey,
        enumerator,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_ContainsKeyAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __in Store_LockMode lockMode,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* found,
    __in fnNotifyContainsKeyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Store_ContainsKeyAsync(
        stateProvider,
        txn,
        key,
        timeout,
        lockMode,
        cts,
        found,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Store_SetNotifyStoreChangeCallback(
    __in StateProviderHandle stateProvider,
    __in fnNotifyStoreChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    return g_reliableCollectionApis.Store_SetNotifyStoreChangeCallback(stateProvider, callback, cleanupCallback, ctx);
}

CLASS_DECLSPEC HRESULT Store_SetNotifyStoreChangeCallbackMask(
    __in StateProviderHandle stateProvider,
    __in NotifyStoreChangeCallbackMask mask)
{
    return g_reliableCollectionApis.Store_SetNotifyStoreChangeCallbackMask(stateProvider, mask);
}

extern "C" HRESULT TxnReplicator_SetNotifyStateManagerChangeCallback(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    return g_reliableCollectionApis.TxnReplicator_SetNotifyStateManagerChangeCallback(txnReplicator, callback, cleanupCallback, ctx);
}

extern "C" HRESULT TxnReplicator_SetNotifyTransactionChangeCallback(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyTransactionChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    return g_reliableCollectionApis.TxnReplicator_SetNotifyTransactionChangeCallback(txnReplicator, callback, cleanupCallback, ctx);
}


extern "C" void Transaction_Release(
    __in TransactionHandle txn) noexcept
{
    g_reliableCollectionApis.Transaction_Release(txn);
}

extern "C" void Transaction_Release2(
    __in TransactionHandle txn) noexcept
{
    g_reliableCollectionApis.Transaction_Release2(txn);
}

extern "C" void Transaction_Dispose(
    __in TransactionHandle txn) noexcept
{
    g_reliableCollectionApis.Transaction_Dispose(txn);
}

extern "C" void Transaction_AddRef(
    __in TransactionHandle txn) noexcept
{
    g_reliableCollectionApis.Transaction_AddRef(txn);
}

extern "C" HRESULT Transaction_CommitAsync(
    __in TransactionHandle txn,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Transaction_CommitAsync(
        txn,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT Transaction_Abort(
    __in TransactionHandle txn) noexcept
{
    return g_reliableCollectionApis.Transaction_Abort(txn);
}

extern "C" HRESULT Transaction_GetInfo(__in TransactionHandle txnHandle, __out Transaction_Info *info)
{
    return g_reliableCollectionApis.Transaction_GetInfo(txnHandle, info);
}

extern "C" HRESULT Transaction_GetVisibilitySequenceNumberAsync(
    __in TransactionHandle txnHandle,
    __out int64_t *sequenceNumber,
    __in fnNotifyGetVisibilitySequenceNumberCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.Transaction_GetVisibilitySequenceNumberAsync(txnHandle, sequenceNumber, callback, ctx, synchronousComplete);
}

extern "C" HRESULT TxnReplicator_CreateTransaction(
    __in TxnReplicatorHandle txnReplicator,
    __out TransactionHandle* txn) noexcept
{
    return g_reliableCollectionApis.TxnReplicator_CreateTransaction(
        txnReplicator,
        txn);
}

extern "C" void TxnReplicator_Release(
    __in TxnReplicatorHandle txnReplicator)
{
    g_reliableCollectionApis.TxnReplicator_Release(txnReplicator);
}

extern "C" HRESULT GetTxnReplicator(
    __in void* statefulServicePartition,
    __in void* dataLossHandler,
    __in void const* fabricReplicatorSettings,
    __in void const* txnReplicatorSettings,
    __in void const* ktlLoggerSharedSettings,
    __out void** primaryReplicator,
    __out TxnReplicatorHandle* txnReplicatorHandle) noexcept
{
    return g_reliableCollectionApis.GetTxnReplicator(
        statefulServicePartition,
        dataLossHandler,
        fabricReplicatorSettings,
        txnReplicatorSettings,
        ktlLoggerSharedSettings,
        primaryReplicator,
        txnReplicatorHandle);
}

extern "C" HRESULT GetTransactionalReplicator(
    __in void* statefulServicePartition,
    __in void* dataLossHandler,
    __in const TxnReplicator_Settings* replicatorSettings,
    __in LPCWSTR configPackageName,
    __in LPCWSTR replicatorSettingsSectionName,
    __out void** primaryReplicator,
    __out TxnReplicatorHandle* txnReplicatorHandle) noexcept
{
    return g_reliableCollectionApis.GetTransactionalReplicator(
        statefulServicePartition,
        dataLossHandler,
        replicatorSettings,
        configPackageName,
        replicatorSettingsSectionName,
        primaryReplicator,
        txnReplicatorHandle);
}

extern "C" HRESULT TxnReplicator_GetOrAddStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __out StateProviderHandle* stateProvider,
    __out BOOL* alreadyExist,
    __in fnNotifyGetOrAddStateProviderAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_GetOrAddStateProviderAsync(
        txnReplicator,
        txn,
        name,
        timeout,
        cts,
        stateProvider,
        alreadyExist,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT TxnReplicator_AddStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_AddStateProviderAsync(
        txnReplicator,
        txn,
        name,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}


extern "C" HRESULT TxnReplicator_RemoveStateProviderAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL*synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_RemoveStateProviderAsync(
        txnReplicator,
        txn,
        name,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT TxnReplicator_CreateEnumerator(
    __in TxnReplicatorHandle txnReplicator,
    __in BOOL parentsOnly,
    __out StateProviderEnumeratorHandle* enumerator)
{
    return g_reliableCollectionApis.TxnReplicator_CreateEnumerator(
        txnReplicator,
        parentsOnly,
        enumerator);
}

extern "C" HRESULT TxnReplicator_GetStateProvider(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR name,
    __out StateProviderHandle* stateProvider) noexcept
{
    return g_reliableCollectionApis.TxnReplicator_GetStateProvider(
        txnReplicator,
        name,
        stateProvider);
}

HRESULT TxnReplicator_BackupAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_BackupAsyn(
        txnReplicator,
        uploadAsyncCallback,
        backupOption,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

HRESULT TxnReplicator_RestoreAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR backupFolder,
    __in Restore_Policy restorePolicy,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle*  cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_RestoreAsync(
        txnReplicator,
        backupFolder,
        restorePolicy,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" void CancellationTokenSource_Cancel(
    CancellationTokenSourceHandle cts)
{
    g_reliableCollectionApis.CancellationTokenSource_Cancel(cts);
}

extern "C" void CancellationTokenSource_Release(
    CancellationTokenSourceHandle cts)
{
    g_reliableCollectionApis.CancellationTokenSource_Release(cts);
}

extern "C" void StateProviderEnumerator_Release(
    __in StateProviderEnumeratorHandle enumerator)
{
    g_reliableCollectionApis.StateProviderEnumerator_Release(enumerator);
}

extern "C" void StateProviderEnumerator_AddRef(
    __in StateProviderEnumeratorHandle enumerator)
{
    g_reliableCollectionApis.StateProviderEnumerator_AddRef(enumerator);
}

extern "C" HRESULT StateProviderEnumerator_MoveNext(
    __in StateProviderEnumeratorHandle enumerator,
    __out BOOL* advanced,
    __out LPCWSTR* providerName,
    __out StateProviderHandle* provider)
{
    return g_reliableCollectionApis.StateProviderEnumerator_MoveNext(
        enumerator,
        advanced,
        providerName,
        provider);
}


extern "C" void StoreKeyEnumerator_Release(
    StoreKeyEnumeratorHandle enumerator)
{
    g_reliableCollectionApis.StoreKeyEnumerator_Release(enumerator);
}

extern "C" void StoreKeyEnumerator_AddRef(
    StoreKeyEnumeratorHandle enumerator)
{
    g_reliableCollectionApis.StoreKeyEnumerator_AddRef(enumerator);
}

extern "C" HRESULT StoreKeyEnumerator_MoveNext(
    __in StoreKeyEnumeratorHandle enumerator,
    __out BOOL* advanced,
    __out LPCWSTR* key)
{
    return g_reliableCollectionApis.StoreKeyEnumerator_MoveNext(
        enumerator,
        advanced,
        key);
}

extern "C" void StoreKeyValueEnumerator_Release(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator)
{
    g_reliableCollectionApis.StoreKeyValueEnumerator_Release(enumerator);
}

extern "C" HRESULT StoreKeyValueEnumerator_MoveNextAsync(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* advanced,
    __out LPCWSTR* key,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out int64_t* versionSequenceNumber,
    __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.StoreKeyValueEnumerator_MoveNextAsync(
        enumerator,
        cts,
        advanced,
        key,
        objectHandle,
        value,
        versionSequenceNumber,
        callback,
        ctx,
        synchronousComplete);
}


extern "C" void Test_UseEnv(BOOL enable)
{
    g_reliableCollectionApis.Test_UseEnv(enable);
}

extern "C" void Buffer_Release(BufferHandle handle)
{
    g_reliableCollectionApis.Buffer_Release(handle);
}

extern "C" HRESULT StateProvider_GetInfo(
    __in StateProviderHandle stateProviderHandle,
    __in LPCWSTR lang,
    __out StateProvider_Info* stateProvider_Info)
{
    return g_reliableCollectionApis.StateProvider_GetInfo(
        stateProviderHandle,
        lang,
        stateProvider_Info);
}

extern "C" void StateProvider_AddRef(
    __in StateProviderHandle stateProviderHandle)
{
    g_reliableCollectionApis.StateProvider_AddRef(stateProviderHandle);
}

extern "C" void StateProvider_Release(
    __in StateProviderHandle stateProviderHandle)
{
    g_reliableCollectionApis.StateProvider_Release(stateProviderHandle);
}

extern "C" HRESULT TxnReplicator_GetOrAddStateProviderAsync2(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in LPCWSTR lang,
    __in StateProvider_Info* stateProviderInfo,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __out StateProviderHandle* stateProvider,
    __out BOOL* alreadyExist,
    __in fnNotifyGetOrAddStateProviderAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_GetOrAddStateProviderAsync2(
        txnReplicator,
        txn,
        name,
        lang,
        stateProviderInfo,
        timeout,
        cts,
        stateProvider,
        alreadyExist,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT TxnReplicator_AddStateProviderAsync2(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in LPCWSTR lang,
    __in StateProvider_Info* stateProviderInfo,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_AddStateProviderAsync2(
        txnReplicator,
        txn,
        name,
        lang,
        stateProviderInfo,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

