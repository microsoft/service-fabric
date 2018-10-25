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

StringLiteral const TraceComponent("GetSecretsAsyncOperation");

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

    WriteInfo(TraceComponent,
        "{0} GetSecrets: Begins.",
        this->TraceId);

    if (this->RequestMsg.GetBody(description))
    {
        WriteNoise(TraceComponent,
            "{0}: GetSecrets: Create StoreTransaction.",
            this->TraceId);
        vector<Secret> secrets;

        error = this->SecretManager.GetSecrets(description.SecretReferences, description.IncludeValue, this->ActivityId, secrets);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent,
                "{0}: GetSecrets: Failed to apply GetSecrets operations, ErrorCode: {1}.",
                this->TraceId,
                error);
        }
        else
        {
            this->SetReply(make_unique<SecretsDescription>(secrets));
        }
    }
    else
    {
        error = ErrorCode::FromNtStatus(this->RequestMsg.GetStatus());
        WriteWarning(TraceComponent,
            "{0}: GetSecrets: Failed to get the body of the request, ErrorCode: {1}.",
            this->TraceId,
            error);
    }

    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
        TraceComponent,
        "{0} GetSecrets: Ended with ErrorCode: {1}",
        this->TraceId,
        error);

    this->Complete(thisSPtr, error);
}
