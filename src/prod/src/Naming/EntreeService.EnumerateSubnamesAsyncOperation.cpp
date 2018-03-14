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

    EntreeService::EnumerateSubnamesAsyncOperation::EnumerateSubnamesAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::EnumerateSubnamesAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (ReceivedMessage->GetBody(enumerateSubNamesRequest_))
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            Name = enumerateSubNamesRequest_.NamingUri;
            StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::EnumerateSubnamesAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
    }

    void EntreeService::EnumerateSubnamesAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr && storeReply)
    {
        EnumerateSubNamesResult result;
        if (storeReply->GetBody(result))
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetEnumerateSubNamesReply(result), ErrorCodeValue::Success);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
        }
    }

    MessageUPtr EntreeService::EnumerateSubnamesAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerEnumerateSubNames(enumerateSubNamesRequest_);
    }
}
