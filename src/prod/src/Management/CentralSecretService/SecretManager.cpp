// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

StringLiteral const TraceComponent("SecretManager");

ErrorCode ValidateSecretNameVersion(__in wstring const & name, __in wstring const & version)
{
    if (name.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Secret name should not be empty.");
    }

    if (version.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Secret version should not be empty.");
    }

    return ErrorCode::Success();
}

ErrorCode ValidateSecretNameVersion(__in SecretReference const & secretRef)
{
    return ValidateSecretNameVersion(secretRef.Name, secretRef.Version);
}

ErrorCode ValidateSecretNameVersion(__in Secret const & secret)
{
    return ValidateSecretNameVersion(secret.Name, secret.Version);
}

SecretManager::SecretManager(
    IReplicatedStore * replicatedStore,
    Store::PartitionedReplicaId const & partitionedReplicaId)
    : ComponentRoot(StoreConfig::GetConfig().EnableReferenceTracking)
    , PartitionedReplicaTraceComponent<TraceTaskCodes::CentralSecretService>(partitionedReplicaId)
    , replicatedStore_(replicatedStore)
    , crypto_()
{
}

ErrorCode SecretManager::GetSecrets(
    vector<SecretReference> const & secretReferences,
    bool includeValue,
    ActivityId const & activityId,
    __out vector<Secret> & secrets)
{
    ErrorCode error;

    WriteNoise(
        TraceComponent,
        "{0}: GetSecrets: Begins with includeValue={1}.",
        this->TraceId,
        includeValue);

    auto storeTx = StoreTransaction::Create(this->replicatedStore_, this->PartitionedReplicaId, activityId);

    for (SecretReference secretRef : secretReferences)
    {
        if (secretRef.Name.empty())
        {
            error = ErrorCode(ErrorCodeValue::SecretInvalid, L"At least, secret name must be supplied.");
            WriteNoise(
                TraceComponent,
                "{0}: GetSecrets: Invalid secret reference, ErrorCode={1}.",
                this->TraceId,
                error);
            break;
        }

        vector<SecretDataItem> secretDataItems;

        if (secretRef.Version.empty())
        {
            WriteNoise(
                TraceComponent,
                "{0}: GetSecrets: Read all secrets with name={1}.",
                this->TraceId,
                secretRef.Name);
            error = storeTx.ReadPrefix(Constants::StoreType::Secrets, secretRef.Name, secretDataItems);

            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    error = ErrorCode(error.ReadValue(), wformatString("Secrets not exist with name={0}", secretRef.Name));
                }

                WriteWarning(
                    TraceComponent,
                    "{0}: GetSecrets: Failed to read all secrets with name={1}, ErrorCode={2}.",
                    this->TraceId,
                    secretRef.Name,
                    error);
                break;
            }
        }
        else
        {
            SecretDataItem dataItem(secretRef);

            WriteNoise(
                TraceComponent,
                "{0}: GetSecrets: Read secret, name={1}, version={2}.",
                this->TraceId,
                secretRef.Name, secretRef.Version);
            error = storeTx.ReadExact(dataItem);

            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    error = ErrorCode(
                        error.ReadValue(),
                        wformatString(
                            "The version of the secret does not exist with name={0}, version={1}",
                            secretRef.Name,
                            secretRef.Version));
                }

                WriteWarning(
                    TraceComponent,
                    "{0}: GetSecrets: Failed to read secret, name={1}, version={2}, ErrorCode={3}.",
                    this->TraceId,
                    secretRef.Name, secretRef.Version,
                    error);
                break;
            }

            secretDataItems.push_back(dataItem);
        }

        for (SecretDataItem secretDataItem : secretDataItems)
        {
            Secret rawSecret = secretDataItem.ToSecret();

            if (!includeValue)
            {
                rawSecret.Value = L"";
            }
            else
            {
                SecureString decryptedValue;

                WriteNoise(
                    TraceComponent,
                    "{0}: GetSecrets: Decrypt secret value, name={1}, version={2}.",
                    this->TraceId,
                    rawSecret.Name, rawSecret.Version);

                error = this->crypto_.Decrypt(rawSecret.Value, decryptedValue);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        "{0}: GetSecrets: Failed to decrypt secret value, name={1}, version={2}, ErrorCode={3}.",
                        this->TraceId,
                        secretRef.Name, secretRef.Version,
                        error);
                    break;
                }

                rawSecret.Value = decryptedValue.GetPlaintext();
            }

            secrets.push_back(rawSecret);
        }

        if (!error.IsSuccess())
        {
            break;
        }
    }

    WriteNoise(
        TraceComponent,
        "{0}: GetSecrets: Ends with ErrorCode={1}.",
        this->TraceId,
        error);

    return error;
}

AsyncOperationSPtr SecretManager::SetSecretsAsync(
    std::vector<Secret> const & secrets,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    WriteNoise(
        TraceComponent,
        "{0}: SetSecretsAsync: Begins.",
        this->TraceId);

    auto storeTx = StoreTransaction::Create(this->replicatedStore_, this->PartitionedReplicaId, activityId);

    for (Secret secret : secrets)
    {
        // Validation begins
        error = ValidateSecretNameVersion(secret);

        if (!error.IsSuccess())
        {
            WriteNoise(
                TraceComponent,
                "{0}: SetSecretsAsync: Invalid secret, name={1}, version={2}, ErrorCode={3}.",
                this->TraceId,
                secret.Name, secret.Version,
                error);
            break;
        }

        SecretDataItem dataItem(secret.Name, secret.Version);

        WriteNoise(
            TraceComponent,
            "{0}: SetSecretsAsync: Read secret with specific version, name={1}, version={2}.",
            this->TraceId,
            secret.Name, secret.Version);
        error = storeTx.ReadExact(dataItem);

        // If the secret with the version already exists,
        // prevent from overwriting the version of the secret.
        if (error.IsSuccess())
        {
            error = ErrorCode(ErrorCodeValue::SecretVersionAlreadyExists, wformatString("Secret \"{0}\" with version \"{1}\" already exists.", secret.Name, secret.Version));
            WriteWarning(
                TraceComponent,
                "{0}: SetSecretsAsync: The secret with the version already exists., name={1}, version={2}, ErrorCode={3}.",
                this->TraceId,
                secret.Name, secret.Version,
                error);
            break;
        }

        wstring encryptedValue;

        WriteNoise(
            TraceComponent,
            "{0}: SetSecretsAsync: Encrypting secret, name={1}, version={2}.",
            this->TraceId,
            secret.Name, secret.Version);

        error = this->crypto_.Encrypt(secret.Value, encryptedValue);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: SetSecretsAsync: Faield to encrypt secret, name={1}, version={2}, ErrorCode={3}.",
                this->TraceId,
                secret.Name, secret.Version,
                error);
            break;
        }

        secret.Value = encryptedValue;

        // Create new version of secret.
        WriteNoise(
            TraceComponent,
            "{0}: SetSecretsAsync: Add new version of the secret, name={1}, version={2}.",
            this->TraceId,
            secret.Name, secret.Version);
        error = storeTx.Insert(SecretDataItem(secret));

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: SetSecretsAsync: Faield to Add new version of the secret, name={1}, version={2}, ErrorCode={3}.",
                this->TraceId,
                secret.Name, secret.Version,
                error);
            break;
        }
    }

    WriteNoise(
        TraceComponent,
        "{0}: SetSecretsAsync: Ends with ErrorCode={1}.",
        this->TraceId,
        error);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}

AsyncOperationSPtr SecretManager::RemoveSecretsAsync(
    std::vector<SecretReference> const & secretReferences,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const& parent,
    ActivityId const & activityId)
{
    ErrorCode error;

    WriteNoise(
        TraceComponent,
        "{0}: RemoveSecretsAsync: Begins.",
        this->TraceId);

    auto storeTx = StoreTransaction::Create(this->replicatedStore_, this->PartitionedReplicaId, activityId);
    vector<SecretDataItem> secretDataItems;

    for (SecretReference secretRef : secretReferences)
    {
        if (secretRef.Name.empty())
        {
            error = ErrorCode(ErrorCodeValue::SecretInvalid, L"Secret name should not be empty.");
            WriteNoise(
                TraceComponent,
                "{0}: SetSecretsAsync: Invalid secret, name={1}, version={2}, ErrorCode={3}.",
                this->TraceId,
                secretRef.Name, secretRef.Version,
                error);
            break;
        }

        if (secretRef.Version.empty())
        {
            WriteNoise(
                TraceComponent,
                "{0}: RemoveSecretsAsync: Read all versions of the secret, name={1}.",
                this->TraceId,
                secretRef.Name);
            error = storeTx.ReadPrefix(Constants::StoreType::Secrets, secretRef.Name, secretDataItems);

            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    WriteNoise(
                        TraceComponent,
                        "{0}: RemoveSecretsAsync: No secrets found with name={1}, ErrorCode={2}.",
                        this->TraceId,
                        secretRef.Name,
                        error);
                    continue;
                }
                else
                {
                    WriteWarning(
                        TraceComponent,
                        "{0}: RemoveSecretsAsync: Failed to Read all versions of the secret, name={1}, ErrorCode={2}.",
                        this->TraceId,
                        secretRef.Name,
                        error);
                    break;
                }
            }
        }
        else
        {
            secretDataItems.push_back(SecretDataItem(secretRef));
        }
    }

    if (error.IsSuccess()) {
        for (SecretDataItem dataItem : secretDataItems)
        {
            WriteNoise(
                TraceComponent,
                "{0}: RemoveSecretsAsync: Remove the version of secret, name={1}, version={2}.",
                this->TraceId,
                dataItem.Name, dataItem.Version);
            error = storeTx.DeleteIfFound(dataItem);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}:RemoveSecretsAsync: Failed to remove the version of secret, name={1}, version={2}, ErrorCode={3}.",
                    this->TraceId,
                    dataItem.Name, dataItem.Version,
                    error);
                break;
            }
        }
    }

    WriteNoise(
        TraceComponent,
        "{0}: RemoveSecretsAsync: Ends with ErrorCode={1}.",
        this->TraceId,
        error);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }

    return StoreTransaction::BeginCommit(move(storeTx), timeout, callback, parent);
}
