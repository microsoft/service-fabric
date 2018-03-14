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

StringLiteral const TraceComponent("UpdateRepairTaskHealthPolicyAsyncOperation");

UpdateRepairTaskHealthPolicyAsyncOperation::UpdateRepairTaskHealthPolicyAsyncOperation(
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

AsyncOperationSPtr UpdateRepairTaskHealthPolicyAsyncOperation::BeginAcceptRequest(
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

    return this->Replica.BeginAcceptUpdateRepairTaskHealthPolicy(
        *body_.Scope,
        body_.TaskId,
        body_.Version,        
        body_.PerformPreparingHealthCheck,
        body_.PerformRestoringHealthCheck,
        move(clientRequest),
        timeout,
        callback,
        root);
}

ErrorCode UpdateRepairTaskHealthPolicyAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    int64 commitVersion = 0;
    ErrorCode error = this->Replica.EndAcceptUpdateRepairTaskHealthPolicy(operation, commitVersion);

    if (error.IsSuccess())
    {
        auto body = Common::make_unique<UpdateRepairTaskReplyMessageBody>(commitVersion);
        this->SetReply(RepairManagerTcpMessage::GetClientOperationSuccess(std::move(body))->GetTcpMessage());
    }

    return error;
}
