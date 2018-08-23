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

StringLiteral const TraceComponent("RemoveSecretsAsyncOperation");

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
    SecretReferencesDescription description;

    WriteInfo(TraceComponent,
        "{0} RemoveSecrets: Begins.",
        this->TraceId);

    if (this->RequestMsg.GetBody(description))
    {
        vector<wstring> resourceNames;

        transform(
            description.SecretReferences.begin(),
            description.SecretReferences.end(),
            back_inserter(resourceNames),
            [](SecretReference const & secretRef)->wstring
            {
                return secretRef.ToResourceName();
            });

        WriteInfo(TraceComponent,
            "{0}: RemoveSecrets: Unregistering secrets resources.",
            this->TraceId);

        this->ResourceManager.UnregisterResources(
            resourceNames,
            this->RemainingTime,
            [this, thisSPtr, description](AsyncOperationSPtr const & operationSPtr)
            {
                this->UnregisterResourcesCallback(thisSPtr, operationSPtr, description);
            },
            thisSPtr,
            this->ActivityId);
    }
    else
    {
        error = ErrorCode::FromNtStatus(this->RequestMsg.GetStatus());
        WriteWarning(TraceComponent,
            "{0}: RemoveSecrets: Failed to get the body of the request, ErrorCode: {1}.",
            this->TraceId,
            error);
        this->Complete(thisSPtr, error);
    }
}

void RemoveSecretsAsyncOperation::OnCompleted()
{
    WriteTrace(
        this->Error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
        TraceComponent,
        "{0} RemoveSecrets: Ended with ErrorCode: {1}",
        this->TraceId,
        this->Error);
}

void RemoveSecretsAsyncOperation::UnregisterResourcesCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr,
    SecretReferencesDescription const & description)
{
    auto error = AsyncOperation::End(operationSPtr);

    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent,
            "{0}: RemoveSecrets: Failed to unregistering secret resources, ErrorCode: {1}.",
            this->TraceId,
            error);
        this->Complete(thisSPtr, error);
        return;
    }

    WriteInfo(TraceComponent,
        "{0}: RemoveSecrets: Removing secrets.",
        this->TraceId);

    this->SecretManager.RemoveSecretsAsync(
        description.SecretReferences,
        this->RemainingTime,
        [this, thisSPtr, description](AsyncOperationSPtr const & operationSPtr)
        {
            this->RemoveSecretsCallback(thisSPtr, operationSPtr, description);
        },
        thisSPtr,
        this->ActivityId);
}

void RemoveSecretsAsyncOperation::RemoveSecretsCallback(
    AsyncOperationSPtr const & thisSPtr,
    AsyncOperationSPtr const & operationSPtr,
    SecretReferencesDescription const & description)
{
    auto error = StoreTransaction::EndCommit(operationSPtr);

    if (!error.IsSuccess())
    {
        WriteWarning(TraceComponent,
            "{0}: RemoveSecrets: Failed to remove secrets, ErrorCode: {1}.",
            this->TraceId,
            error);
    }
    else
    {
        this->SetReply(make_unique<SecretReferencesDescription>(description));
    }

    this->Complete(thisSPtr, error);
}