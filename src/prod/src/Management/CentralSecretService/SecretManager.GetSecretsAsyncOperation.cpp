// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::GetSecretsAsyncOperation::GetSecretsAsyncOperation(
    Crypto const & crypto,
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    vector<SecretReference> const & secretReferences,
    bool const & includeValue,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : SecretManager::SecretsAsyncOperation(replicatedStore, partitionedReplicaId, timeout, callback, parent, activityId)
    , crypto_(crypto)
    , secretReferences_(secretReferences)
    , includeValue_(includeValue)
    , readSecrets_()
{
}

StringLiteral const SecretManager::GetSecretsAsyncOperation::TraceComponent("SecretManager::GetSecretsAsyncOperation");

StringLiteral const & SecretManager::GetSecretsAsyncOperation::get_TraceComponent() const
{
    return SecretManager::GetSecretsAsyncOperation::TraceComponent;
}

ErrorCode SecretManager::GetSecretsAsyncOperation::ValidateSecretReferences()
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

void SecretManager::GetSecretsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
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

    //
    // For the time being (Ignite release), this API (and a several others of the SecretManager) serve
    // a dual purpose; if the input specifies a name only, the operation will apply to secret resources 
    // (ie metadata collection entries); if the input specifies also a version id, the operation will
    // apply to that specific versioned secret value. Finally, if no name is specified, the operation 
    // will return a list of all secret resources. 
    // There is no API which returns the values of all versions of a secret reference; its replacement 
    // is the GetSecretVersions API.
    //
    for (SecretReference const & secretRef : this->secretReferences_)
    {
        SecretMetadataDataItem resourceDataItem(secretRef.Name);
        SecretDataItem valueDataItem(secretRef);

        if (secretRef.Name.empty())
        {
            error = ListSecrets(storeTx, readSecrets);
            if (!error.IsSuccess())
            {
                this->Complete(thisSPtr, error);
                return;
            }

            // currently we expect input collections of exactly 1 element;
            // continue or break therefore has the same effect. Revisit this
            // post 6.4.
            continue;
        }

        // in any case, the metadata item must exist. It's an additional read, but protects us from
        // 'orphaned' data items - the case where the resource is lost, but values continues to exist.
        error = storeTx.ReadExact(resourceDataItem);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCode(
                    error.ReadValue(),
                    wformatString(
                        "The secret (Name={0}) does not exist.",
                        secretRef.Name));
            }

            this->Complete(thisSPtr, error);
            return;
        }

        if (secretRef.Version.empty())
        {
            // return metadata only
            readSecrets.push_back(resourceDataItem.ToSecret());
            continue;
        }

        // continuing with the request, if a version value was requested
        error = storeTx.ReadExact(valueDataItem);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCode(
                    error.ReadValue(),
                    wformatString(
                        "The versioned secret value (Name={0}, Version={1}) does not exist.",
                        secretRef.Name,
                        secretRef.Version));
            }

            this->Complete(thisSPtr, error);
            return;
        }

        // in this case, the plaintext value is always included in the response, so decrypt it here
        Secret secret = valueDataItem.ToSecret();
        SecureString decryptedValue;

        error = this->crypto_.Decrypt(secret.Value, decryptedValue);
        if (!error.IsSuccess())
        {
            this->Complete(thisSPtr, error);
            return;
        }

        secret.Value = decryptedValue.GetPlaintext();
        readSecrets.push_back(secret);
    }

    this->readSecrets_ = readSecrets;
    this->Complete(thisSPtr, ErrorCode::Success());
}

ErrorCode SecretManager::GetSecretsAsyncOperation::End(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<Secret> & secrets)
{
    auto getOperation = AsyncOperation::End<SecretManager::GetSecretsAsyncOperation>(operationSPtr);

    if (getOperation->Error.IsSuccess())
    {
        secrets = getOperation->readSecrets_;
    }

    return getOperation->Error;
}

ErrorCode SecretManager::GetSecretsAsyncOperation::ListSecrets(
    Store::StoreTransaction const & storeTx,
    vector<Secret> & retrievedSecrets)
{
    vector<SecretMetadataDataItem> retrievedSecretResources;

    auto error = storeTx.ReadPrefix(Constants::StoreType::SecretsMetadata, L"", retrievedSecretResources);
    if (error.IsSuccess())
    {
        for (SecretMetadataDataItem const & secretEntry : retrievedSecretResources)
        {
            retrievedSecrets.push_back(secretEntry.ToSecret());
        }
    }

    return error;
}
