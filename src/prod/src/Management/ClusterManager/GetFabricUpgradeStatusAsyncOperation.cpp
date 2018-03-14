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


AsyncOperationSPtr GetFabricUpgradeStatusAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    UNREFERENCED_PARAMETER(clientRequest);
    UNREFERENCED_PARAMETER(timeout);

    auto thisSPtr = shared_from_this();

    FabricUpgradeStatusDescription statusDescription;
    ErrorCode error = this->Replica.GetFabricUpgradeStatus(
        dynamic_pointer_cast<ClientRequestAsyncOperation>(thisSPtr),
        statusDescription);

    if (error.IsSuccess())
    {
        this->SetReply(ClusterManagerMessage::GetClientOperationSuccess(statusDescription));
        this->TryComplete(thisSPtr, error);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        error,
        callback,
        root);
}

ErrorCode GetFabricUpgradeStatusAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);

}
