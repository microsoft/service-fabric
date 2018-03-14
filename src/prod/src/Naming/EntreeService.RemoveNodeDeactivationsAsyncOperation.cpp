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

    EntreeService::RemoveNodeDeactivationsAsyncOperation::RemoveNodeDeactivationsAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : AdminRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::RemoveNodeDeactivationsAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        this->StartRequest(thisSPtr);
    }

    void EntreeService::RemoveNodeDeactivationsAsyncOperation::StartRequest(AsyncOperationSPtr const& thisSPtr)
    {
        Reliability::RemoveNodeDeactivationRequestMessageBody body;
        if (this->ReceivedMessage->GetBody(body))
        {
            auto operation = this->Properties.AdminClient.BeginRemoveNodeDeactivations(
                body.BatchId,
                this->ActivityHeader,
                this->RemainingTime,
                [this] (AsyncOperationSPtr const& operation) { this->OnRequestCompleted(operation, false); },
                thisSPtr);
            this->OnRequestCompleted(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::RemoveNodeDeactivationsAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = this->Properties.AdminClient.EndRemoveNodeDeactivations(operation);          

        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetRemoveNodeDeactivationsReply(), error);
        }
        else if (this->IsRetryable(error))
        {
            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void EntreeService::RemoveNodeDeactivationsAsyncOperation::OnRetry(AsyncOperationSPtr const& thisSPtr)
    {
        this->StartRequest(thisSPtr);
    }
}
