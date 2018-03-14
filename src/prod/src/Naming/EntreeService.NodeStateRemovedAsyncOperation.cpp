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

    EntreeService::NodeStateRemovedAsyncOperation::NodeStateRemovedAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::NodeStateRemovedAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::NodeStateRemovedAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        NodeStateRemovedMessageBody body;
        if (ReceivedMessage->GetBody(body))
        {
            NodeId nodeId;
            if (body.NodeName.empty())
            {
                nodeId = body.NodeId;
            }
            else
            {
                ErrorCode errorCode = Federation::NodeIdGenerator::GenerateFromString(body.NodeName, nodeId);
                if (!errorCode.IsSuccess())
                {
                    this->TryComplete(thisSPtr, errorCode);
                    return;
                }
            }

            AsyncOperationSPtr inner = Properties.AdminClient.BeginNodeStateRemoved(
                nodeId,
                ActivityHeader,
                RemainingTime,
                [this] (AsyncOperationSPtr const& asyncOperation) 
                {
                    OnRequestCompleted(asyncOperation, false /*expectedCompletedSynchronously*/);
                },
                thisSPtr);
            OnRequestCompleted(inner, true /*expectedCompletedSynchronously*/);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::NodeStateRemovedAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr adminClientReply;
        ErrorCode error = Properties.AdminClient.EndNodeStateRemoved(asyncOperation);          
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(asyncOperation->Parent, NamingMessage::GetNodeStateRemovedReply(), error);
        }
        else if (this->IsRetryable(error))
        {
            HandleRetryStart(asyncOperation->Parent);
        }
        else
        {
            TryComplete(asyncOperation->Parent, error);
        }
    }

    void EntreeService::NodeStateRemovedAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }

    bool EntreeService::NodeStateRemovedAsyncOperation::IsRetryable(ErrorCode const& error)
    {
        switch (error.ReadValue())
        {
            case ErrorCodeValue::InvalidState:
            case ErrorCodeValue::ReconfigurationPending:
                return true;

            default:
                return NamingErrorCategories::IsRetryableAtGateway(error);
        }
    }
}
