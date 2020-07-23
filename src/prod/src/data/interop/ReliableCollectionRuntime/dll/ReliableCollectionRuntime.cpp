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

StringLiteral const TraceComponent("ReliableCollectionRuntime");

static ReliableCollectionApis g_reliableCollectionApis;

extern "C" HRESULT ReliableCollectionRuntime_StartTraceSessions()
{
    auto error =  Common::TraceSession::Instance()->StartTraceSessions();
    if (!error.IsSuccess())
    {
        return error.ToHResult();
    }

    Trace.WriteNoise(TraceComponent, "Trace sessions successfully started");
    return S_OK;
}

extern "C" HRESULT ReliableCollectionRuntime_Initialize(uint16_t apiVersion)
{
    return ReliableCollectionRuntime_Initialize2(apiVersion, /*standaloneMode*/ false);
}

#ifdef _WIN32
extern "C" HRESULT ReliableCollectionRuntime_Initialize2(uint16_t apiVersion, BOOL standAloneMode)
{
    HMODULE module;
    std::wstring currentModuleFullName;
    std::wstring fullName;
    Environment::GetCurrentModuleFileName(currentModuleFullName);
    std::wstring directoryName = Path::GetDirectoryName(currentModuleFullName);
    
    Trace.WriteInfo(TraceComponent, "[ReliableCollectionRuntime_Initialize2] apiVersion={0} standAloneMode={1}", apiVersion, standAloneMode);

    if (standAloneMode)
        fullName = Path::Combine(directoryName, RELIABLECOLLECTION_STANDALONE_DLL); // This is app local, so need to loaded from current directory
    else
        fullName = wstring(RELIABLECOLLECTION_CLUSTER_DLL);

    module = ::LoadLibrary(fullName.c_str());
    if (module == NULL)
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] LoadLibrary failed for filename={0} error={1}", fullName, GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pfnFabricGetReliableCollectionApiTable pfnGetReliableCollectionApiTable = (pfnFabricGetReliableCollectionApiTable)GetProcAddress(module, "FabricGetReliableCollectionApiTable");
    if (pfnGetReliableCollectionApiTable == NULL)
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] GetProcAddress failed error={0}", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT status = pfnGetReliableCollectionApiTable(apiVersion, &g_reliableCollectionApis);
    if (FAILED(status))
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] FabricGetReliableCollectionApiTable failed error={0}", status);
    }

    return status;
}
#else
#include <dlfcn.h>
extern "C" HRESULT ReliableCollectionRuntime_Initialize2(uint16_t apiVersion, BOOL standAloneMode)
{
    void* module = nullptr;
    std::wstring currentModuleFullName;
    std::wstring fullName;
    Environment::GetCurrentModuleFileName(currentModuleFullName);
    std::wstring directoryName = Path::GetDirectoryName(currentModuleFullName);

    Trace.WriteInfo(TraceComponent, "[ReliableCollectionRuntime_Initialize2] apiVersion={0} standAloneMode={1}", apiVersion, standAloneMode);

    if (standAloneMode)
        fullName = Path::Combine(directoryName, RELIABLECOLLECTION_STANDALONE_DLL);
    else
        fullName = wstring(RELIABLECOLLECTION_CLUSTER_DLL);

    string name = Utf16To8(fullName.c_str());
    module = dlopen(name.c_str(), RTLD_LAZY);
    if (module == nullptr)
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] dlopen failed for filename={0} error={1}", name, GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pfnFabricGetReliableCollectionApiTable pfnGetReliableCollectionApiTable = (pfnFabricGetReliableCollectionApiTable)dlsym(module, "FabricGetReliableCollectionApiTable");
    if (pfnGetReliableCollectionApiTable == nullptr)
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] dlsym failed error={0}", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT status = pfnGetReliableCollectionApiTable(apiVersion, &g_reliableCollectionApis);
    if (FAILED(status))
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionRuntime_Initialize2] FabricGetReliableCollectionApiTable failed error={0}", status);
    }

    return status;
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

extern "C" HRESULT Store_SetNotifyStoreChangeCallbackMask(
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

extern "C" HRESULT TxnReplicator_GetInfo(
    __in TxnReplicatorHandle txnReplicator,
    __out TxnReplicator_Info* info)
{
    return g_reliableCollectionApis.TxnReplicator_GetInfo(
        txnReplicator,
        info);
}

extern "C" HRESULT PrimaryReplicator_UpdateReplicatorSettings(
    __in PrimaryReplicatorHandle primaryReplicator,
    __in TxnReplicator_Settings const* replicatorSettings)
{
    return g_reliableCollectionApis.PrimaryReplicator_UpdateReplicatorSettings(
        primaryReplicator,
        replicatorSettings);
}

extern "C" void Transaction_Release(
    __in TransactionHandle txn) noexcept
{
    g_reliableCollectionApis.Transaction_Release(txn);
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
    return g_reliableCollectionApis.GetTxnReplicator(
        replicaId,
        statefulServicePartition,
        dataLossHandler,
        replicatorSettings,
        configPackageName,
        replicatorSettingsSectionName,
        replicatorSecuritySectionName,
        primaryReplicator,
        txnReplicatorHandle);
}

extern "C" HRESULT TxnReplicator_GetOrAddStateProviderAsync(
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
    return g_reliableCollectionApis.TxnReplicator_GetOrAddStateProviderAsync(
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
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_AddStateProviderAsync(
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

HRESULT TxnReplicator_BackupAsync2(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.TxnReplicator_BackupAsync2(
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

extern "C" HRESULT ConcurrentQueue_EnqueueAsync(
    __in StateProviderHandle concurrentQueue,
    __in TransactionHandle txn,
    __in size_t objectHandle,               // handle of object to be stored  
    __in void* bytes,                       // serialized byte array of object
    __in uint32_t bytesLength,              // byte array length
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.ConcurrentQueue_EnqueueAsync(
        concurrentQueue,
        txn,
        objectHandle,
        bytes,
        bytesLength,
        timeout,
        cts,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT ConcurrentQueue_TryDequeueAsync(
    __in StateProviderHandle concurrentQueue,
    __in TransactionHandle txn,
    __in int64_t timeout,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* succeeded,
    __in fnNotifyTryDequeueAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    return g_reliableCollectionApis.ConcurrentQueue_TryDequeueAsync(
        concurrentQueue,
        txn,
        timeout,
        objectHandle,
        value,
        cts,
        succeeded,
        callback,
        ctx,
        synchronousComplete);
}

extern "C" HRESULT ConcurrentQueue_GetCount(
    __in StateProviderHandle concurrentQueue,
    __out int64_t* count)
{
    return g_reliableCollectionApis.ConcurrentQueue_GetCount(concurrentQueue, count);
}


