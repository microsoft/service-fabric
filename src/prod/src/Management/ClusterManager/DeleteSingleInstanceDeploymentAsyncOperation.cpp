//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;

AsyncOperationSPtr DeleteSingleInstanceDeploymentAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    DeleteSingleInstanceDeploymentMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptDeleteSingleInstanceDeployment(
            body.Description,
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

ErrorCode DeleteSingleInstanceDeploymentAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.EndAcceptDeleteSingleInstanceDeployment(operation);
}
