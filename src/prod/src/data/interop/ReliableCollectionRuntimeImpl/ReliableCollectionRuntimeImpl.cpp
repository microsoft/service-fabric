// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

void GetReliableCollectionImplApiTable(ReliableCollectionApis* reliableCollectionApis)
{
    if (reliableCollectionApis == nullptr)
        return;

    *reliableCollectionApis = ReliableCollectionApis{
        Store_Release,
        Store_ConditionalGetAsync,
        Store_AddAsync,
        Store_ConditionalUpdateAsync,
        Store_ConditionalRemoveAsync,
        Store_GetCount,
        Store_CreateKeyEnumeratorAsync,
        Store_CreateEnumeratorAsync,
        Transaction_Release,
        Transaction_CommitAsync,
        Transaction_Abort,
        TxnReplicator_CreateTransaction,
        GetTxnReplicator,
        TxnReplicator_Release,
        TxnReplicator_GetOrAddStateProviderAsync,
        TxnReplicator_AddStateProviderAsync,
        TxnReplicator_RemoveStateProviderAsync,
        TxnReplicator_CreateEnumerator,
        TxnReplicator_GetStateProvider,
        TxnReplicator_BackupAsync,
        TxnReplicator_RestoreAsync,
        CancellationTokenSource_Cancel,
        CancellationTokenSource_Release,
        StateProviderEnumerator_Release,
        StateProviderEnumerator_MoveNext,
        StoreKeyEnumerator_Release,
        StoreKeyEnumerator_MoveNext,
        StoreKeyValueEnumerator_Release,
        StoreKeyValueEnumerator_MoveNextAsync,
        Test_UseEnv,
        Buffer_Release,
        Store_SetNotifyStoreChangeCallback,
        TxnReplicator_SetNotifyStateManagerChangeCallback,
        TxnReplicator_SetNotifyTransactionChangeCallback,
        Transaction_AddRef,
        Store_AddRef,
        StoreKeyEnumerator_AddRef,
        StateProviderEnumerator_AddRef,
        GetTransactionalReplicator,
        Store_SetNotifyStoreChangeCallbackMask,
        TxnReplicator_AddStateProviderAsync2,
        TxnReplicator_GetOrAddStateProviderAsync2,
        StateProvider_GetInfo,
        StateProvider_AddRef,
        StateProvider_Release,
        Transaction_GetInfo,
        Transaction_GetVisibilitySequenceNumberAsync,
        Transaction_Dispose,
        Transaction_Release2,
        Store_CreateRangedEnumeratorAsync,
        Store_ContainsKeyAsync
    };
}

