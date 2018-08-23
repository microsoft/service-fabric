// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Utilities;
using namespace TStore;
using namespace TxnReplicator;
using namespace Data::Interop;

HRESULT StateManagerChangeHandler::Create(
    __in KAllocator& allocator,
    __in ITransactionalReplicator *txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void* ctx,
    __out StateManagerChangeHandler::SPtr& result)
{
    StateManagerChangeHandler* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) StateManagerChangeHandler(txnReplicator, callback, cleanupCallback, ctx);
    if (!pointer)
        return E_OUTOFMEMORY;

    result = pointer;
    return S_OK;
}

void StateManagerChangeHandler::OnRebuilt(
    __in ITransactionalReplicator & source,
    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<IStateProvider2>>> & stateProviders)
{
    CODING_ERROR_ASSERT(txnReplicator_ == &source);

    auto enumerator = reinterpret_cast<StateProviderEnumeratorHandle>(&stateProviders);
    StateManagerChangeData_Rebuild rebuildData = { enumerator };

    notifyStateManagerChangeCallback_(ctx_, txnReplicator_, StateManagerChangeKind_Rebuild, &rebuildData);        
}

void StateManagerChangeHandler::OnAdded(
    __in ITransactionalReplicator & source,
    __in ITransaction const & transaction,
    __in KUri const & stateProviderName,
    __in IStateProvider2 & stateProvider)
{
    CODING_ERROR_ASSERT(txnReplicator_ == &source);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&transaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);
    auto stateProviderHandle = reinterpret_cast<StateProviderHandle>(&stateProvider);

    // Is this is wrapper state provider for compat distributed dictionary state provider
    if (dynamic_cast<CompatRDStateProvider*>(&stateProvider) != nullptr)
    {
        CompatRDStateProvider* pCompatRDStateProvider = dynamic_cast<CompatRDStateProvider*>(&stateProvider);
        // Get actual data store state provider
        stateProviderHandle = dynamic_cast<TxnReplicator::IStateProvider2*>(pCompatRDStateProvider->DataStore.RawPtr());
    }

    // @TODO - Fix KTL so that KUri LPCWSTR conversion has correct const-ness
    StateManagerChangeData_SingleEntityChanged addData = { txnHandle, stateProviderHandle, (LPCWSTR) const_cast<KUri &>(stateProviderName) };

    notifyStateManagerChangeCallback_(ctx_, txnReplicator_, StateManagerChangeKind_Add, &addData);
}

void StateManagerChangeHandler::OnRemoved(
    __in ITransactionalReplicator & source,
    __in ITransaction const & transaction,
    __in KUri const & stateProviderName,
    __in IStateProvider2 & stateProvider)
{
    CODING_ERROR_ASSERT(txnReplicator_ == &source);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&transaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);
    auto stateProviderHandle = reinterpret_cast<StateProviderHandle>(&stateProvider);

    // Is this is wrapper state provider for compat distributed dictionary state provider
    if (dynamic_cast<CompatRDStateProvider*>(&stateProvider) != nullptr)
    {
        CompatRDStateProvider* pCompatRDStateProvider = dynamic_cast<CompatRDStateProvider*>(&stateProvider);
        // Get actual data store state provider
        stateProviderHandle = dynamic_cast<TxnReplicator::IStateProvider2*>(pCompatRDStateProvider->DataStore.RawPtr());
    }

    StateManagerChangeData_SingleEntityChanged removeData = { txnHandle, stateProviderHandle, (LPCWSTR) const_cast<KUri &>(stateProviderName) };

    notifyStateManagerChangeCallback_(ctx_, txnReplicator_, StateManagerChangeKind_Remove, &removeData);
}

StateManagerChangeHandler::StateManagerChangeHandler(
    __in ITransactionalReplicator *txnReplicator,
    __in fnNotifyStateManagerChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    txnReplicator_ = txnReplicator;
    notifyStateManagerChangeCallback_ = callback;
    cleanupContextCallback_ = cleanupCallback;
    ctx_ = ctx;
}

StateManagerChangeHandler::~StateManagerChangeHandler()
{
    //
    // Make sure API caller gets a chance to destroy the context
    // Note that it is possible to have a race between removing the notification and store firing another notification
    // with the cached instance. So we destroy it when store release the cached instance on stack
    //
    if (cleanupContextCallback_)
        cleanupContextCallback_(ctx_);
}
