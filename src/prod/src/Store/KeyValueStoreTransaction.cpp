// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

StringLiteral const TraceComponent("Transaction");

KeyValueStoreTransaction::KeyValueStoreTransaction(
    Guid const & id,
    FABRIC_TRANSACTION_ISOLATION_LEVEL const isolationLevel,
    IStoreBase::TransactionSPtr const & replicatedStoreTx,
    DeadlockDetectorSPtr const & deadlockDetector)
    : Api::ITransaction()
    , ComponentRoot()
    , id_(id)
    , isolationLevel_(isolationLevel)
    , replicatedStoreTx_(replicatedStoreTx)
    , deadlockDetector_(deadlockDetector)
{
    this->SetTraceId(id.ToString());

    WriteNoise(TraceComponent,"{0} ctor",this->TraceId);
}

KeyValueStoreTransaction::~KeyValueStoreTransaction()
{
    WriteNoise(TraceComponent,"{0} ~dtor",this->TraceId);
}

Guid KeyValueStoreTransaction::get_Id()
{
    return id_;
}

FABRIC_TRANSACTION_ISOLATION_LEVEL KeyValueStoreTransaction::get_IsolationLevel()
{
    return isolationLevel_;
}

AsyncOperationSPtr KeyValueStoreTransaction::BeginCommit(
    TimeSpan const & timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    deadlockDetector_->TrackAssertEvent(Constants::LifecycleCommit, id_, StoreConfig::GetConfig().LifecycleCommitAssertTimeout);

    return replicatedStoreTx_->BeginCommit(timeout, callback, parent);
}

ErrorCode KeyValueStoreTransaction::EndCommit(
    AsyncOperationSPtr const & asyncOperation, 
    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
{
    deadlockDetector_->UntrackAssertEvent(Constants::LifecycleCommit, id_);

    return replicatedStoreTx_->EndCommit(asyncOperation, commitSequenceNumber);
}

void KeyValueStoreTransaction::Rollback()
{
    return replicatedStoreTx_->Rollback();
}
