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

AsyncOperationSPtr UpdateFabricUpgradeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    UpdateFabricUpgradeMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptUpdateFabricUpgrade(
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

ErrorCode UpdateFabricUpgradeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    auto error = this->Replica.EndAcceptUpdateFabricUpgrade(operation);

    if (error.IsError(ErrorCodeValue::FabricUpgradeInProgress))
    {
        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        this->TryComplete(operation->Parent, error);
    }

    return error;
}
