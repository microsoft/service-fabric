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

    EntreeService::RecoverPartitionAsyncOperation::RecoverPartitionAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::RecoverPartitionAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::RecoverPartitionAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        RecoverPartitionMessageBody body;
        if (ReceivedMessage->GetBody(body))
        {
            AsyncOperationSPtr inner = Properties.AdminClient.BeginRecoverPartition(
                body.PartitionId,
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

    void EntreeService::RecoverPartitionAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr adminClientReply;
        ErrorCode error = Properties.AdminClient.EndRecoverPartition(asyncOperation);          
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(asyncOperation->Parent, NamingMessage::GetRecoverPartitionReply(), error);
        }
        else if (IsRetryable(error))
        {
            HandleRetryStart(asyncOperation->Parent);
        }
        else
        {
            TryComplete(asyncOperation->Parent, error);
        }
    }

    void EntreeService::RecoverPartitionAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
