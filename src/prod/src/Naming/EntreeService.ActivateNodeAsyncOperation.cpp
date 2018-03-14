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

    EntreeService::ActivateNodeAsyncOperation::ActivateNodeAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::ActivateNodeAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::ActivateNodeAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        ActivateNodeMessageBody body;
        if (ReceivedMessage->GetBody(body))
        {
            NodeId nodeId;
            ErrorCode errorCode = Federation::NodeIdGenerator::GenerateFromString(body.NodeName, nodeId);
            if (!errorCode.IsSuccess())
            {
                this->TryComplete(thisSPtr, errorCode);
                return;
            }

            auto operation = Properties.AdminClient.BeginRemoveNodeDeactivations(
                nodeId.ToString(),
                ActivityHeader,
                RemainingTime,
                [this] (AsyncOperationSPtr const& operation) { this->OnRequestCompleted(operation, false); },
                thisSPtr);
            this->OnRequestCompleted(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::ActivateNodeAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = Properties.AdminClient.EndRemoveNodeDeactivations(operation);          
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetActivateNodeReply(), error);
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

    void EntreeService::ActivateNodeAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
