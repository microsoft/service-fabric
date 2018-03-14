// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

class StateManagerChangeHandler 
    : public KObject<StateManagerChangeHandler>
    , public KShared<StateManagerChangeHandler>
    , public TxnReplicator::IStateManagerChangeHandler
{
    K_FORCE_SHARED(StateManagerChangeHandler);
    K_SHARED_INTERFACE_IMP(IStateManagerChangeHandler);

public:
    static HRESULT Create(
        __in KAllocator& allocator,
        __in TxnReplicator::ITransactionalReplicator *txnReplicator,
        __in fnNotifyStateManagerChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx,
        __out StateManagerChangeHandler::SPtr& result);

public :
    //
    // IStateManagerChangeHandler methods
    //
    virtual void OnRebuilt(
        __in TxnReplicator::ITransactionalReplicator & source,
        __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders);

    virtual void OnAdded(
        __in TxnReplicator::ITransactionalReplicator & source,
        __in TxnReplicator::ITransaction const & transaction,
        __in KUri const & stateProviderName,
        __in TxnReplicator::IStateProvider2 & stateProvider);

    virtual void OnRemoved(
        __in TxnReplicator::ITransactionalReplicator & source,
        __in TxnReplicator::ITransaction const & transaction,
        __in KUri const & stateProviderName,
        __in TxnReplicator::IStateProvider2 & stateProvider);

private:
    StateManagerChangeHandler(
        __in TxnReplicator::ITransactionalReplicator *txnReplicator,
        __in fnNotifyStateManagerChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void* ctx);

private:
    fnNotifyStateManagerChangeCallback notifyStateManagerChangeCallback_;
    fnCleanupContextCallback cleanupContextCallback_;
	TxnReplicator::ITransactionalReplicator *txnReplicator_;
    void* ctx_;
};

