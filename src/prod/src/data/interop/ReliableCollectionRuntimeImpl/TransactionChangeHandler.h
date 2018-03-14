// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

class TransactionChangeHandler 
    : public KObject<TransactionChangeHandler>
    , public KShared<TransactionChangeHandler>
    , public TxnReplicator::ITransactionChangeHandler
{
    K_FORCE_SHARED(TransactionChangeHandler);
    K_SHARED_INTERFACE_IMP(ITransactionChangeHandler);

public:
    static HRESULT Create(
        __in KAllocator& allocator,
        __in TxnReplicator::ITransactionalReplicator *txnReplicator,
        __in fnNotifyTransactionChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx,
        __out TransactionChangeHandler::SPtr& result);

public :
    //
    // ITransactionChangeHandler methods
    //
    virtual void OnTransactionCommitted(
        __in TxnReplicator::ITransactionalReplicator & source,
        __in TxnReplicator::ITransaction const & transaction);

private:
    TransactionChangeHandler(
        __in TxnReplicator::ITransactionalReplicator *txnReplicator,
        __in fnNotifyTransactionChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx);

private:
    fnNotifyTransactionChangeCallback notifyTransactionChangeCallback_;
    fnCleanupContextCallback cleanupContextCallback_;
    TxnReplicator::ITransactionalReplicator *txnReplicator_;
    void* ctx_;
};

