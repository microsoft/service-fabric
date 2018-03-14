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

    EntreeService::EnumeratePropertiesAsyncOperation::EnumeratePropertiesAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::EnumeratePropertiesAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (ReceivedMessage->GetBody(enumeratePropertiesRequest_))
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            Name = enumeratePropertiesRequest_.NamingUri;
            StartStoreCommunication(thisSPtr, Name);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::EnumeratePropertiesAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name);
    }

    void EntreeService::EnumeratePropertiesAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr && storeReply)
    {
        EnumeratePropertiesResult result;
        if (storeReply->GetBody(result))
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetEnumeratePropertiesReply(result), ErrorCodeValue::Success);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
        }
    }

    MessageUPtr EntreeService::EnumeratePropertiesAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerEnumerateProperties(enumeratePropertiesRequest_);
    }
}
