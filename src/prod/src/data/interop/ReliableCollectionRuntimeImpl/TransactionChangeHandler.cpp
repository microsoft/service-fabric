// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Utilities;
using namespace TStore;
using namespace TxnReplicator;

HRESULT TransactionChangeHandler::Create(
    __in KAllocator& allocator,
    __in ITransactionalReplicator *txnReplicator,
    __in fnNotifyTransactionChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void* ctx,
    __out TransactionChangeHandler::SPtr& result)
{
    TransactionChangeHandler* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) TransactionChangeHandler(txnReplicator, callback, cleanupCallback, ctx);
    if (!pointer)
        return E_OUTOFMEMORY;

    result = pointer;
    return S_OK;
}

void TransactionChangeHandler::OnTransactionCommitted(
    __in ITransactionalReplicator & source,
    __in ITransaction const & transaction)
{
    CODING_ERROR_ASSERT(txnReplicator_ == &source);

    auto txn = const_cast<TxnReplicator::Transaction *>(dynamic_cast<TxnReplicator::Transaction const *>(&transaction));
    auto txnHandle = reinterpret_cast<TransactionHandle>(txn);

    TransactionChangeData_Commit commitData = { txnHandle };
    
    notifyTransactionChangeCallback_(ctx_, txnReplicator_, TransactionChangeKind_Commit, &commitData);
}

TransactionChangeHandler::TransactionChangeHandler(
    __in ITransactionalReplicator *txnReplicator,
    __in fnNotifyTransactionChangeCallback callback,
    __in fnCleanupContextCallback cleanupCallback,
    __in void *ctx)
{
    txnReplicator_ = txnReplicator;
    notifyTransactionChangeCallback_ = callback;
    cleanupContextCallback_ = cleanupCallback;
    ctx_ = ctx;
}

TransactionChangeHandler::~TransactionChangeHandler()
{
    //
    // Make sure API caller gets a chance to destroy the context
    // Note that it is possible to have a race between removing the notification and store firing another notification
    // with the cached instance. So we destroy it when store release the cached instance on stack
    //
    if (cleanupContextCallback_)
        cleanupContextCallback_(ctx_);
}
