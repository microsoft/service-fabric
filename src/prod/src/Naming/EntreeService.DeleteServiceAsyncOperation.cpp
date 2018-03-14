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

    EntreeService::DeleteServiceAsyncOperation::DeleteServiceAsyncOperation(
        __in GatewayProperties & properties,
        Transport::MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
      , messageBody_()
    {
    }

    void EntreeService::DeleteServiceAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (ReceivedMessage->GetBody(messageBody_))
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            Name = messageBody_.NamingUri;

            StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::DeleteServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
    }

    void EntreeService::DeleteServiceAsyncOperation::OnNameNotFound(Common::AsyncOperationSPtr const & thisSPtr)
    {
        TryComplete(thisSPtr, ErrorCodeValue::UserServiceNotFound);
    }

    void EntreeService::DeleteServiceAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr && reply)
    {
        MessageUPtr message;
        
        if (reply && reply->Action == NamingMessage::DeleteServiceReplyAction)
        {
            VersionedReplyBody body;
            if (reply->GetBody(body))
            {
                message = NamingMessage::GetDeleteServiceReply(body);
            }
        }

        if (!message)
        {
            // reply was from a V1 Naming service replica
            message = NamingMessage::GetServiceOperationReply();
        }

        this->SetReplyAndComplete(thisSPtr, move(message), ErrorCodeValue::Success);
    }

    MessageUPtr EntreeService::DeleteServiceAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerDeleteService(messageBody_);
    }

    void EntreeService::DeleteServiceAsyncOperation::OnCompleted()
    {
        // Delete the cached entry in all cases, even if the delete wasn't successful
        this->Properties.PsdCache.TryRemove(this->Name.Name);
        NamingRequestAsyncOperationBase::OnCompleted();
    }
}
