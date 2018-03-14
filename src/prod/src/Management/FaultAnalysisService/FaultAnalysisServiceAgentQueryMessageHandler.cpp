// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Query;
using namespace Reliability;
using namespace Naming;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("FaultAnalysisServiceAgentQueryMessageHandler");

FaultAnalysisServiceAgentQueryMessageHandler::FaultAnalysisServiceAgentQueryMessageHandler(
    __in FaultAnalysisServiceAgent & owner,
    __in QueryMessageHandlerUPtr & queryMessageHandler,
    MessageUPtr && request,
    IpcReceiverContextUPtr && requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ClientRequestAsyncOperation(
    owner,
    move(request),
    move(requestContext),
    timeout,
    callback,
    root)
    , queryMessageHandler_(queryMessageHandler)
{
}

AsyncOperationSPtr FaultAnalysisServiceAgentQueryMessageHandler::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timespan,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(clientRequest);
    return queryMessageHandler_->BeginProcessQueryMessage(Request, timespan, callback, parent);
}

ErrorCode FaultAnalysisServiceAgentQueryMessageHandler::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;

    ErrorCode error = queryMessageHandler_->EndProcessQueryMessage(operation, reply);
    if (error.IsSuccess())
    {
        SetReply(move(reply));
    }

    return error;
}

