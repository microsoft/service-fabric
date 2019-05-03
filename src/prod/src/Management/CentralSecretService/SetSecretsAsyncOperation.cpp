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

StringLiteral const SetSecretsAsyncOperation::TraceComponent("CentralSecretServiceReplica::SetSecretsAsyncOperation");

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
    , secrets_()
{
}

void SetSecretsAsyncOperation::Execute(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);
    SecretsDescription description;

    if (this->RequestMsg.GetBody(description))
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Setting secrets ...",
            this->TraceId,
            this->TraceComponent);

        auto secretsOperationSPtr = this->SecretManager.BeginSetSecrets(
            description.Secrets,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operationSPtr)
            {
                this->CompleteSetSecretsOperation(operationSPtr, false);
            },
            thisSPtr,
            this->ActivityId);

        this->CompleteSetSecretsOperation(secretsOperationSPtr, true);
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

void SetSecretsAsyncOperation::CompleteSetSecretsOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = this->SecretManager.EndSetSecrects(operationSPtr, this->secrets_);
    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to set secrets. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);

        this->Complete(operationSPtr->Parent, error);
        return;
    }

    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Registering secret resources ...",
        this->TraceId,
        this->TraceComponent);

    vector<wstring> resourceNames;

    transform(
        this->secrets_.begin(),
        this->secrets_.end(),
        back_inserter(resourceNames),
        [](Secret const & secret)
        {
            return secret.ToResourceName();
        });

    auto resourcesOperationSPtr = this->ResourceManager.RegisterResources(
        resourceNames,
        this->RemainingTime,
        [this](AsyncOperationSPtr const & resourcesOperationSPtr)
        {
            this->CompleteRegisterResourcesOperation(resourcesOperationSPtr, false);
        },
        operationSPtr->Parent,
        this->ActivityId);

    this->CompleteRegisterResourcesOperation(resourcesOperationSPtr, true);
}

void SetSecretsAsyncOperation::CompleteRegisterResourcesOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End(operationSPtr);

    // The registration is invoked for any SetSecrets operation, which could correspond
    // to an update of an existing secret. In that case, RegisterResource (which calls
    // internally Insert) would fail with an internal error of 'duplicate key'. If that's
    // the case, eat the failure and return success.
    // todo [dragosav]: this is obviously a hack, made necessary by the shortness of time.
    // Post-Ignite, revisit the CSS API and either return a differentiated status from the
    // SetSecrets API, or handle conflicts inside RegisterResource with an SF-specific error
    // code (as opposed to the JET error being returned currently.)
    if (!error.IsSuccess()
        && error.IsError(ErrorCodeValue::StoreRecordAlreadyExists))
    {
        // expected for updates; reset and carry on
        error.Reset();
    }

    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to register resources. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
    }
    else
    {
        this->SetReply(make_unique<SecretsDescription>(this->secrets_));
    }

    this->Complete(operationSPtr->Parent, error);
}
