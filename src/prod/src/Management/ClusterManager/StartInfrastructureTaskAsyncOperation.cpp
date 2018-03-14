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


AsyncOperationSPtr StartInfrastructureTaskAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    InfrastructureTaskDescription body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptStartInfrastructureTask(
            body,
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

ErrorCode StartInfrastructureTaskAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    ErrorCode error = this->Replica.EndAcceptStartInfrastructureTask(operation);

    if (error.IsSuccess() || error.IsError(ErrorCodeValue::InfrastructureTaskInProgress))
    {
        this->CompleteRequest(ErrorCodeValue::Success);
    }

    return error;
}
