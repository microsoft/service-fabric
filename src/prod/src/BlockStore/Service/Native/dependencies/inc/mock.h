// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef RELIABLE_COLLECTION_TEST

#include "servicefabric.h"

namespace service_fabric
{
    struct mock
    {
        enum mock_mode
        {
            //
            // Not yet initialized - will get initialized from env
            //
            uninitialized,

            //
            // Real cluster - using real SF product code and this is the mode app will be running on
            //
            real_cluster,

            //
            // Completed mocked implementation on C API layer based on STL
            //
            mock_native_binding,

            //
            // Using SF product code but running as a unit test
            //
            mock_cluster
        };

        static mock_mode get_mode();

        static mock_mode _mode;

        static HRESULT ReliableCollectionService_InitializeEx(LPCU16STR serviceTypeName, int port, AddPartitionContextCallback add, RemovePartitionContextCallback remove, ChangeRoleCallback change, BOOL registerManifestEndpoints,
            BOOL skipUserPortEndpointRegistration, BOOL reportEndpointsOnlyOnPrimaryReplica);
        
        static HRESULT CLUSTER_ReliableCollectionService_InitializeEx(LPCU16STR serviceTypeName, int port, AddPartitionContextCallback add, RemovePartitionContextCallback remove, ChangeRoleCallback change, BOOL registerManifestEndpoints,
            BOOL skipUserPortEndpointRegistration, BOOL reportEndpointsOnlyOnPrimaryReplica);

        static HRESULT ReliableCollectionService_Cleanup();
        static HRESULT CLUSTER_ReliableCollectionService_Cleanup();

        /*************************************
        * Transaction APIs
        *************************************/
        static void Transaction_Release(
            __in TransactionHandle txn) noexcept;

        static void Transaction_Dispose(
            __in TransactionHandle txn) noexcept;

        static HRESULT Transaction_CommitAsync(
            __in TransactionHandle txn,
            __in fnNotifyAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete) noexcept;

        static HRESULT Transaction_Abort(
            __in TransactionHandle txn) noexcept;

        /*************************************
         * TStore APIs
         *************************************/
        static void Store_Release(
            __in StateProviderHandle storeHandle);

        static HRESULT Store_ConditionalGetAsync(
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
            __out BOOL* synchronousComplete);    // if call completed synchronously

        static HRESULT Store_AddAsync(
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
            __out BOOL* synchronousComplete);

        static HRESULT Store_ConditionalUpdateAsync(
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
            __out BOOL* synchronousComplete);    

        static HRESULT Store_ConditionalRemoveAsync(
            __in StateProviderHandle store,
            __in TransactionHandle txn,
            __in LPCU16STR key,
            __in int64_t timeout,
            __out CancellationTokenSourceHandle* cts,
            __in int64_t conditionalVersion,
            __out BOOL* removed,
            __in fnNotifyRemoveAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        static HRESULT Store_GetCount(
            __in StateProviderHandle store,
            __out int64_t* count);

        static HRESULT Store_CreateKeyEnumeratorAsync(
            __in StateProviderHandle store,
            __in TransactionHandle txn,
            __in LPCU16STR firstKey,
            __in LPCU16STR lastKey,
            __out StoreKeyEnumeratorHandle* enumerator,
            __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        static HRESULT Store_CreateEnumeratorAsync(
            __in StateProviderHandle store,
            __in TransactionHandle txn,
            __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
            __in fnNotifyCreateEnumeratorAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        static HRESULT Store_CreateRangedEnumeratorAsync(
            __in StateProviderHandle store,
            __in TransactionHandle txn,
            __in LPCU16STR firstKey,
            __in LPCU16STR lastKey,
            __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
            __in fnNotifyCreateEnumeratorAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        /*************************************
         * TxnReplicator APIs
         *************************************/
        static HRESULT TxnReplicator_CreateTransaction(
            __in TxnReplicatorHandle txnReplicator,
            __out TransactionHandle* txn) noexcept;

        static void TxnReplicator_Release(
            __in TxnReplicatorHandle txnReplicator) noexcept;

        typedef void(*fnNotifyGetOrAddStateProviderAsyncCompletion)(void* ctx, HRESULT status, StateProviderHandle store, BOOL exist);

        static HRESULT TxnReplicator_GetOrAddStateProviderAsync(
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
            __out BOOL* synchronousComplete) noexcept;

        static HRESULT TxnReplicator_AddStateProviderAsync(
            __in TxnReplicatorHandle txnReplicator,
            __in TransactionHandle txn,
            __in LPCU16STR name,
            __in LPCU16STR lang,
            __in StateProvider_Info* stateProviderInfo,
            __in int64_t timeout,
            __out CancellationTokenSourceHandle* cts,
            __in fnNotifyAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete) noexcept;

        static HRESULT TxnReplicator_RemoveStateProviderAsync(
            __in TxnReplicatorHandle txnReplicator,
            __in TransactionHandle txn,
            __in LPCU16STR name,
            __in int64_t timeout,
            __out CancellationTokenSourceHandle* cts,
            __in fnNotifyAsyncCompletion callback,
            __in void* ctx,
            __out BOOL*synchronousComplete) noexcept;

        static HRESULT TxnReplicator_CreateEnumerator(
            __in TxnReplicatorHandle txnReplicator,
            __in BOOL parentsOnly,
            __out StateProviderEnumeratorHandle* enumerator) noexcept;

        static HRESULT TxnReplicator_GetStateProvider(
            __in TxnReplicatorHandle txnReplicator,
            __in LPCU16STR name,
            __out StateProviderHandle* store) noexcept;

        static HRESULT TxnReplicator_BackupAsync(
            __in TxnReplicatorHandle txnReplicator,
            __in fnUploadAsync uploadAsyncCallback,
            __in Backup_Option backupOption,
            __in int64_t timeout,
            __out CancellationTokenSourceHandle* cts,
            __in fnNotifyAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        static HRESULT TxnReplicator_RestoreAsync(
            __in TxnReplicatorHandle txnReplicator,
            __in LPCU16STR backupFolder,
            __in Restore_Policy restorePolicy,
            __in int64_t timeout,
            __out CancellationTokenSourceHandle*  cts,
            __in fnNotifyAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronouscomplete);


        static HRESULT GetTxnReplicator(
            __in void* statefulServicePartition, 
            __in void* dataLossHandler, 
            __in const void* fabricReplicatorSettings, 
            __in void* txnReplicatorSettings, 
            __in void* ktlloggerSharedSettings, 
            __out void** primaryReplicator,
            __out TxnReplicatorHandle* txnReplicatorHandle);

        /*************************************
         * CancellationTokenSource APIs
         *************************************/
        static void CancellationTokenSource_Cancel(
            CancellationTokenSourceHandle cts);
        static void CancellationTokenSource_Release(
            CancellationTokenSourceHandle cts);


        /*************************************
         * StateProviderEnumerator APIs
         *************************************/
        static void StateProviderEnumerator_Release(
            __in StateProviderEnumeratorHandle enumerator);

        static HRESULT StateProviderEnumerator_MoveNext(
            __in StateProviderEnumeratorHandle enumerator,
            __out BOOL* advanced,
            __out LPCU16STR* providerName,
            __out StateProviderHandle* provider);


        /*************************************
         * StoreKeyEnumerator APIs
         *************************************/
        static void StoreKeyEnumerator_Release(
            StoreKeyEnumeratorHandle enumerator);

        static HRESULT StoreKeyEnumerator_MoveNext(
            __in StoreKeyEnumeratorHandle enumerator,
            __out BOOL* advanced,
            __out LPCU16STR* key);


        /*************************************
         * StoreKeyValueEnumerator APIs
         *************************************/
        static void StoreKeyValueEnumerator_Release(
            __in StoreKeyValueAsyncEnumeratorHandle enumerator);

        static HRESULT StoreKeyValueEnumerator_MoveNextAsync(
            __in StoreKeyValueAsyncEnumeratorHandle enumerator,
            __out CancellationTokenSourceHandle* cts,
            __out BOOL* advanced,
            __out LPCU16STR* key,
            __out size_t* objectHandle,
            __out Buffer* value,
            __out int64_t* versionSequenceNumber,
            __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
            __in void* ctx,
            __out BOOL* synchronousComplete);

        static void Buffer_Release(BufferHandle handle);
                                            
    };
}

#endif