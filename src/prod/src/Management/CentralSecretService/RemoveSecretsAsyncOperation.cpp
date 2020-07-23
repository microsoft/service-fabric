// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace std;
using namespace ClientServerTransport;
using namespace Store;
using namespace ServiceModel;

using namespace Management::ResourceManager;
using namespace Management::CentralSecretService;

StringLiteral const RemoveSecretsAsyncOperation::TraceComponent("CentralSecretServiceReplica::RemoveSecretsAsyncOperation");

RemoveSecretsAsyncOperation::RemoveSecretsAsyncOperation(
    Management::CentralSecretService::SecretManager & secretManager,
    IpcResourceManagerService & resourceManager,
    MessageUPtr requestMsg,
    IpcReceiverContextUPtr receiverContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ClientRequestAsyncOperation(
        secretManager,
        resourceManager,
        move(requestMsg),
        move(receiverContext),
        timeout,
        callback,
        parent)
{
}

void RemoveSecretsAsyncOperation::Execute(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);
    
    if (this->RequestMsg.GetBody(this->description_))
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Getting existing secrets resources ...",
            this->TraceId,
            this->TraceComponent);

        auto secretsOperationSPtr = this->SecretManager.BeginGetSecrets(
            this->description_.SecretReferences,
            false,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operationSPtr)
            {
                this->CompleteGetSecretsOperation(operationSPtr, false);
            },
            thisSPtr,
            this->ActivityId);

        this->CompleteGetSecretsOperation(secretsOperationSPtr, true);
    }
    else
    {
        error = ErrorCode::FromNtStatus(this->RequestMsg.GetStatus());
        WriteWarning(
            this->TraceComponent,
            "{0}: {1} -> Failed to get the body of the request. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
        this->Complete(thisSPtr, error);
    }
}

void RemoveSecretsAsyncOperation::CompleteGetSecretsOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    vector<Secret> readSecrets;

    auto error = this->SecretManager.EndGetSecrets(operationSPtr, readSecrets);
    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to get secrets. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
        this->Complete(operationSPtr->Parent, error);
        return;
    }

    if (readSecrets.size() <= 0)
    {
        this->Complete(operationSPtr->Parent, ErrorCode(ErrorCodeValue::NotFound));
        return;
    }

    vector<wstring> resourceNames;

    transform(
        readSecrets.begin(),
        readSecrets.end(),
        back_inserter(resourceNames),
        [](Secret const & secret)->wstring
        {
            return secret.ToResourceName();
        });

    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Unregistering secrets resources ...",
        this->TraceId,
        this->TraceComponent);

    auto resourcesOperationSPtr = this->ResourceManager.UnregisterResources(
        resourceNames,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->CompleteUnregisterResourcesOperation(operationSPtr, false);
        },
        operationSPtr->Parent,
        this->ActivityId);

    this->CompleteUnregisterResourcesOperation(resourcesOperationSPtr, true);
}

void RemoveSecretsAsyncOperation::CompleteUnregisterResourcesOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End(operationSPtr);
    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to unregister resources. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
        this->Complete(operationSPtr->Parent, error);
        return;
    }

    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Removing secrets ...",
        this->TraceId,
        this->TraceComponent);

    auto secretsOperationSPtr = this->SecretManager.BeginRemoveSecrets(
        this->description_.SecretReferences,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->CompleteRemoveSecretsOperation(operationSPtr, false);
        },
        operationSPtr->Parent,
        this->ActivityId);

    this->CompleteRemoveSecretsOperation(secretsOperationSPtr, true);
}

void RemoveSecretsAsyncOperation::CompleteRemoveSecretsOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    vector<SecretReference> removedSecretRefs;

    auto error = this->SecretManager.EndRemoveSecrets(operationSPtr, removedSecretRefs);
    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to remove secrets. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
    }
    else
    {
        this->SetReply(make_unique<SecretReferencesDescription>(removedSecretRefs));
    }

    this->Complete(operationSPtr->Parent, error);
}