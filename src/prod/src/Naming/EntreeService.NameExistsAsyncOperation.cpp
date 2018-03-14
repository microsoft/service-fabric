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

    EntreeService::NameExistsAsyncOperation::NameExistsAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::NameExistsAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->TryGetNameFromRequestBody())
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            StartStoreCommunication(thisSPtr, Name);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::NameExistsAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name);
    }

    void EntreeService::NameExistsAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr && storeReply)
    {
        NameExistsReplyMessageBody body;
        if (storeReply->GetBody(body))
        {
            this->SetReplyAndComplete(thisSPtr, NamingMessage::GetNameExistsReply(body), ErrorCodeValue::Success);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
        }
    }

    MessageUPtr EntreeService::NameExistsAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerNameExists(Name);
    }
}
