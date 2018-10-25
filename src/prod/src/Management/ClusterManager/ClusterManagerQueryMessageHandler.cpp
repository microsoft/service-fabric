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

using namespace Management::ClusterManager;

ClusterManagerQueryMessageHandler::ClusterManagerQueryMessageHandler(
    __in ClusterManagerReplica & owner,
    __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
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

AsyncOperationSPtr ClusterManagerQueryMessageHandler::BeginAcceptRequest(
    std::shared_ptr<ClientRequestAsyncOperation> && clientRequest,
    Common::TimeSpan const timespan, 
    Common::AsyncCallback const & callback, 
    Common::AsyncOperationSPtr const & parent)
{    
    UNREFERENCED_PARAMETER(clientRequest);

    return queryMessageHandler_->BeginProcessQueryMessage(Request, timespan, callback, parent);
}

ErrorCode ClusterManagerQueryMessageHandler::EndAcceptRequest(Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = queryMessageHandler_->EndProcessQueryMessage(operation, reply);
    if (error.IsSuccess())
    {
        SetReply(move(reply));        
    }
   
    CompleteRequest(error);
    return error;
}
