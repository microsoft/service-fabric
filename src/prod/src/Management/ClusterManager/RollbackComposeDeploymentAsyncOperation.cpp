// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Reliability;
using namespace Naming;
using namespace std;

using namespace Management::ClusterManager;

AsyncOperationSPtr RollbackComposeDeploymentAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    RollbackComposeDeploymentMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptRollbackComposeDeployment(
            body.DeploymentName,
            move(clientRequest),
            timeout,
            callback,
            root);
    }
    else
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }
}

ErrorCode RollbackComposeDeploymentAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    auto error = this->Replica.EndAcceptRollbackComposeDeployment(operation);

    if (error.IsError(ErrorCodeValue::SingleInstanceApplicationUpgradeInProgress))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        this->TryComplete(operation->Parent, error);
    }
    return error;
}

