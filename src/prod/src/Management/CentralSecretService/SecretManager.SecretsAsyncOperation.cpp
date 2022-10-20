// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::SecretsAsyncOperation::SecretsAsyncOperation(
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : TimedAsyncOperation(timeout, callback, parent)
    , replicatedStore_(replicatedStore)
    , partitionedReplicaId_(partitionedReplicaId)
    , activityId_(activityId)
{
}

SecretManager::SecretsAsyncOperation::~SecretsAsyncOperation()
{
}

void SecretManager::SecretsAsyncOperation::OnTimeout(AsyncOperationSPtr const & operationSPtr)
{
    WriteWarning(
        this->TraceComponent,
        "{0}: {1} -> Timeout.",
        this->ActivityId,
        this->TraceComponent);

    TimedAsyncOperation::OnTimeout(operationSPtr);
}

void SecretManager::SecretsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Begins.",
        this->ActivityId,
        this->TraceComponent);

    TimedAsyncOperation::OnStart(thisSPtr);
}

void SecretManager::SecretsAsyncOperation::OnCompleted()
{
    TimedAsyncOperation::OnCompleted();

    if (this->Error.IsSuccess())
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Ended.",
            this->ActivityId,
            this->TraceComponent);
    }
    else 
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Ended. Error: {2}",
            this->ActivityId,
            this->TraceComponent,
            this->Error);
    }
}

void SecretManager::SecretsAsyncOperation::OnCancel()
{
    TimedAsyncOperation::OnCancel();

    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Cancelled.",
        this->ActivityId,
        this->TraceComponent);
}

StoreTransaction SecretManager::SecretsAsyncOperation::CreateTransaction()
{
    return StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, this->ActivityId);
}

void SecretManager::SecretsAsyncOperation::Complete(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & errorCode)
{
    if (!this->TryComplete(thisSPtr, errorCode))
    {
        Assert::CodingError("Failed to complete {0}.", this->TraceComponent);
    }
}