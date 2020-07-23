// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::GetSecretVersionsAsyncOperation::GetSecretVersionsAsyncOperation(
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    vector<SecretReference> const & secretReferences,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : SecretManager::SecretsAsyncOperation(replicatedStore, partitionedReplicaId, timeout, callback, parent, activityId)
    , incomingSecretReferences_(secretReferences)
{
}

StringLiteral const SecretManager::GetSecretVersionsAsyncOperation::TraceComponent("SecretManager::GetSecretVersionsAsyncOperation");

StringLiteral const & SecretManager::GetSecretVersionsAsyncOperation::get_TraceComponent() const
{
    return SecretManager::GetSecretVersionsAsyncOperation::TraceComponent;
}

ErrorCode SecretManager::GetSecretVersionsAsyncOperation::ValidateSecretReferences()
{
    for (SecretReference const & secretRef : this->incomingSecretReferences_)
    {
        auto error = secretRef.Validate(
            true,   // strict validation
            false); // as a versioned secret reference

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCode::Success();
}

void SecretManager::GetSecretVersionsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    SecretManager::SecretsAsyncOperation::OnStart(thisSPtr);

    auto error = this->ValidateSecretReferences();
    if (!error.IsSuccess())
    {
        this->Complete(thisSPtr, error);
        return;
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, this->ActivityId);
    vector<SecretReference> retrievedSecretVersions;

    for (SecretReference const & secretRef : this->incomingSecretReferences_)
    {
        vector<SecretDataItem> secretDataItems;

        // Read the corresponding metadata entry
        SecretMetadataDataItem secretResource(secretRef.Name);
        error = storeTx.ReadExact(secretResource);
        if (!error.IsSuccess())
        {
            this->Complete(thisSPtr, error);
            return;
        }

        // Read versions.
        error = storeTx.ReadPrefix(Constants::StoreType::Secrets, SecretDataItem::ToKeyName(secretRef.Name), secretDataItems);
        if (!error.IsSuccess())
        {
            this->Complete(thisSPtr, error);
            return;
        }

        // convert each data item into a secret reference
        for (SecretDataItem const & item : secretDataItems)
        {
            retrievedSecretVersions.push_back(item.ToSecretReference());
        }
    }

    this->retrievedSecretVersionReferences_ = retrievedSecretVersions;
    this->Complete(thisSPtr, ErrorCode::Success());
}

ErrorCode SecretManager::GetSecretVersionsAsyncOperation::End(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<SecretReference> & secretReferences)
{
    auto listVersionsOperationSPtr = AsyncOperation::End<SecretManager::GetSecretVersionsAsyncOperation>(operationSPtr);

    if (listVersionsOperationSPtr->Error.IsSuccess())
    {
        secretReferences = listVersionsOperationSPtr->retrievedSecretVersionReferences_;
    }

    return listVersionsOperationSPtr->Error;
}