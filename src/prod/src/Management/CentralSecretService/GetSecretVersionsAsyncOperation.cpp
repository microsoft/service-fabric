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

StringLiteral const GetSecretVersionsAsyncOperation::TraceComponent("CentralSecretServiceReplica::GetSecretVersionsAsyncOperation");

GetSecretVersionsAsyncOperation::GetSecretVersionsAsyncOperation(
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

void GetSecretVersionsAsyncOperation::Execute(AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error(ErrorCodeValue::Success);

    if (this->RequestMsg.GetBody(this->description_))
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Getting existing secret versions...",
            this->TraceId,
            this->TraceComponent);

        auto listingOperationSPtr = this->SecretManager.BeginGetSecretVersions(
            this->description_.SecretReferences,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->CompleteGetSecretVersionsOperation(operationSPtr, false);
        },
            thisSPtr,
            this->ActivityId);

        this->CompleteGetSecretVersionsOperation(listingOperationSPtr, true);
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

void GetSecretVersionsAsyncOperation::CompleteGetSecretVersionsOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    vector<SecretReference> retrievedSecretRefs;

    auto error = this->SecretManager.EndGetSecretVersions(operationSPtr, retrievedSecretRefs);
    if (!error.IsSuccess())
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Failed to retrieve secret versions. Error: {2}.",
            this->TraceId,
            this->TraceComponent,
            error);
    }
    else
    {
        this->SetReply(make_unique<SecretReferencesDescription>(retrievedSecretRefs));
    }

    this->Complete(operationSPtr->Parent, error);
}
