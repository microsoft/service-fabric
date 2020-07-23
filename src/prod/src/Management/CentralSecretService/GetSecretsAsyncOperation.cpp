// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;

using namespace Management::ResourceManager;
using namespace Management::CentralSecretService;

StringLiteral const GetSecretsAsyncOperation::TraceComponent("CentralSecretServiceReplica::GetSecretsAsyncOperation");

GetSecretsAsyncOperation::GetSecretsAsyncOperation(
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

void GetSecretsAsyncOperation::Execute(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);
    GetSecretsDescription description;

    if (this->RequestMsg.GetBody(description))
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Getting secrets ...",
            this->TraceId,
            this->TraceComponent);
        
        auto operationSPtr = this->SecretManager.BeginGetSecrets(
            description.SecretReferences,
            description.IncludeValue,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operationSPtr)
            {
                this->CompleteGetSecretsOperation(operationSPtr, false);
            },
            thisSPtr,
            this->ActivityId);

        this->CompleteGetSecretsOperation(operationSPtr, true);
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

void GetSecretsAsyncOperation::CompleteGetSecretsOperation(
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
    }
    else
    {
        this->SetReply(make_unique<SecretsDescription>(readSecrets));
    }

    this->Complete(operationSPtr->Parent, error);
}