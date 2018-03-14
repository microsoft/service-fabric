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

    EntreeService::DeactivateNodeAsyncOperation::DeactivateNodeAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::DeactivateNodeAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::DeactivateNodeAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        DeactivateNodeMessageBody body;
        if (ReceivedMessage->GetBody(body))
        {
            NodeId nodeId;
            auto errorCode = Federation::NodeIdGenerator::GenerateFromString(body.NodeName, nodeId);
            if (!errorCode.IsSuccess())
            {
                this->TryComplete(thisSPtr, errorCode);
                return;
            }

            map<NodeId, NodeDeactivationIntent::Enum> nodes;

            NodeDeactivationIntent::Enum deactivationIntent = NodeDeactivationIntent::None;
            ErrorCode error = NodeDeactivationIntent::FromPublic(body.DeactivationIntent, deactivationIntent);

            if (error.IsSuccess())
            {
                nodes.insert(make_pair(nodeId, deactivationIntent));

                auto operation = Properties.AdminClient.BeginDeactivateNodes(
                    move(nodes),
                    nodeId.ToString(),
                    ActivityHeader,
                    RemainingTime,
                    [this](AsyncOperationSPtr const& operation) { this->OnRequestCompleted(operation, false); },
                    thisSPtr);
                this->OnRequestCompleted(operation, true);
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::DeactivateNodeAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ErrorCode error = Properties.AdminClient.EndDeactivateNodes(operation);          
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetDeactivateNodeReply(), error);
        }
        else if (IsRetryable(error))
        {
            HandleRetryStart(thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::DeactivateNodeAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
