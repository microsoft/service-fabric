// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretManager::SetSecretsAsyncOperation::SetSecretsAsyncOperation(
    Crypto const & crypto,
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    vector<Secret> const & secrets,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : SecretManager::SecretsAsyncOperation(replicatedStore, partitionedReplicaId, timeout, callback, parent, activityId)
    , crypto_(crypto)
    , secrets_(secrets)
{
}

StringLiteral const & SecretManager::SetSecretsAsyncOperation::get_TraceComponent() const
{
    return SecretManager::SetSecretsAsyncOperation::TraceComponent;
}

StringLiteral const SecretManager::SetSecretsAsyncOperation::TraceComponent("SecretManager::SetSecretsAsyncOperation");

ErrorCode SecretManager::SetSecretsAsyncOperation::ValidateSecrets()
{
    for (Secret const & secret : this->secrets_)
    {
        auto error = secret.Validate();

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCode::Success();
}

void SecretManager::SetSecretsAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    SecretManager::SecretsAsyncOperation::OnStart(thisSPtr);

    // Validates all the secrets.
    auto error = this->ValidateSecrets();
    if (!error.IsSuccess())
    {
        this->Complete(thisSPtr, error);
        return;
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, this->ActivityId);
    
    for (Secret & secret : this->secrets_)
    {
        //
        // At this point, this incoming object represents either a secret 
        // resource or a secret value. Secret resources are added to the 
        // collection if a matching entry does not exist, and updated otherwise.
        // Secret values can only be added if a match does not already exist.
        //
        // Note: since the SecretStore API takes a batch of secret objects, it 
        // is possible that a resource and a corresponding value are added in the
        // same batch. This scenario will fail with the current implementation, 
        // since adding a value expects the corresponding resource to pre-exist.
        // This is ok, because the public API does not yet allow batching.
        //
        // algorithm:
        // 
        //   retrieve the corresponding metadata entry for this name
        //   if not found
        //      if this object is a resource
        //          put object
        //          continue
        //      fail
        //  else
        //      if this object is a resource
        //          verify the only difference is description 
        //      else
        //          copy metadata from parent
        //      put object
        //      continue
        //      
        auto isSecretValue = !secret.Value.empty();
        auto result = false;
        SecretMetadataDataItem existingResource(secret.Name);
        error = storeTx.ReadExact(existingResource);

        // fail if we can't retrieve the resource, or a value is added for a non-existing resource
        if (!error.IsSuccess()
            && (!error.IsError(ErrorCodeValue::NotFound)
                || isSecretValue))
        {
            this->Complete(thisSPtr, error);
            return;
        }

        if (isSecretValue)
        {
            result = TryAddOrUpdateSecretValueToStoreTransaction(storeTx, secret, existingResource, error);
        }
        else
        {
            // if here, the resource exists if the ReadExact operation succeeded.
            auto resourceExists = error.IsSuccess();
            result = TryAddOrUpdateSecretResourceToStoreTransaction(storeTx, secret, existingResource, resourceExists, error);
        }

        if (!result)
        {
            this->Complete(thisSPtr, error);
            return;
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

void SecretManager::SetSecretsAsyncOperation::CompleteStoreOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = StoreTransaction::EndCommit(operationSPtr);

    this->Complete(operationSPtr->Parent, error);
}

ErrorCode SecretManager::SetSecretsAsyncOperation::End(
    AsyncOperationSPtr const & operationSPtr,
    __out vector<Secret> & secrets)
{
    auto setOperation = AsyncOperation::End<SecretManager::SetSecretsAsyncOperation>(operationSPtr);

    if (setOperation->Error.IsSuccess())
    {
        secrets = setOperation->secrets_;
    }

    return setOperation->Error;
}

bool SecretManager::SetSecretsAsyncOperation::TryAddOrUpdateSecretValueToStoreTransaction(
    Store::StoreTransaction & storeTx,
    Secret & secret,
    SecretMetadataDataItem & existingResource,
    ErrorCode & error)
{
    SecretDataItem existingVersion(secret.Name, secret.Version);

    // attempt to retrieve a matching existing value
    error = storeTx.ReadExact(existingVersion);
    auto valueExists = error.IsSuccess();

    if (!valueExists
        && !error.IsError(ErrorCodeValue::NotFound))
    {
        // abort on errors other than NotFound
        return false;
    }

    if (valueExists)
    {
        // currently an update is not permitted on existing secret values, unless the PUT is
        // idempotent; neither the version nor the value may be changed after insertion. The 
        // other fields (description, content type) are not exposed in the public API model, 
        // and so they cannot be updated either. Consider relaxing this if/when any properties 
        // become mutable.
        //
        // Checking for idempotency requires decrypting the existing value, as encryption 
        // may use a different data encryption key at various times. 
        SecureString decryptedValue;
        error = this->crypto_.Decrypt(existingVersion.Value, decryptedValue);
        if (!error.IsSuccess())
        {
            return false;
        }

        // build 2 data items for a deep comparison; the incoming object may be missing fields
        // such as content type or description. Invoke the == operator, rather than performing 
        // a direct string comparison here, as the logic may be relaxed or otherwise changed.
        SecretDataItem plaintextIncomingVersion(secret.Name,
            secret.Version,
            secret.Value,
            secret.Kind.empty() ? existingVersion.Kind : secret.Kind,
            secret.ContentType.empty() ? existingVersion.ContentType : secret.ContentType,
            secret.Description.empty() ? existingVersion.Description : secret.Description);
        SecretDataItem plaintextExistingVersion(existingVersion.Name,
            existingVersion.Version,
            decryptedValue.GetPlaintext(),
            existingVersion.Kind,
            existingVersion.ContentType,
            existingVersion.Description);

        if (plaintextExistingVersion == plaintextIncomingVersion)
        {
            // no op
            return true;
        }

        error = ErrorCode(ErrorCodeValue::SecretVersionAlreadyExists, L"An existing secret's value or version cannot be changed.");
        return false;
    }

    // this is a new version; proceed with encrypting its plaintext value
    wstring encryptedValue;
    error = this->crypto_.Encrypt(secret.Value, encryptedValue);
    if (!error.IsSuccess())
    {
        return false;
    }

    // build an item representing the encrypted incoming value and the parent resource's metadata.
    SecretDataItem encryptedIncomingVersion(secret.Name,
        secret.Version,
        encryptedValue,
        existingResource.Kind,
        existingResource.ContentType,
        existingResource.Description);
    secret.Value = L"";                 // plaintext value is not returned back
    storeTx.Insert(encryptedIncomingVersion);

    // update the latest version number on the existing resource
    existingResource.LatestVersionNumber++;
    storeTx.Update(existingResource);

    return true;
}

bool SecretManager::SetSecretsAsyncOperation::TryAddOrUpdateSecretResourceToStoreTransaction(
    Store::StoreTransaction & storeTx,
    Secret const & secret,
    SecretMetadataDataItem & existingResource,
    bool resourceExists,
    ErrorCode & error)
{
    SecretMetadataDataItem incomingResource(secret);

    if (!resourceExists)
    {
        storeTx.Insert(incomingResource);
    }
    else
    {
        // ARM requires that idempotent PUTs succeed - this is a case sensitive comparison of each field. 
        if (incomingResource == existingResource)
        {
            // no-op
            return true;
        }

        // check whether this is an allowed update; this means (currently) only the description may differ
        if (!existingResource.TryUpdateTo(incomingResource))
        {
            error = ErrorCode(ErrorCodeValue::SecretTypeCannotBeChanged, L"An existing secret's kind, content type or name cannot be changed.");

            return false;
        }

        storeTx.Update(existingResource);
    }

    return true;
}