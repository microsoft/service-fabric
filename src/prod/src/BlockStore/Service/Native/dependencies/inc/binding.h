// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "libpal.h"
#include "servicefabric.h"
#include "mock.h"

namespace service_fabric
{
    struct binding
    {
        void *_hReliableServices;

        pfnReliableCollectionService_InitializeEx _pfnReliableCollectionService_InitializeEx;
        pfnReliableCollectionService_Cleanup _pfnReliableCollectionService_Cleanup;
        pfnReliableCollectionTxnReplicator_GetInfo _pfnReliableCollectionTxnReplicator_GetInfo;

        void *_hReliableDictionaryInterop;

        pfnReliableCollectionRuntime_Initialize _pfnReliableCollectionRuntime_Initialize;
        pfnTxnReplicator_GetOrAddStateProviderAsync _pfnTxnReplicator_GetOrAddStateProviderAsync;
        pfnTxnReplicator_RemoveStateProviderAsync _pfnTxnReplicator_RemoveStateProviderAsync;
        pfnTxnReplicator_GetStateProvider _pfnTxnReplicator_GetStateProvider;
        pfnTxnReplicator_CreateEnumerator _pfnTxnReplicator_CreateEnumerator;
        pfnTxnReplicator_CreateTransaction _pfnTxnReplicator_CreateTransaction;
        pfnStateProviderEnumerator_Release _pfnStateProviderEnumerator_Release;
        pfnStateProviderEnumerator_MoveNext _pfnStateProviderEnumerator_MoveNext;

        pfnTransaction_CommitAsync _pfnTransaction_CommitAsync;
        pfnTransaction_Abort _pfnTransaction_Abort;
        pfnTransaction_Release _pfnTransaction_Release;
        pfnTransaction_Dispose _pfnTransaction_Dispose;

        pfnStore_GetCount _pfnStore_GetCount;
        pfnStore_AddAsync _pfnStore_AddAsync;
        pfnStore_ConditionalUpdateAsync _pfnStore_ConditionalUpdateAsync;
        pfnStore_ConditionalRemoveAsync _pfnStore_ConditionalRemoveAsync;
        pfnStore_ConditionalGetAsync _pfnStore_ConditionalGetAsync;
        pfnStore_Release _pfnStore_Release;

        pfnBuffer_Release _pfnBuffer_Release;

        pfnStore_CreateKeyEnumeratorAsync _pfnStore_CreateKeyEnumeratorAsync;
        pfnStoreKeyEnumerator_MoveNext _pfnStoreKeyEnumerator_MoveNext;
        pfnStoreKeyEnumerator_Release _pfnStoreKeyEnumerator_Release;

        pfnStore_CreateRangedEnumeratorAsync _pfnStore_CreateRangedEnumeratorAsync;
        pfnStoreKeyValueEnumerator_MoveNextAsync _pfnStoreKeyValueEnumerator_MoveNextAsync;
        pfnStoreKeyValueEnumerator_Release _pfnStoreKeyValueEnumerator_Release;

        void *_hTestHelper;

        pfnInitializeTest _pfnInitializeTest;
        pfnCleanupTest _pfnCleanupTest;

        void init()
        {
#ifdef RELIABLE_COLLECTION_TEST
            mock::mock_mode mode = mock::get_mode();

            if (mode == mock::mock_mode::mock_native_binding)
            {
                _pfnReliableCollectionService_InitializeEx = mock::ReliableCollectionService_InitializeEx;
                _pfnReliableCollectionService_Cleanup = mock::ReliableCollectionService_Cleanup;
                _pfnReliableCollectionTxnReplicator_GetInfo = mock::ReliableCollectionTxnReplicator_GetInfo;
                _pfnTxnReplicator_GetOrAddStateProviderAsync = mock::TxnReplicator_GetOrAddStateProviderAsync;
                _pfnTxnReplicator_RemoveStateProviderAsync = mock::TxnReplicator_RemoveStateProviderAsync;
                _pfnTxnReplicator_GetStateProvider = mock::TxnReplicator_GetStateProvider;
                _pfnTxnReplicator_CreateEnumerator = mock::TxnReplicator_CreateEnumerator;
                _pfnTxnReplicator_CreateTransaction = mock::TxnReplicator_CreateTransaction;

                _pfnStateProviderEnumerator_Release = mock::StateProviderEnumerator_Release;
                _pfnStateProviderEnumerator_MoveNext = mock::StateProviderEnumerator_MoveNext;

                _pfnTransaction_CommitAsync = mock::Transaction_CommitAsync;
                _pfnTransaction_Abort = mock::Transaction_Abort;
                _pfnTransaction_Release = mock::Transaction_Release;
                _pfnTransaction_Dispose = mock::Transaction_Dispose;
                _pfnStore_GetCount = mock::Store_GetCount;
                _pfnStore_AddAsync = mock::Store_AddAsync;
                _pfnStore_ConditionalGetAsync = mock::Store_ConditionalGetAsync;
                _pfnStore_ConditionalUpdateAsync = mock::Store_ConditionalUpdateAsync;
                _pfnStore_ConditionalRemoveAsync = mock::Store_ConditionalRemoveAsync;
                _pfnStore_Release = mock::Store_Release;

                _pfnBuffer_Release = mock::Buffer_Release;

                _pfnStore_CreateKeyEnumeratorAsync = mock::Store_CreateKeyEnumeratorAsync;
                _pfnStoreKeyEnumerator_MoveNext = mock::StoreKeyEnumerator_MoveNext;
                _pfnStoreKeyEnumerator_Release = mock::StoreKeyEnumerator_Release;

                _pfnStore_CreateRangedEnumeratorAsync = mock::Store_CreateRangedEnumeratorAsync;
                _pfnStoreKeyValueEnumerator_MoveNextAsync = mock::StoreKeyValueEnumerator_MoveNextAsync;
                _pfnStoreKeyValueEnumerator_Release = mock::StoreKeyValueEnumerator_Release;
            }
            else
#endif
            {
                //
                // Use real service fabric DLL implementation
                //

                // load interop DLL first so that it can be found when loading service DLL
                initialize_library(_hReliableDictionaryInterop, RELIABLE_DICTIONARY_INTEROP_DLL);

                initialize_library(_hReliableServices, RELIABLE_SERVICES_DLL);

#ifdef RELIABLE_COLLECTION_TEST
                if (mode != mock::mock_mode::mock_cluster)
#endif // RELIABLE_COLLECTION_TEST
                {
                    initialize_func(_pfnReliableCollectionService_InitializeEx, "ReliableCollectionService_InitializeEx", _hReliableServices);
                    initialize_func(_pfnReliableCollectionService_Cleanup, "ReliableCollectionService_Cleanup", _hReliableServices);
                }
                

                initialize_func(_pfnReliableCollectionRuntime_Initialize, "ReliableCollectionRuntime_Initialize", _hReliableDictionaryInterop);
                initialize_func(_pfnReliableCollectionTxnReplicator_GetInfo, "TxnReplicator_GetInfo", _hReliableDictionaryInterop);
                initialize_func(_pfnTxnReplicator_GetOrAddStateProviderAsync, "TxnReplicator_GetOrAddStateProviderAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnTxnReplicator_RemoveStateProviderAsync, "TxnReplicator_RemoveStateProviderAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnTxnReplicator_GetStateProvider, "TxnReplicator_GetStateProvider", _hReliableDictionaryInterop);
                initialize_func(_pfnTxnReplicator_CreateEnumerator, "TxnReplicator_CreateEnumerator", _hReliableDictionaryInterop);
                initialize_func(_pfnTxnReplicator_CreateTransaction, "TxnReplicator_CreateTransaction", _hReliableDictionaryInterop);

                initialize_func(_pfnStateProviderEnumerator_Release, "StateProviderEnumerator_Release", _hReliableDictionaryInterop);
                initialize_func(_pfnStateProviderEnumerator_MoveNext, "StateProviderEnumerator_MoveNext", _hReliableDictionaryInterop);

                initialize_func(_pfnTransaction_CommitAsync, "Transaction_CommitAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnTransaction_Abort, "Transaction_Abort", _hReliableDictionaryInterop);
                initialize_func(_pfnTransaction_Release, "Transaction_Release", _hReliableDictionaryInterop);
                initialize_func(_pfnTransaction_Dispose, "Transaction_Dispose", _hReliableDictionaryInterop);

                initialize_func(_pfnStore_GetCount, "Store_GetCount", _hReliableDictionaryInterop);
                initialize_func(_pfnStore_AddAsync, "Store_AddAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStore_ConditionalGetAsync, "Store_ConditionalGetAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStore_ConditionalUpdateAsync, "Store_ConditionalUpdateAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStore_ConditionalRemoveAsync, "Store_ConditionalRemoveAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStore_Release, "Store_Release", _hReliableDictionaryInterop);

                initialize_func(_pfnBuffer_Release, "Buffer_Release", _hReliableDictionaryInterop);

                initialize_func(_pfnStore_CreateKeyEnumeratorAsync, "Store_CreateKeyEnumeratorAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStoreKeyEnumerator_Release, "StoreKeyEnumerator_Release", _hReliableDictionaryInterop);
                initialize_func(_pfnStoreKeyEnumerator_MoveNext, "StoreKeyEnumerator_MoveNext", _hReliableDictionaryInterop);

                // We use this because Store_CreateEnumeratorAsync cannot search for range
                initialize_func(_pfnStore_CreateRangedEnumeratorAsync, "Store_CreateRangedEnumeratorAsync", _hReliableDictionaryInterop);
                initialize_func(_pfnStoreKeyValueEnumerator_Release, "StoreKeyValueEnumerator_Release", _hReliableDictionaryInterop);
                initialize_func(_pfnStoreKeyValueEnumerator_MoveNextAsync, "StoreKeyValueEnumerator_MoveNextAsync", _hReliableDictionaryInterop);
#ifdef RELIABLE_COLLECTION_TEST
                if (mode == mock::mock_mode::mock_cluster)
                {
                    //
                    // This initialize service fabric with minimal setup
                    // This is close to end-to-end testing without a real cluster
                    //
                    initialize_library(_hTestHelper, TEST_HELPER_DLL);

                    initialize_func(_pfnInitializeTest, "ReliableCollectionStandalone_Initialize", _hTestHelper);
                    initialize_func(_pfnCleanupTest, "ReliableCollectionStandalone_Cleanup", _hTestHelper);

                    _pfnReliableCollectionService_InitializeEx = mock::CLUSTER_ReliableCollectionService_InitializeEx;
                    _pfnReliableCollectionService_Cleanup= mock::CLUSTER_ReliableCollectionService_Cleanup;
                    _pfnReliableCollectionTxnReplicator_GetInfo = mock::CLUSTER_ReliableCollectionTxnReplicator_GetInfo;
                }
#endif
            }
        }

        void initialize_library(void *&hmod, const char *name)
        {
            hmod = ::load_library(name);
            if (hmod == NULL)
            {
                string msg(::get_load_error());
                msg.append(": ");
                msg.append(name);
                throw runtime_error(msg.c_str());
            }
        }

        template <typename T>
        void initialize_func(T &pfn, const char *name, void *hmod)
        {
            pfn = reinterpret_cast<T>(::load_symbol(hmod, name));
            if (pfn == NULL)
            {
                string msg("failed to locate function: ");
                msg.append(name);
                throw runtime_error(name);
            }
        }

        static binding &get()
        {
            static binding g_binding;

            return g_binding;
        }
     };
}


