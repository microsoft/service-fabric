// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;

    EntreeService::ReportFaultAsyncOperation::ReportFaultAsyncOperation(
        __in GatewayProperties & properties,
        Transport::MessageUPtr && receivedMessage,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase(properties, move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ReportFaultAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::ReportFaultAsyncOperation::StartRequest(Common::AsyncOperationSPtr const & thisSPtr)
    {
        ReportFaultRequestMessageBody body;

        if (!ReceivedMessage->GetBody(body))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        NodeId nodeId;
        auto error = NodeIdGenerator::GenerateFromString(body.NodeName, nodeId);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        auto message = RSMessage::GetReportFaultRequest().CreateMessage(ActivityHeader, body);
        RouteToNode(move(message), nodeId, 0, false, thisSPtr);
    }
}
