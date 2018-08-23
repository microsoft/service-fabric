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

    EntreeService::UpdateServiceAsyncOperation::UpdateServiceAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , requestBody_()
      , systemServiceUpdate_(false)
    {
    }

    void EntreeService::UpdateServiceAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (ReceivedMessage->GetBody(requestBody_))
        {
            if (StringUtility::StartsWithCaseInsensitive<wstring>(
                    requestBody_.ServiceName.ToString(),
                    ServiceModel::SystemServiceApplicationNameHelper::SystemServiceApplicationName))
            {
                systemServiceUpdate_ = true;
            }
            else
            {
                TimedAsyncOperation::OnStart(thisSPtr);
                systemServiceUpdate_ = false;
                Name = requestBody_.ServiceName;
            }

            DoUpdate(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::UpdateServiceAsyncOperation::DoUpdate(AsyncOperationSPtr const & thisSPtr)
    {
        if (systemServiceUpdate_)
        {
            AsyncOperationSPtr inner = Properties.AdminClient.BeginUpdateService(
                    requestBody_.ServiceName.ToString(),
                    requestBody_.UpdateDescription,
                    ActivityHeader,
                    RemainingTime,
                    [this](AsyncOperationSPtr const& asyncOperation)
                    {
                    OnRequestCompleted(asyncOperation, false /*expectedCompletedSynchronously*/);
                    },
                    thisSPtr);
            OnRequestCompleted(inner, true /*expectedCompletedSynchronously*/);
        }
        else
        {
            StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
        }
    }

    void EntreeService::UpdateServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        DoUpdate(thisSPtr);
    }

    void EntreeService::UpdateServiceAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr &&)
    {
        Properties.PsdCache.TryRemove(Name.Name); // removing because the old entry is now stale
        this->SetReplyAndComplete(thisSPtr, NamingMessage::GetServiceOperationReply(), ErrorCodeValue::Success);
    }

    MessageUPtr EntreeService::UpdateServiceAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerUpdateService(requestBody_);
    }

    void EntreeService::UpdateServiceAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        Reliability::UpdateServiceReplyMessageBody replyBody;
        ErrorCode error = Properties.AdminClient.EndUpdateService(asyncOperation, replyBody);

        error = ErrorCode::FirstError(error, replyBody.Error);

        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(asyncOperation->Parent, NamingMessage::GetServiceOperationReply(), ErrorCodeValue::Success);
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
}
