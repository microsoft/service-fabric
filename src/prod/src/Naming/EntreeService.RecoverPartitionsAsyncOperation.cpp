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

    EntreeService::RecoverPartitionsAsyncOperation::RecoverPartitionsAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::RecoverPartitionsAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        StartRequest(thisSPtr);
    }

    void EntreeService::RecoverPartitionsAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        AsyncOperationSPtr inner = Properties.AdminClient.BeginRecoverPartitions(
            ActivityHeader,
            RemainingTime,
            [this] (AsyncOperationSPtr const& asyncOperation) 
            {
                OnRequestCompleted(asyncOperation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnRequestCompleted(inner, true /*expectedCompletedSynchronously*/);
    }

    void EntreeService::RecoverPartitionsAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr adminClientReply;
        ErrorCode error = Properties.AdminClient.EndRecoverPartitions(asyncOperation);          
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(asyncOperation->Parent, NamingMessage::GetRecoverPartitionsReply(), error);
        }
        else
        {
            HandleRetryStart(asyncOperation->Parent);
        }
    }

    void EntreeService::RecoverPartitionsAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
