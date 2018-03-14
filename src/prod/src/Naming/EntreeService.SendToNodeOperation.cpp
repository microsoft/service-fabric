// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace ServiceModel;
    using namespace std;

    StringLiteral const TraceComponent("ProcessRequest.SendToNodeOperation");

    EntreeService::SendToNodeOperation::SendToNodeOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        NodeId nodeId,
        uint64 nodeInstanceId)
        : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent),
        nodeId_(nodeId),
        nodeInstanceId_(nodeInstanceId)
    {
    }

    void EntreeService::SendToNodeOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartSend(thisSPtr);
    }

    void EntreeService::SendToNodeOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        StartSend(thisSPtr);
    }

    void EntreeService::SendToNodeOperation::StartSend(Common::AsyncOperationSPtr const & thisSPtr)
    {
        this->ReceivedMessage->Headers.Replace(TimeoutHeader(this->RemainingTime));

        this->RouteToNode(
            this->ReceivedMessage->Clone(),
            nodeId_,
            nodeInstanceId_,
            true, // exact routing
            thisSPtr);
    }

    void EntreeService::SendToNodeOperation::OnRouteToNodeSuccessful(
        Common::AsyncOperationSPtr const & thisSPtr,
        Transport::MessageUPtr & reply)
    {
        QueryResult queryResult;
        if (!reply->GetBody(queryResult))
        {
            WriteWarning(
                TraceComponent,
                "{0}: Could not get the body of the reply message",
                this->TraceId);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        if (!queryResult.QueryProcessingError.IsSuccess())
        {
            TryComplete(thisSPtr, queryResult.QueryProcessingError);
            return;
        }

        this->SetReplyAndComplete(thisSPtr, std::move(reply), ErrorCode::Success());
    }

    void EntreeService::SendToNodeOperation::OnRouteToNodeFailedNonRetryableError(
        Common::AsyncOperationSPtr const & thisSPtr,
        Common::ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidAddress);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    bool EntreeService::SendToNodeOperation::IsRetryable(ErrorCode const & error)
    {
        //
        // Queries addressed to components in specific nodes need exact routing(eg: RA, Hosting), fail when
        // node doesnt match exactly. This provides specific error code to user when the node name passed in
        // is not valid.
        //
        if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
        {
            return false;
        }
        else
        {
            return NamingErrorCategories::IsRetryableAtGateway(error);
        }
    }
}
