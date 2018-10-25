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

StringLiteral const TraceComponent("SetSecretsAsyncOperation");

SetSecretsAsyncOperation::SetSecretsAsyncOperation(
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

void SetSecretsAsyncOperation::Execute(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);
    SecretsDescription description;

    WriteInfo(TraceComponent,
        "{0}: SetSecrets: Begins.",
        this->TraceId);

    if (this->RequestMsg.GetBody(description))
    {
        vector<SecretReference> secretReferences;

        transform(
            description.Secrets.begin(),
            description.Secrets.end(),
            back_inserter(secretReferences),
            [](Secret const & secret)
            {
                return SecretReference(secret.Name, secret.Version);
            });

        WriteInfo(TraceComponent,
            "{0}: SetSecrets: Setting secrets.",
            this->TraceId);

        this->SecretManager.SetSecretsAsync(
            description.Secrets,
            this->RemainingTime,
            [this, thisSPtr, secretReferences](AsyncOperationSPtr const & operationSPtr)
            {
                this->SetSecretsCallback(thisSPtr, operationSPtr, secretReferences);
            },
            thisSPtr,
            this->ActivityId);
    }
    else
    {
        error = ErrorCode::FromNtStatus(this->RequestMsg.GetStatus());
        WriteWarning(TraceComponent,
            "{0}: SetSecrets: Failed to get the body of the request, ErrorCode: {1}.",
            this->TraceId,
            error);

        this->Complete(thisSPtr, error);
    }
}

void SetSecretsAsyncOperation::OnCompleted()
{
    WriteTrace(
        this->Error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
        TraceComponent,
        "{0}: SetSecrets: Ended with ErrorCode: {1}",
        this->TraceId,
        this->Error);
}

void SetSecretsAsyncOperation::SetSecretsCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr,
    vector<SecretReference> const & secretReferences)
{
    auto error = StoreTransaction::EndCommit(operationSPtr);

    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent,
            "{0}: SetSecrets: Failed to setting secrets, ErrorCode: {1}.",
            this->TraceId,
            error);

        this->Complete(thisSPtr, error);
        return;
    }

    WriteInfo(TraceComponent,
        "{0}: SetSecrets: Registering secret resources.",
        this->TraceId);

    vector<wstring> resourceNames;

    transform(
        secretReferences.begin(),
        secretReferences.end(),
        back_inserter(resourceNames),
        [](SecretReference const & secretRef)
        {
            return secretRef.ToResourceName();
        });

    this->ResourceManager.RegisterResources(
        resourceNames,
        this->RemainingTime,
        [this, thisSPtr, secretReferences](AsyncOperationSPtr const & operationSPtr)
        {
            this->RegisterResourcesCallback(thisSPtr, operationSPtr, secretReferences);
        },
        thisSPtr,
        this->ActivityId);
}

void SetSecretsAsyncOperation::RegisterResourcesCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr,
    vector<SecretReference> const & secretReferences)
{
    auto error = AsyncOperation::End(operationSPtr);

    if (!error.IsSuccess())
    {
        WriteError(TraceComponent,
            "{0}: SetSecrets: Failed to register resources, ErrorCode: {1}.",
            this->TraceId,
            error);
    }
    else
    {
        this->SetReply(make_unique<SecretReferencesDescription>(secretReferences));
    }

    this->Complete(thisSPtr, error);
}
