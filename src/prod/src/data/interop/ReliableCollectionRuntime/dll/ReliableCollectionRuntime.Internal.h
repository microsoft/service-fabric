// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

typedef void (*pfnStore_Release)(
    __in StateProviderHandle stateProviderHandle);

typedef HRESULT (*pfnStore_ConditionalGetAsync)(
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
    __out BOOL* synchronousComplete);   


typedef HRESULT (*pfnStore_AddAsync)(
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
    __out BOOL* synchronousComplete);


typedef HRESULT (*pfnStore_ConditionalUpdateAsync)(
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
    __out BOOL* synchronousComplete);    


typedef HRESULT (*pfnStore_ConditionalRemoveAsync)(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in int64_t conditionalVersion,
    __out BOOL* removed,
    __in fnNotifyRemoveAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnStore_GetCount)(
    __in StateProviderHandle stateProvider,
    __out int64_t* count);

typedef HRESULT (*pfnStore_CreateKeyEnumeratorAsync)(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyEnumeratorHandle* enumerator,
    __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef	HRESULT (*pfnStore_CreateEnumeratorAsync)(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT(*pfnStore_CreateRangedEnumeratorAsync)(
    __in StoreHandle store,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT(*pfnStore_ContainsKeyAsync)(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in int64_t timeout,
    __in Store_LockMode lockMode,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* found,
    __in fnNotifyContainsKeyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnConcurrentQueue_EnqueueAsync)(
    __in StateProviderHandle concurrentQueue,
    __in TransactionHandle txn,
    __in size_t objectHandle,               // handle of object to be stored  
    __in void* bytes,                       // serialized byte array of object
    __in uint32_t bytesLength,              // byte array length
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnConcurrentQueue_TryDequeueAsync)(
    __in StateProviderHandle concurrentQueue,
    __in TransactionHandle txn,
    __in int64_t timeout,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* succeeded,
    __in fnNotifyTryDequeueAsyncCompletion callback,      // callback to be called when call does not complete synchronously
    __in void* ctx,                      // Context to be passed in when making callback
    __out BOOL* synchronousComplete);    // if call completed synchronously

typedef HRESULT (*pfnConcurrentQueue_GetCount)(
    __in StateProviderHandle concurrentQueue,
    __out int64_t* count);

typedef void (*pfnTransaction_Release)(
    __in TransactionHandle txn);

typedef void (*pfnTransaction_Dispose)(
    __in TransactionHandle txn);

typedef HRESULT (*pfnTransaction_CommitAsync)(
    __in TransactionHandle txn,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTransaction_Abort)(
    __in TransactionHandle txn);

typedef HRESULT (*pfnTransaction_GetInfo)(__in TransactionHandle txnHandle, __out Transaction_Info *info);

typedef HRESULT(*pfnTransaction_GetVisibilitySequenceNumberAsync)(
    __in TransactionHandle txnHandle,
    __out int64_t *sequenceNumber,
    __in fnNotifyGetVisibilitySequenceNumberCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_CreateTransaction)(
    __in TxnReplicatorHandle txnReplicator,
    __out TransactionHandle* txn);

typedef HRESULT (*pfnGetTxnReplicator)(
    __in int64_t replicaId,
    __in void* statefulServicePartition,
    __in void* dataLossHandler,
    __in TxnReplicator_Settings const* replicatorSettings,
    __in LPCWSTR configPackageName,
    __in LPCWSTR replicatorSettingsSectionName,
    __in LPCWSTR replicatorSecuritySectionName,
    __out PrimaryReplicatorHandle* primaryReplicator,
    __out TxnReplicatorHandle* txnReplicatorHandle);

typedef void (*pfnTxnReplicator_Release)(
    __in TxnReplicatorHandle txnReplicator);

typedef HRESULT(*pfnTxnReplicator_GetOrAddStateProviderAsync)(
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
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_AddStateProviderAsync)(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in LPCWSTR lang,
    __in StateProvider_Info* stateProviderInfo,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_RemoveStateProviderAsync)(
    __in TxnReplicatorHandle txnReplicator,
    __in TransactionHandle txn,
    __in LPCWSTR name,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL*synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_CreateEnumerator)(
    __in TxnReplicatorHandle txnReplicator,
    __in BOOL parentsOnly,
    __out StateProviderEnumeratorHandle* enumerator);

typedef HRESULT (*pfnTxnReplicator_GetStateProvider)(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR name,
    __out StateProviderHandle* stateProvider);

typedef HRESULT (*pfnTxnReplicator_BackupAsync)(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT(*pfnTxnReplicator_BackupAsync2)(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_RestoreAsync)(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR backupFolder,
    __in Restore_Policy restorePolicy,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle*  cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef HRESULT (*pfnTxnReplicator_GetInfo)(
    __in TxnReplicatorHandle txnReplicator,
    __out TxnReplicator_Info* info);

typedef HRESULT (*pfnPrimaryReplicator_UpdateReplicatorSettings)(
    __in PrimaryReplicatorHandle primaryReplicator,
    __in TxnReplicator_Settings const* replicatorSettings);

typedef void (*pfnCancellationTokenSource_Cancel)(
    CancellationTokenSourceHandle cts);

typedef void (*pfnCancellationTokenSource_Release)(
    CancellationTokenSourceHandle cts);


typedef void (*pfnStateProviderEnumerator_Release)(
    __in StateProviderEnumeratorHandle enumerator);

typedef HRESULT (*pfnStateProviderEnumerator_MoveNext)(
    __in StateProviderEnumeratorHandle enumerator,
    __out BOOL* advanced,
    __out LPCWSTR* providerName,
    __out StateProviderHandle* provider);


typedef void (*pfnStoreKeyEnumerator_Release)(
    StoreKeyEnumeratorHandle enumerator);

typedef HRESULT (*pfnStoreKeyEnumerator_MoveNext)(
    __in StoreKeyEnumeratorHandle enumerator,
    __out BOOL* advanced,
    __out LPCWSTR* key);


typedef void (*pfnStoreKeyValueEnumerator_Release)(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator);

typedef HRESULT (*pfnStoreKeyValueEnumerator_MoveNextAsync)(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* advanced,
    __out LPCWSTR* key,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out int64_t* versionSequenceNumber,
    __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete);

typedef void (*pfnBuffer_Release)(
    __in BufferHandle handle);

typedef void (*pfnTest_UseEnv)(BOOL enable);

typedef HRESULT (*pfnStore_SetNotifyStoreChangeCallback)(
    __in StateProviderHandle stateProvider,
    __in fnNotifyStoreChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx);

typedef HRESULT(*pfnTxnReplicator_SetNotifyStateManagerChangeCallback)(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx);

typedef HRESULT(*pfnTxnReplicator_SetNotifyTransactionChangeCallback)(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyTransactionChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx);

typedef HRESULT (*pfnStore_SetNotifyStoreChangeCallbackMask)(
    __in StateProviderHandle stateProvider,
    __in NotifyStoreChangeCallbackMask mask);

typedef void (*pfnStore_AddRef)(__in StateProviderHandle handle);
typedef void (*pfnTransaction_AddRef)(__in TransactionHandle handle);
typedef void (*pfnStoreKeyEnumerator_AddRef)(__in StoreKeyEnumeratorHandle handle);
typedef void (*pfnStateProviderEnumerator_AddRef)(__in StateProviderEnumeratorHandle handle);

typedef HRESULT (*pfnStateProvider_GetInfo)(
    __in StateProviderHandle stateProviderHandle,
    __in LPCWSTR lang,
    __out StateProvider_Info* stateProvider_Info);
typedef void (*pfnStateProvider_AddRef)(__in StateProviderHandle stateProviderHandle);
typedef void (*pfnStateProvider_Release)(__in StateProviderHandle stateProviderHandle);

struct ReliableCollectionApis
{
    pfnStore_Release Store_Release;
    pfnStore_ConditionalGetAsync Store_ConditionalGetAsync;
    pfnStore_AddAsync Store_AddAsync;
    pfnStore_ConditionalUpdateAsync Store_ConditionalUpdateAsync;
    pfnStore_ConditionalRemoveAsync Store_ConditionalRemoveAsync;
    pfnStore_GetCount Store_GetCount;
    pfnStore_CreateKeyEnumeratorAsync Store_CreateKeyEnumeratorAsync;
    pfnStore_CreateEnumeratorAsync Store_CreateEnumeratorAsync;
    pfnTransaction_Release Transaction_Release;
    pfnTransaction_CommitAsync Transaction_CommitAsync;
    pfnTransaction_Abort Transaction_Abort;
    pfnTxnReplicator_CreateTransaction TxnReplicator_CreateTransaction;
    pfnGetTxnReplicator GetTxnReplicator;
    pfnTxnReplicator_Release TxnReplicator_Release;
    pfnTxnReplicator_GetOrAddStateProviderAsync TxnReplicator_GetOrAddStateProviderAsync;
    pfnTxnReplicator_AddStateProviderAsync TxnReplicator_AddStateProviderAsync;
    pfnTxnReplicator_RemoveStateProviderAsync TxnReplicator_RemoveStateProviderAsync;
    pfnTxnReplicator_CreateEnumerator TxnReplicator_CreateEnumerator;
    pfnTxnReplicator_GetStateProvider TxnReplicator_GetStateProvider;
    pfnTxnReplicator_BackupAsync TxnReplicator_BackupAsyn;
    pfnTxnReplicator_RestoreAsync TxnReplicator_RestoreAsync;
    pfnCancellationTokenSource_Cancel CancellationTokenSource_Cancel;
    pfnCancellationTokenSource_Release CancellationTokenSource_Release;
    pfnStateProviderEnumerator_Release StateProviderEnumerator_Release;
    pfnStateProviderEnumerator_MoveNext StateProviderEnumerator_MoveNext;
    pfnStoreKeyEnumerator_Release StoreKeyEnumerator_Release;
    pfnStoreKeyEnumerator_MoveNext StoreKeyEnumerator_MoveNext;
    pfnStoreKeyValueEnumerator_Release StoreKeyValueEnumerator_Release;
    pfnStoreKeyValueEnumerator_MoveNextAsync StoreKeyValueEnumerator_MoveNextAsync;
    pfnTest_UseEnv Test_UseEnv;
    pfnBuffer_Release Buffer_Release;
    pfnStore_SetNotifyStoreChangeCallback Store_SetNotifyStoreChangeCallback;
    pfnTxnReplicator_SetNotifyStateManagerChangeCallback TxnReplicator_SetNotifyStateManagerChangeCallback;
    pfnTxnReplicator_SetNotifyTransactionChangeCallback TxnReplicator_SetNotifyTransactionChangeCallback;
    pfnTransaction_AddRef Transaction_AddRef;
    pfnStore_AddRef Store_AddRef;
    pfnStoreKeyEnumerator_AddRef StoreKeyEnumerator_AddRef;
    pfnStateProviderEnumerator_AddRef StateProviderEnumerator_AddRef;
    pfnStore_SetNotifyStoreChangeCallbackMask Store_SetNotifyStoreChangeCallbackMask;
    pfnStateProvider_GetInfo StateProvider_GetInfo;
    pfnStateProvider_AddRef StateProvider_AddRef;
    pfnStateProvider_Release StateProvider_Release;
    pfnTransaction_GetInfo Transaction_GetInfo;
    pfnTransaction_GetVisibilitySequenceNumberAsync Transaction_GetVisibilitySequenceNumberAsync;
    pfnTransaction_Dispose Transaction_Dispose;
    pfnStore_CreateRangedEnumeratorAsync Store_CreateRangedEnumeratorAsync;
    pfnStore_ContainsKeyAsync Store_ContainsKeyAsync;
    pfnTxnReplicator_GetInfo TxnReplicator_GetInfo;
    pfnPrimaryReplicator_UpdateReplicatorSettings PrimaryReplicator_UpdateReplicatorSettings;
    pfnConcurrentQueue_EnqueueAsync ConcurrentQueue_EnqueueAsync;
    pfnConcurrentQueue_TryDequeueAsync ConcurrentQueue_TryDequeueAsync;
    pfnConcurrentQueue_GetCount ConcurrentQueue_GetCount;
    pfnTxnReplicator_BackupAsync2 TxnReplicator_BackupAsync2;
};

extern "C" HRESULT FabricGetReliableCollectionApiTable(
    __in uint16_t apiVersion,
    __out ReliableCollectionApis* reliableCollectionApis);
typedef HRESULT (*pfnFabricGetReliableCollectionApiTable)(uint16_t apiVersion, ReliableCollectionApis* reliableCollectionApis);
