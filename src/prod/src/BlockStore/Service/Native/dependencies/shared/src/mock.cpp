// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// TODO: Review this.
#define _CRT_SECURE_NO_WARNINGS

#if !defined(PLATFORM_UNIX)
#include "stdafx.h"
#endif

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <cstdlib>
#include <mutex>

#if defined(PLATFORM_UNIX)
#include <sal.h>
#include "PAL.h"
#define UNREFERENCED_PARAMETER(x)
#define __in
#define __out
#endif

#include "../../inc/servicefabric.h"
#include "../../inc/external/SfStatus.h"
#include "../../inc/mock.h"
#include "../../inc/binding.h"


using namespace std;
using namespace service_fabric;

namespace service_fabric
{
    mock::mock_mode mock::_mode = mock::mock_mode::uninitialized;

    mock::mock_mode mock::get_mode()
    {
        if (_mode == mock::mock_mode::uninitialized)
        {
#if defined(RELIABLE_COLLECTION_TEST_MOCK_NATIVE)
            _mode = mock::mock_mode::mock_native_binding;
#elif defined(RELIABLE_COLLECTION_TEST_MOCK_CLUSTER)
            _mode = mock::mock_mode::mock_cluster;
#else
            if (std::getenv("SF_MOCK_NATIVE"))
                _mode = mock::mock_mode::mock_native_binding;
            else if (std::getenv("SF_MOCK_CLUSTER"))
                _mode = mock::mock_mode::mock_cluster;
            else
                _mode = mock::mock_mode::real_cluster;
#endif
        }

        return _mode;
    }

    void AddContextThreadWorker(AddPartitionContextCallback add, KEY_TYPE key, TxnReplicatorHandle hReplicator, PartitionHandle hPartition, GUID guidPartition, int64_t replicaId)
    {
        add(key, hReplicator, hPartition, guidPartition, replicaId);
    }

    HRESULT mock::CLUSTER_ReliableCollectionService_InitializeEx(LPCU16STR serviceTypeName, int port, AddPartitionContextCallback add, RemovePartitionContextCallback remove, ChangeRoleCallback change, BOOL registerManifestEndpoints,
            BOOL skipUserPortEndpointRegistration, BOOL reportEndpointsOnlyOnPrimaryReplica)
    {
        UNREFERENCED_PARAMETER(serviceTypeName);
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(remove);
        UNREFERENCED_PARAMETER(registerManifestEndpoints);
        UNREFERENCED_PARAMETER(skipUserPortEndpointRegistration);
        UNREFERENCED_PARAMETER(reportEndpointsOnlyOnPrimaryReplica);

        auto pfnInitTestFunc = binding::get()._pfnInitializeTest;

        TxnReplicatorHandle hReplicator;
        PartitionHandle hPartition;
        GUID partitionGuid;
        int64_t replicaId;
        HRESULT initStatus = pfnInitTestFunc(u"NodeReliableCollectionTest", false, &hReplicator, &hPartition, &partitionGuid, &replicaId);

        thread addContextWorkerThread(AddContextThreadWorker, add, (KEY_TYPE)0, hReplicator, hPartition, partitionGuid, replicaId);
        addContextWorkerThread.join();

        // For Mock Cluster, invoke the ChangeRoleCallback to simulate transition to the primary replica role
        change((KEY_TYPE)0, 2 /* Primary Replica */);
       
        return initStatus;
    }

    HRESULT mock::CLUSTER_ReliableCollectionTxnReplicator_GetInfo(_in TxnReplicatorHandle txnReplicator, __inout TxnReplicator_Info* info)
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        info->LastStableSequenceNumber = 1000;
        info->LastCommittedSequenceNumber = 1000;
        info->CurrentEpoch.DataLossNumber = 1;
        info->CurrentEpoch.ConfigurationNumber = 1;
        info->CurrentEpoch.Reserved = nullptr;
        return S_OK;
    }

    HRESULT mock::ReliableCollectionService_InitializeEx(LPCU16STR serviceTypeName, int port, AddPartitionContextCallback add, RemovePartitionContextCallback remove, ChangeRoleCallback change, BOOL registerManifestEndpoints,
            BOOL skipUserPortEndpointRegistration, BOOL reportEndpointsOnlyOnPrimaryReplica)
    {
        UNREFERENCED_PARAMETER(serviceTypeName);
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(remove);
        UNREFERENCED_PARAMETER(change);
        UNREFERENCED_PARAMETER(registerManifestEndpoints);
        UNREFERENCED_PARAMETER(skipUserPortEndpointRegistration);
        UNREFERENCED_PARAMETER(reportEndpointsOnlyOnPrimaryReplica);

        GUID dummyPartitionGuid = {0};
        thread addContextWorkerThread(AddContextThreadWorker, add, (KEY_TYPE)0, (TxnReplicatorHandle)0x100, (PartitionHandle)0x100, dummyPartitionGuid, 0);
        addContextWorkerThread.join();

        return S_OK;
    }

    HRESULT mock::ReliableCollectionService_Cleanup()
    {
        return S_FALSE;
    }

    HRESULT mock::CLUSTER_ReliableCollectionService_Cleanup()
    {
        return S_FALSE;
    }

    HRESULT mock::ReliableCollectionTxnReplicator_GetInfo(_in TxnReplicatorHandle txnReplicator, __inout TxnReplicator_Info* info)
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        info->LastStableSequenceNumber = 1000;
        info->LastCommittedSequenceNumber = 1000;
        info->CurrentEpoch.DataLossNumber = 1;
        info->CurrentEpoch.ConfigurationNumber = 1;
        info->CurrentEpoch.Reserved = nullptr;
        return S_OK;
    }

    size_t g_nextStateProviderHandle = 0;

    mutex g_storeMutex;
    map<u16string, StateProviderHandle> g_stores;
    map<StateProviderHandle, map<u16string, vector<char>>> g_dicts;

    HRESULT mock::TxnReplicator_GetOrAddStateProviderAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in TransactionHandle txn,
        __in LPCU16STR name,
        __in LPCU16STR lang,
        __in StateProvider_Info* stateProviderInfo,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __out StateProviderHandle* store,
        __out BOOL* alreadyExist,
        __in fnNotifyGetOrAddStateProviderAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) noexcept 
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(name);
        UNREFERENCED_PARAMETER(lang);
        UNREFERENCED_PARAMETER(stateProviderInfo);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(cts);
        
        u16string store_name(name);
        StateProviderHandle retStore;
        bool exists;

        {
            lock_guard<mutex> lock(g_storeMutex);

            auto it = g_stores.find(store_name);
            if (it == g_stores.end())
            {
                retStore = (StateProviderHandle)(++g_nextStateProviderHandle);
                g_stores.insert({ store_name, retStore });
                exists = false;
            }
            else
            {
                retStore = it->second;
                exists = true;
            }
        }
    
        *store = retStore;
        *alreadyExist = exists;
        *synchronousComplete = false;

        // Simulate running on KTL thread
        std::thread callback_thread([=]() {
            callback(ctx, S_OK, retStore, exists);
        });
        callback_thread.detach();

        return S_OK;
    }

    HRESULT mock::TxnReplicator_RemoveStateProviderAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in TransactionHandle txn,
        __in LPCU16STR name,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL*synchronousComplete) noexcept  
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(cts);
        u16string store_name(name);

        {
            lock_guard<mutex> lock(g_storeMutex);

            auto it = g_stores.find(store_name);
            if (it == g_stores.end())
            {
                return -1; // @TODO - SF_STATUS_NAME_DOES_NOT_EXIST;
            }
            else
            {
                // @TODO - implement correct lifetime semantics in mock
                g_stores.erase(it);
            }
        }

        *synchronousComplete = false;

        // Simulate running on KTL thread
        std::thread callback_thread([=]() {
            callback(ctx, S_OK);
        });
        callback_thread.detach();

        return S_OK;
    }

    HRESULT mock::TxnReplicator_GetStateProvider(
        __in TxnReplicatorHandle txnReplicator,
        __in LPCU16STR name,
        __out StateProviderHandle* store) noexcept
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        u16string store_name(name);

        {
            lock_guard<mutex> lock(g_storeMutex);

            auto it = g_stores.find(store_name);
            if (it == g_stores.end())
            {
                *store = NULL;
            }
            else
            {
                *store = it->second;
            }
        }

        return S_OK;
    }

    HRESULT mock::TxnReplicator_CreateEnumerator(
        __in TxnReplicatorHandle txnReplicator,
        __in BOOL parentsOnly,
        __out StateProviderEnumeratorHandle* enumerator) noexcept
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        UNREFERENCED_PARAMETER(parentsOnly);
        using store_iterator = map<u16string, StateProviderHandle>::const_iterator;

        store_iterator *it = new store_iterator(g_stores.begin());

        *enumerator = reinterpret_cast<StateProviderEnumeratorHandle>(it);

        return S_OK;
    }

    void mock::StateProviderEnumerator_Release(
        __in StateProviderEnumeratorHandle enumerator)  
    {
        using store_iterator = map<u16string, StateProviderHandle>::const_iterator;
        delete reinterpret_cast<store_iterator *>(enumerator);
    }

    HRESULT mock::StateProviderEnumerator_MoveNext(
        __in StateProviderEnumeratorHandle enumerator,
        __out BOOL* advanced,
        __out LPCU16STR* providerName,
        __out StateProviderHandle* provider)
    {
        using store_iterator = map<u16string, StateProviderHandle>::const_iterator;
        store_iterator *it = reinterpret_cast<store_iterator *>(enumerator);

        if ((*it) == g_stores.end())
        {
            *advanced = false;
        }
        else
        {
            *providerName = (*it)->first.c_str();
            *provider = (*it)->second;

            *advanced = TRUE;
            ++(*it);
        }

        return S_OK;
    }

    HRESULT mock::TxnReplicator_CreateTransaction(
        __in TxnReplicatorHandle txnReplicator,
        __out TransactionHandle* txn) noexcept
    {
        UNREFERENCED_PARAMETER(txnReplicator);
        *txn = (TransactionHandle)0x200;
        return S_OK;
    }

    HRESULT mock:: Transaction_CommitAsync(
        __in TransactionHandle txn,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) noexcept
    {
        UNREFERENCED_PARAMETER(txn);
        *synchronousComplete = false;
        callback(ctx, S_OK);
        return S_OK;
    }

    HRESULT mock::Transaction_Abort(
        __in TransactionHandle txn) noexcept
    {
        UNREFERENCED_PARAMETER(txn);
        return S_OK;
    }

    void mock::Transaction_Release(
        __in TransactionHandle txn) noexcept
    {
        UNREFERENCED_PARAMETER(txn);
    }

    void mock::Transaction_Dispose(
        __in TransactionHandle txn) noexcept
    {
        UNREFERENCED_PARAMETER(txn);
    }

    HRESULT mock::Store_GetCount(
        __in StateProviderHandle store,
        __out int64_t* count)
    {
        *count = (int64_t) g_dicts[store].size();

        return S_OK;
    }

    HRESULT mock::Store_AddAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR key,
        __in size_t objectHandle,               // handle of object to be stored  
        __in void* bytes,                    // serailized byte array of object
        __in uint32_t bytesLength,             // byte array length
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) 
    {
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(objectHandle);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(cts);
        *synchronousComplete = false;
        char *pchBytes = reinterpret_cast<char *>(bytes);

        g_dicts[store].insert(
            std::pair<u16string, vector<char>>(
                key,
                vector<char>(pchBytes, pchBytes + bytesLength)));

        callback(ctx, S_OK);
        return S_OK;
    }

    HRESULT mock::Store_ConditionalUpdateAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR key,
        __in size_t objectHandle,               // handle of object to be stored  
        __in void* bytes,                    // serailized byte array of object
        __in uint32_t bytesLength,                // byte array length
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in int64_t conditionalVersion,
        __out BOOL* updated,
        __in fnNotifyUpdateAsyncCompletion callback,
        __in void* ctx,                
        __out BOOL* synchronousComplete)
    {
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(objectHandle);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(cts);
        UNREFERENCED_PARAMETER(conditionalVersion);
        u16string key_str(key);

        auto &dict = g_dicts[store];
        auto dict_it = dict.find(key_str);
        if (dict_it != dict.end())
        {
            *updated = true;
            char *pchBytes = reinterpret_cast<char *>(bytes);
            dict_it->second.assign(pchBytes, pchBytes + bytesLength);
        }
        else
        {
            *updated = false;
        }

        *synchronousComplete = false;
        callback(ctx, S_OK, *updated);

        return S_OK;
    }

    HRESULT mock::Store_ConditionalRemoveAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR key,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in int64_t conditionalVersion,
        __out BOOL* removed,
        __in fnNotifyRemoveAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete)
    {
        UNREFERENCED_PARAMETER(cts);
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(conditionalVersion);
        u16string key_str(key);
        auto &dict = g_dicts[store];

        if (dict.find(key) != dict.end())
        {
            *removed = true;
            dict.erase(key_str);
        }
        else
        {
            *removed = false;
        }

        *synchronousComplete = false;
        callback(ctx, S_OK, *removed);

        return S_OK;
    }

    HRESULT mock::Store_ConditionalGetAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR key,
        __in int64_t timeout,
        __in Store_LockMode lockMode,
        __out size_t* objectHandle,
        __out Buffer* value,
        __out int64_t* versionSequenceNumber,
        __out CancellationTokenSourceHandle* cts,
        __out BOOL* found,
        __in fnNotifyGetAsyncCompletion callback,      // callback to be called when call does not complete synchronously
        __in void* ctx,                      // Context to be passed in when making callback
        __out BOOL* synchronousComplete)    // if call completed synchronously 
    {
        UNREFERENCED_PARAMETER(lockMode);
        UNREFERENCED_PARAMETER(cts);
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(timeout);
        u16string key_str(key);

        auto &dict = g_dicts[store];

        if (dict.find(key) != dict.end())
        {
            vector<char> &allBytes = dict[key_str];

            *found = true;
            value->Bytes = allBytes.data();
            value->Length = (uint32_t) allBytes.size();
        }
        else
        {
            *found = false;
            value->Bytes = NULL;
            value->Length = 0;
        }

        value->Handle = NULL;

        *versionSequenceNumber = 1;
        *objectHandle = 0;
        *synchronousComplete = false;

        callback(ctx, S_OK, *found, *objectHandle, value->Bytes, value->Length, *versionSequenceNumber);

        return S_OK;
    }

    void mock::Store_Release(StateProviderHandle store)
    {
        UNREFERENCED_PARAMETER(store);
    }

    class KeyEnumerator
    {
    public:
        KeyEnumerator(StateProviderHandle store, u16string firstKey, u16string lastKey):
            _dict(g_dicts[store])
        {
            if (firstKey.empty()){
            _it = _dict.begin();
            }else{
                _it = _dict.lower_bound(firstKey);
            }

            if (lastKey.empty()){
                _end = _dict.end();
            }else{
                _end = _dict.upper_bound(lastKey);
            }
            _first = true;
        }

        bool MoveNext()
        {
            if (_it == _end)
                return false;

            if (_first)
                _first = false;
            else{

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

        vector<char> &CurrentValue()
        {
            return _it->second;
        }

    private:
        map<u16string, vector<char>> &_dict;
        map<u16string, vector<char>>::iterator _it;
        map<u16string, vector<char>>::iterator _end;
        bool _first;
    };

    HRESULT mock::Store_CreateKeyEnumeratorAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR firstKey,
        __in LPCU16STR lastKey,
        __out StoreKeyEnumeratorHandle* enumerator,
        __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete)
    {
        UNREFERENCED_PARAMETER(txn);
        UNREFERENCED_PARAMETER(firstKey);
        UNREFERENCED_PARAMETER(lastKey);
        *enumerator = NULL;
        *synchronousComplete = false;


        u16string firstKey_str = (firstKey) ? u16string(firstKey) : u16string();
        u16string lastKey_str = (lastKey) ? u16string(lastKey) : u16string();
        callback(ctx, S_OK, (StoreKeyEnumeratorHandle) new KeyEnumerator(store, firstKey_str, lastKey_str));
        return S_OK;
    }

    HRESULT mock::StoreKeyEnumerator_MoveNext(
        __in StoreKeyEnumeratorHandle enumerator,
        __out BOOL* advanced,
        __out LPCU16STR* key)
    {
        KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;
        *advanced = pEnumerator->MoveNext();
        if (*advanced)
            *key = pEnumerator->Current().c_str();
        return S_OK;
    }

    void mock::StoreKeyEnumerator_Release(StoreKeyEnumeratorHandle enumerator)
    {
        KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;

        delete pEnumerator;
    }

    HRESULT mock::Store_CreateRangedEnumeratorAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCU16STR firstKey,
        __in LPCU16STR lastKey,
        __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
        __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete)
    {
        UNREFERENCED_PARAMETER(txn);

        *enumerator = NULL;
        *synchronousComplete = false;

        u16string firstKey_str = (firstKey) ? u16string(firstKey) : u16string();
        u16string lastKey_str = (lastKey) ? u16string(lastKey) : u16string();
        callback(ctx, S_OK, (StoreKeyValueAsyncEnumeratorHandle) new KeyEnumerator(store, firstKey_str, lastKey_str));
        return S_OK;
    }

    HRESULT mock::StoreKeyValueEnumerator_MoveNextAsync(
            __in StoreKeyValueAsyncEnumeratorHandle enumerator,
            __out CancellationTokenSourceHandle* cts,
            __out BOOL* advanced,
            __out LPCU16STR* key,
            __out size_t* objectHandle,
            __out Buffer* value,
            __out int64_t* versionSequenceNumber,
            __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete)
    {
        UNREFERENCED_PARAMETER(cts);

        KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;
        *advanced = pEnumerator->MoveNext();
        if (*advanced)
            *key = pEnumerator->Current().c_str();
            vector<char> &allBytes = pEnumerator->CurrentValue();
            value->Bytes = allBytes.data();
            value->Length = (uint32_t) allBytes.size();

        value->Handle = NULL;
        *versionSequenceNumber = 1;
        *objectHandle = 0;
        *synchronousComplete = false;

        callback(ctx, S_OK, *advanced, *key ,*objectHandle, value->Bytes, value->Length, *versionSequenceNumber);
        return S_OK;
    }

    void mock::StoreKeyValueEnumerator_Release(StoreKeyValueAsyncEnumeratorHandle enumerator)
    {
        KeyEnumerator *pEnumerator = (KeyEnumerator *)enumerator;

        delete pEnumerator;
    }

    void mock::Buffer_Release(BufferHandle handle)
    {
        UNREFERENCED_PARAMETER(handle);
    }
}
