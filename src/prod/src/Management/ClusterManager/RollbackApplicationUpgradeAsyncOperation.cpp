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


AsyncOperationSPtr RollbackApplicationUpgradeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    RollbackApplicationUpgradeMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptRollbackApplicationUpgrade(
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

ErrorCode RollbackApplicationUpgradeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    auto error = this->Replica.EndAcceptRollbackApplicationUpgrade(operation);

    if (error.IsError(ErrorCodeValue::ApplicationUpgradeInProgress))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        this->TryComplete(operation->Parent, error);
    }

    return error;
}
