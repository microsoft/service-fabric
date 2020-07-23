// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::RemoveSecretsAsyncOperation::RemoveSecretsAsyncOperation(
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    vector<SecretReference> const & secretReferences,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : SecretManager::SecretsAsyncOperation(replicatedStore, partitionedReplicaId, timeout, callback, parent, activityId)
    , secretReferences_(secretReferences)
{
}

StringLiteral const SecretManager::RemoveSecretsAsyncOperation::TraceComponent("SecretManager::RemoveSecretsAsyncOperation");

StringLiteral const & SecretManager::RemoveSecretsAsyncOperation::get_TraceComponent() const
{
    return SecretManager::RemoveSecretsAsyncOperation::TraceComponent;
}

ErrorCode SecretManager::RemoveSecretsAsyncOperation::ValidateSecretReferences()
{
    for (SecretReference const & secretRef : this->secretReferences_)
    {
        auto error = secretRef.Validate();

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCode::Success();
}

void SecretManager::RemoveSecretsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    SecretManager::SecretsAsyncOperation::OnStart(thisSPtr);

    auto error = this->ValidateSecretReferences();
    if (!error.IsSuccess())
    {
        this->Complete(thisSPtr, error);
        return;
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, this->ActivityId);
    vector<Secret> readSecrets;

    for (SecretReference const & secretRef : this->secretReferences_)
    {
        vector<SecretDataItem> secretDataItems;

        // Read all versions.
        if (secretRef.Version.empty())
        {
            ErrorCode versionError;
            ErrorCode metadataError;

            // Read secrets.
            versionError = storeTx.ReadPrefix(Constants::StoreType::Secrets, SecretDataItem::ToKeyName(secretRef.Name), secretDataItems);
            if (!versionError.IsSuccess() && !versionError.IsError(ErrorCodeValue::NotFound))
            {
                this->Complete(thisSPtr, versionError);
                return;
            }
            
            // Remove secret metadata;
            SecretMetadataDataItem metadataDataItem(secretRef.Name);

            metadataError = storeTx.ReadExact(metadataDataItem);
            if (!metadataError.IsSuccess() && !metadataError.IsError(ErrorCodeValue::NotFound))
            {
                this->Complete(thisSPtr, error);
                return;
            }

            if (versionError.IsError(ErrorCodeValue::NotFound)
                && metadataError.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCode(
                    ErrorCodeValue::NotFound,
                    wformatString(
                        "The secrets (Name={0}) don't exist.",
                        secretRef.Name));

                this->Complete(thisSPtr, error);
                return;
            }

            storeTx.Delete(metadataDataItem);
        }
        // Read specific verison.
        else
        {
            SecretDataItem secretDataItem(secretRef);

            error = storeTx.ReadExact(secretDataItem);
            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    error = ErrorCode(
                        error.ReadValue(),
                        wformatString(
                            "The secret (Name={0}, Version={1}) doesn't exist.",
                            secretRef.Name,
                            secretRef.Version));
                }

                this->Complete(thisSPtr, error);
                return;
            }

            secretDataItems.push_back(secretDataItem);
        }

        for (SecretDataItem & secretDataItem : secretDataItems)
        {
            storeTx.Delete(secretDataItem);
        }
    }

    auto storeOperationSPtr = StoreTransaction::BeginCommit(
        move(storeTx),
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->CompleteStoreOperation(operationSPtr, false);
        },
        thisSPtr);

    this->CompleteStoreOperation(storeOperationSPtr, true);
}

void SecretManager::RemoveSecretsAsyncOperation::CompleteStoreOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = StoreTransaction::EndCommit(operationSPtr);

    this->Complete(operationSPtr->Parent, error);
}

ErrorCode SecretManager::RemoveSecretsAsyncOperation::End(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<SecretReference> & secretReferences)
{
    auto removalOperationSPtr = AsyncOperation::End<SecretManager::RemoveSecretsAsyncOperation>(operationSPtr);

    if (removalOperationSPtr->Error.IsSuccess())
    {
        secretReferences = removalOperationSPtr->secretReferences_;
    }

    return removalOperationSPtr->Error;
}