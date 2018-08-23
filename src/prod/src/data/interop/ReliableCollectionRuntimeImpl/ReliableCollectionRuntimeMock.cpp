// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable : 4100)  

/*
void MOCK_CLUSTER_Initialize(LPCWSTR serviceTypeName, uint32_t port)
{
}

void AddContextThreadWorker(AddPartitionContextCallback add, KEY_TYPE key, TxnReplicatorHandle hReplicator, PARTITION_HANDLE hPartition)
{
    add(key, hReplicator, hPartition);
}

void MOCK_CLUSTER_ReliableServices_Initialize(AddPartitionContextCallback add, RemovePartitionContextCallback remove)
{
    auto pfnInitTestFunc = ReliableCollections::This()->GetInitializeTestFunc();

    TxnReplicatorHandle hReplicator;
    PARTITION_HANDLE hPartition;
    pfnInitTestFunc(u"NodeReliableCollectionTest", &hReplicator, &hPartition);

    thread addContextWorkerThread(AddContextThreadWorker, add, (KEY_TYPE)0, hReplicator, hPartition);
    addContextWorkerThread.join();
}

void MOCK_Initialize(LPCWSTR serviceTypeName, uint32_t port)
{

}

void MOCK_ReliableServices_Initialize(AddPartitionContextCallback add, RemovePartitionContextCallback remove)
{
    thread addContextWorkerThread(AddContextThreadWorker, add, (KEY_TYPE)0, (TxnReplicatorHandle) 0x100, (PARTITION_HANDLE) 0x100);
    addContextWorkerThread.join();
}
*/
void MOCK_Store_Release(__in StateProviderHandle stateProviderHandle) { }

void MOCK_Transaction_Release(__in TransactionHandle txn) {}

HRESULT MOCK_TxnReplicator_GetOrAddStateProviderAsync(
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
    *alreadyExist = true;
    *stateProvider = (StateProviderHandle)0x100;
    *synchronousComplete = false;
    callback(ctx, S_OK, *stateProvider, true);
    return S_OK;
}

HRESULT MOCK_TxnReplicator_CreateTransaction(
    __in TxnReplicatorHandle txnReplicator,
    __out TransactionHandle* txn)
{
    *txn = (TransactionHandle) 0x200;
    return S_OK;
}

void MOCK_TxnReplicator_Release(
    __in TxnReplicatorHandle txnReplicator)
{
}

HRESULT MOCK_TxnReplicator_BackupAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    Backup_Info backup_info = {};
    uploadAsyncCallback(ctx, backup_info,
        [](void* context, BOOL result) {

        },
        nullptr);
    callback(ctx, S_OK);
    return S_OK;
}

HRESULT MOCK_TxnReplicator_BackupAsync2(
    __in TxnReplicatorHandle txnReplicator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in Backup_Option backupOption,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    Backup_Info2 backup_info = {};
    uploadAsyncCallback(ctx, &backup_info, sizeof(backup_info),
        [](void* context, BOOL result) {

    },
        nullptr);
    callback(ctx, S_OK);
    return S_OK;
}

HRESULT MOCK_TxnReplicator_RestoreAsync(
    __in TxnReplicatorHandle txnReplicator,
    __in LPCWSTR backupFolder,
    __in Restore_Policy restorePolicy,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle*  cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronouscomplete)
{
    if (cts != nullptr)
        *cts = (CancellationTokenSourceHandle)0x900;
    callback(ctx, S_OK);
    return S_OK;
}

HRESULT MOCK_Transaction_CommitAsync(
    __in TransactionHandle txn,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    *synchronousComplete = false;
    callback(ctx, S_OK);
    return S_OK;
}

HRESULT MOCK_Transaction_Abort(
    __in TransactionHandle txn)
{
    return S_OK;
}

map<u16string, vector<char>> g_dict;

HRESULT MOCK_Store_GetCount(
    __in StateProviderHandle stateProvider,
    __out int64_t* count)
{
    *count = g_dict.size();
    return S_OK;
}

HRESULT MOCK_Store_AddAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR key,
    __in size_t objectHandle,               
    __in void* bytes,                    // serailized byte array of object
    __in uint32_t bytesLength,             // byte array length
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    *synchronousComplete = false;
    char* pchBytes = reinterpret_cast<char *>(bytes);
    g_dict.insert(
        std::pair<u16string, vector<char>>(
            (char16_t*)key, 
            vector<char>(pchBytes, pchBytes + bytesLength)));

    callback(ctx, S_OK);
    return S_OK;
}

HRESULT MOCK_Store_ConditionalUpdateAsync(
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
    u16string u16key((char16_t*)key);

    if (g_dict.find(u16key) != g_dict.end())
    {
        *updated = true;
        char *pchBytes = reinterpret_cast<char *>(bytes);
        g_dict[u16key].assign(pchBytes, pchBytes + bytesLength);
    }
    else
    {
        *updated = false;
    }

    *synchronousComplete = false;
    callback(ctx, S_OK, *updated);

    return S_OK;
}

HRESULT MOCK_Store_ConditionalRemoveAsync(
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
    u16string u16key((char16_t*)key);
    
    if (g_dict.find(u16key) != g_dict.end())
    {
        *removed = true;
        g_dict.erase(u16key);
    }
    else
    {
        *removed = false;
    }

    *synchronousComplete = false;
    callback(ctx, S_OK, *removed);

    return S_OK;
}

HRESULT MOCK_Store_ConditionalGetAsync(
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
    u16string u16key((char16_t*)key);

    if (g_dict.find(u16key) != g_dict.end())
    {
        vector<char> &allBytes = g_dict[u16key];
        
        *found = true;
        value->Bytes = allBytes.data();
        value->Length = (uint32_t)allBytes.size();
    }
    else
    {
        *found = false;
        value->Bytes = NULL;
        value->Length = 0;
    }

    *versionSequenceNumber = 1;
    *objectHandle = NULL;
    if (cts != nullptr)
        *cts = NULL;
    *synchronousComplete = false;
    
    callback(ctx, S_OK, *found, *objectHandle, value->Bytes, value->Length, *versionSequenceNumber);

    return S_OK;
}

HRESULT MOCK_Store_ContainsKeyAsync(
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
    u16string u16key((char16_t*)key);
    
    if (g_dict.find(u16key) != g_dict.end())
    {
        *found = true;
    }
    else
    {
        *found = false;
    }

    *synchronousComplete = false;
    callback(ctx, S_OK, *found);

    return S_OK;
}

class KeyEnumerator
{
public :
    KeyEnumerator()
    {
        _it = g_dict.begin();
        _end = g_dict.end();
        _first = true;
    }

    KeyEnumerator(u16string firstKey, u16string lastKey)
    {
        if (firstKey.empty()) {
            _it = g_dict.begin();
        }
        else {
            _it = g_dict.lower_bound(firstKey);
        }

        if (lastKey.empty()) {
            _end = g_dict.end();
        }
        else {
            _end = g_dict.upper_bound(lastKey);
        }
        _first = true;
    }

    bool MoveNext()
    {
        if (_it == _end)
            return false;

        if (_first)
            _first = false;
        else {

            ++_it;
        }

        if (_it == _end)
            return false;

        return true;
    }

    const u16string &Current()
    {
        return _it->first;
    }

private :
    map<u16string, vector<char>>::iterator _it;
    map<u16string, vector<char>>::iterator _end;
    bool _first;
};

HRESULT MOCK_Store_CreateKeyEnumeratorAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyEnumeratorHandle* enumerator,
    __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    *enumerator = NULL;
    *synchronousComplete = false;
    u16string firstKey_str = (firstKey) ? u16string((char16_t*)firstKey) : u16string();
    u16string lastKey_str = (lastKey) ? u16string((char16_t*)lastKey) : u16string();
    callback(ctx, S_OK, (StoreKeyEnumeratorHandle) new KeyEnumerator(firstKey_str, lastKey_str));
    return S_OK;
}

HRESULT MOCK_StoreKeyEnumerator_MoveNext(
     StoreKeyEnumeratorHandle enumerator,
    BOOL* advanced,
    LPCWSTR * key)
{
    KeyEnumerator *pEnumerator = (KeyEnumerator *) enumerator;
    *advanced = pEnumerator->MoveNext();
    if (*advanced) 
        *key = (LPCWSTR)pEnumerator->Current().c_str();
    return S_OK;
}

void MOCK_StoreKeyEnumerator_Release(StoreKeyEnumeratorHandle enumerator)
{
    KeyEnumerator *pEnumerator = (KeyEnumerator *) enumerator;

    delete pEnumerator;
}

HRESULT MOCK_Store_CreateEnumeratorAsync(
    __in StateProviderHandle stateProvider,
    __in TransactionHandle txn,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    *enumerator = NULL;
    *synchronousComplete = false;
    callback(ctx, S_OK, (StoreKeyEnumeratorHandle) new KeyEnumerator());
    return S_OK;
}

HRESULT MOCK_Store_CreateRangedEnumeratorAsync(
    __in StoreHandle store,
    __in TransactionHandle txn,
    __in LPCWSTR firstKey,
    __in LPCWSTR lastKey,
    __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
    __in fnNotifyCreateEnumeratorAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    *enumerator = NULL;
    *synchronousComplete = false;
    u16string firstKey_str = (firstKey) ? u16string((char16_t*)firstKey) : u16string();
    u16string lastKey_str = (lastKey) ? u16string((char16_t*)lastKey) : u16string();
    callback(ctx, S_OK, (StoreKeyEnumeratorHandle) new KeyEnumerator(firstKey_str, lastKey_str));
    return S_OK;
}

void MOCK_StoreKeyValueEnumerator_Release(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator)
{
    KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;

    delete pEnumerator;
}

HRESULT MOCK_StoreKeyValueEnumerator_MoveNextAsync(
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
    KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;
    *advanced = pEnumerator->MoveNext();
    if (*advanced)
    {
        *key = (LPCWSTR)pEnumerator->Current().c_str();
        vector<char> &allBytes = g_dict[pEnumerator->Current()];

        value->Bytes = allBytes.data();
        value->Length = (uint32_t)allBytes.size();
        *versionSequenceNumber = 1;
        *objectHandle = 0;
    }
    return S_OK;
}

void MOCK_CancellationTokenSource_Cancel(CancellationTokenSourceHandle cts) {}
void MOCK_CancellationTokenSource_Release(CancellationTokenSourceHandle cts) {}

void MOCK_Buffer_Release(BufferHandle handle) {}

HRESULT MOCK_Store_SetNotifyStoreChangeCallback(
    __in StateProviderHandle stateProvider,
    __in fnNotifyStoreChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    return S_OK;
}

HRESULT MOCK_TxnReplicator_SetNotifyStateManagerChangeCallback(
    __in TxnReplicatorHandle txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    return S_OK;
}

HRESULT MOCK_StateProvider_GetInfo(
    __in StateProviderHandle stateProviderHandle,
    __in LPCWSTR lang,
    __out StateProvider_Info* stateProvider_Info)
{
    return S_OK;
}

void MOCK_AddRef(__in void* handle)
{

}

HRESULT MOCK_TxnReplicator_GetInfo(
    __in TxnReplicatorHandle txnReplicator,
    __inout TxnReplicator_Info* info)
{
    info->LastStableSequenceNumber = 1000;
    info->LastCommittedSequenceNumber = 1000;
    info->CurrentEpoch.DataLossNumber = 1;
    info->CurrentEpoch.ConfigurationNumber = 1;
    info->CurrentEpoch.Reserved = nullptr;
    return S_OK;
}

void GetReliableCollectionMockApiTable(ReliableCollectionApis* reliableCollectionApis)
{
    if (reliableCollectionApis == nullptr)
        return;

    *reliableCollectionApis = ReliableCollectionApis{
        MOCK_Store_Release,
        MOCK_Store_ConditionalGetAsync,
        MOCK_Store_AddAsync,
        MOCK_Store_ConditionalUpdateAsync,
        MOCK_Store_ConditionalRemoveAsync,
        MOCK_Store_GetCount,
        MOCK_Store_CreateKeyEnumeratorAsync,
        MOCK_Store_CreateEnumeratorAsync,
        MOCK_Transaction_Release,
        MOCK_Transaction_CommitAsync,
        MOCK_Transaction_Abort,
        MOCK_TxnReplicator_CreateTransaction,
        nullptr, //GetTxnReplicator,
        MOCK_TxnReplicator_Release,
        MOCK_TxnReplicator_GetOrAddStateProviderAsync,
        nullptr, //TxnReplicator_AddStateProviderAsync,
        nullptr, //TxnReplicator_RemoveStateProviderAsync,
        nullptr, //TxnReplicator_CreateEnumerator,
        nullptr, //TxnReplicator_GetStateProvider,
        MOCK_TxnReplicator_BackupAsync,
        MOCK_TxnReplicator_RestoreAsync,
        MOCK_CancellationTokenSource_Cancel,
        MOCK_CancellationTokenSource_Release,
        nullptr, //StateProviderEnumerator_Release,
        nullptr, //StateProviderEnumerator_MoveNext,
        MOCK_StoreKeyEnumerator_Release,
        MOCK_StoreKeyEnumerator_MoveNext,
        MOCK_StoreKeyValueEnumerator_Release,
        MOCK_StoreKeyValueEnumerator_MoveNextAsync,
        nullptr, //Test_UseEnv,
        MOCK_Buffer_Release,
        MOCK_Store_SetNotifyStoreChangeCallback,
        MOCK_TxnReplicator_SetNotifyStateManagerChangeCallback,
        nullptr, // TxnReplicator_SetNotifyTransactionChangeCallback
        MOCK_AddRef,
        MOCK_AddRef,
        MOCK_AddRef,
        MOCK_AddRef,
        nullptr, // SetNotifyStoreChangeCallbackMask
        MOCK_StateProvider_GetInfo,
        MOCK_AddRef,
        MOCK_Store_Release,
        nullptr, // Transaction_GetInfo
        nullptr, // Transaction_GetVisibilitySequenceNumberAsync
        nullptr, // Transaction_Dispose
        MOCK_Store_CreateRangedEnumeratorAsync,
        MOCK_Store_ContainsKeyAsync,
        MOCK_TxnReplicator_GetInfo,
        nullptr, // PrimaryReplicator_UpdateReplicatorSettings
        nullptr,
        nullptr,
        nullptr,
        MOCK_TxnReplicator_BackupAsync2
    };
}
