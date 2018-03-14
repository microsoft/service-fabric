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

AsyncOperationSPtr UpdateApplicationUpgradeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    UpdateApplicationUpgradeMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptUpdateApplicationUpgrade(
            body.UpdateDescription,
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

ErrorCode UpdateApplicationUpgradeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    auto error = this->Replica.EndAcceptUpdateApplicationUpgrade(operation);

    // This is expected since we are intentionally updating the upgrade context
    // while the upgrade is pending. Do not handle CMRequestAlreadyProcessing here
    // since that is an ambiguous state (application might be creating/deleting).
    //
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
