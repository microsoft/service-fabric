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
using namespace ClientServerTransport;

using namespace Management::RepairManager;

StringLiteral const TraceComponent("DeleteRepairRequestAsyncOperation");

DeleteRepairRequestAsyncOperation::DeleteRepairRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ClientRequestAsyncOperation(
        owner,
        move(request),
        move(receiverContext),
        timeout,
        callback,
        root)
    , body_()
{
}

DeleteRepairRequestAsyncOperation::DeleteRepairRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    DeleteRepairTaskMessageBody && requestBody,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ClientRequestAsyncOperation(
        owner,
        activityId,
        timeout,
        callback,
        root)
    , body_(requestBody)
{
}

AsyncOperationSPtr DeleteRepairRequestAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (this->HasRequest && !this->Request.GetBody(body_))
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }

    if (!body_.Scope)
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }

    return this->Replica.BeginAcceptDeleteRepairRequest(
        *body_.Scope,
        body_.TaskId,
        body_.Version,
        move(clientRequest),
        timeout,
        callback,
        root);
}

ErrorCode DeleteRepairRequestAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    // Base class will set the simple success reply
    return this->Replica.EndAcceptDeleteRepairRequest(operation);
}
