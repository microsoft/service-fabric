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

AsyncOperationSPtr DeleteComposeDeploymentAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    DeleteComposeDeploymentMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptDeleteComposeDeployment(
            body.DeploymentName,
            body.ApplicationName,
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

ErrorCode DeleteComposeDeploymentAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.EndAcceptDeleteComposeDeployment(operation);
}

