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
    using namespace Query;

    EntreeService::FMQueryAsyncOperation::FMQueryAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        bool isTargetFMM,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , isTargetFMM_(isTargetFMM)
    {
    }

    void EntreeService::FMQueryAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        StartRequest(thisSPtr);
    }

    void EntreeService::FMQueryAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        QueryRequestMessageBodyInternal queryRequestMessageBody;

        if (ReceivedMessage->GetBody(queryRequestMessageBody))
        {
            QueryAddressHeader addressHeader;
            ReceivedMessage->Headers.TryReadFirst(addressHeader);

            AsyncOperationSPtr inner = Properties.AdminClient.BeginQuery(
                ActivityHeader,
                addressHeader,
                queryRequestMessageBody,
                isTargetFMM_,
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

    void EntreeService::FMQueryAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr replyMessage;
        ErrorCode error = Properties.AdminClient.EndQuery(asyncOperation, replyMessage);
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(asyncOperation->Parent, move(replyMessage), error);
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

    void EntreeService::FMQueryAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        StartRequest(thisSPtr);
    }
}
