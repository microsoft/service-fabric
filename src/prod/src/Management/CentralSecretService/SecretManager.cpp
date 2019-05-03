// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::SecretManager(
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId)
    : ComponentRoot(StoreConfig::GetConfig().EnableReferenceTracking)
    , replicatedStore_(replicatedStore)
    , partitionedReplicaId_(partitionedReplicaId)
    , crypto_()
{
}

AsyncOperationSPtr SecretManager::BeginGetSecrets(
    vector<SecretReference> const & secretReferences,
    bool includeValue,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parentSPtr,
    ActivityId const & activityId)
{
    return AsyncOperation::CreateAndStart<SecretManager::AutoRetryAsyncOperation>(
        [&, this](AsyncCallback const & callback, AsyncOperationSPtr const & parentSPtr)
    {
        return AsyncOperation::CreateAndStart<SecretManager::GetSecretsAsyncOperation>(
            this->CryptoAgent,
            this->ReplicatedStore,
            this->PartitionedReplicaId,
            secretReferences,
            includeValue,
            timeout,
            callback,
            parentSPtr,
            activityId);
    },
        timeout,
        callback,
        parentSPtr,
        activityId);
}

ErrorCode SecretManager::EndGetSecrets(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<Secret> & secrets)
{
    AsyncOperationSPtr getOperationSPtr;
    ErrorCode error = SecretManager::AutoRetryAsyncOperation::End(operationSPtr, getOperationSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }

    return SecretManager::GetSecretsAsyncOperation::End(getOperationSPtr, secrets);
}

AsyncOperationSPtr SecretManager::BeginSetSecrets(
    vector<Secret> const & secrets,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parentSPtr,
    ActivityId const & activityId)
{
    return AsyncOperation::CreateAndStart<SecretManager::AutoRetryAsyncOperation>(
        [&, this](AsyncCallback const & callback, AsyncOperationSPtr const & parentSPtr)
    {
        return AsyncOperation::CreateAndStart<SecretManager::SetSecretsAsyncOperation>(
            this->CryptoAgent,
            this->ReplicatedStore,
            this->PartitionedReplicaId,
            secrets,
            timeout,
            callback,
            parentSPtr,
            activityId);
    },
        timeout,
        callback,
        parentSPtr,
        activityId);
}

ErrorCode SecretManager::EndSetSecrects(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<Secret> & secrets)
{
    AsyncOperationSPtr setOperationSPtr;
    ErrorCode error = SecretManager::AutoRetryAsyncOperation::End(operationSPtr, setOperationSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }

    return SecretManager::SetSecretsAsyncOperation::End(setOperationSPtr, secrets);
}

AsyncOperationSPtr SecretManager::BeginRemoveSecrets(
    vector<SecretReference> const & secretReferences,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parentSPtr,
    ActivityId const & activityId)
{
    return AsyncOperation::CreateAndStart<SecretManager::AutoRetryAsyncOperation>(
        [&, this](AsyncCallback const & callback, AsyncOperationSPtr const & parentSPtr)
    {
        return AsyncOperation::CreateAndStart<SecretManager::RemoveSecretsAsyncOperation>(
            this->ReplicatedStore,
            this->PartitionedReplicaId,
            secretReferences,
            timeout,
            callback,
            parentSPtr,
            activityId);
    },
        timeout,
        callback,
        parentSPtr,
        activityId);
}

ErrorCode SecretManager::EndRemoveSecrets(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<SecretReference> & secretReferences)
{
    AsyncOperationSPtr removalOperationSPtr;
    ErrorCode error = SecretManager::AutoRetryAsyncOperation::End(operationSPtr, removalOperationSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }

    return SecretManager::RemoveSecretsAsyncOperation::End(removalOperationSPtr, secretReferences);
}

AsyncOperationSPtr SecretManager::BeginGetSecretVersions(
    vector<SecretReference> const & secretReferences,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parentSPtr,
    ActivityId const & activityId)
{
    return AsyncOperation::CreateAndStart<SecretManager::AutoRetryAsyncOperation>(
        [&, this](AsyncCallback const & callback, AsyncOperationSPtr const & parentSPtr)
    {
        return AsyncOperation::CreateAndStart<SecretManager::GetSecretVersionsAsyncOperation>(
            this->ReplicatedStore,
            this->PartitionedReplicaId,
            secretReferences,
            timeout,
            callback,
            parentSPtr,
            activityId);
    },
        timeout,
        callback,
        parentSPtr,
        activityId);
}

ErrorCode SecretManager::EndGetSecretVersions(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<SecretReference> & secretReferences)
{
    AsyncOperationSPtr removalOperationSPtr;
    ErrorCode error = SecretManager::AutoRetryAsyncOperation::End(operationSPtr, removalOperationSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }

    return SecretManager::GetSecretVersionsAsyncOperation::End(removalOperationSPtr, secretReferences);
}
