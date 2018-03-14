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


AsyncOperationSPtr RollbackFabricUpgradeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return this->Replica.BeginAcceptRollbackFabricUpgrade(
        move(clientRequest),
        timeout,
        callback,
        root);
}

ErrorCode RollbackFabricUpgradeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    auto error = this->Replica.EndAcceptRollbackFabricUpgrade(operation);

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

