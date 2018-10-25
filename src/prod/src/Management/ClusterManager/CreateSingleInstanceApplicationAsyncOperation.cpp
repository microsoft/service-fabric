//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Management::ClusterManager;
using namespace ServiceModel;

StringLiteral const TraceComponent("CreateSingleInstanceApplicationAsyncOperation");

AsyncOperationSPtr CreateSingleInstanceApplicationAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    CreateApplicationResourceMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.BeginAcceptCreateSingleInstanceApplication(
            body.TakeDescription(),
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

ErrorCode CreateSingleInstanceApplicationAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.EndAcceptCreateSingleInstanceApplication(operation);
}
