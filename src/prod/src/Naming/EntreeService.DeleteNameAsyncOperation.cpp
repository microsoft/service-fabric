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

    EntreeService::DeleteNameAsyncOperation::DeleteNameAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::DeleteNameAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->TryGetNameFromRequestBody())
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::DeleteNameAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
    }

    void EntreeService::DeleteNameAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr &&)
    {
        this->SetReplyAndComplete(thisSPtr, NamingMessage::GetNameOperationReply(), ErrorCode::Success());
    }

    MessageUPtr EntreeService::DeleteNameAsyncOperation::CreateMessageForStoreService()
    {
        auto request = NamingMessage::GetPeerNameDelete(Name);

        DeleteNameHeader header;
        if (this->ReceivedMessage->Headers.TryReadFirst(header))
        {
            request->Headers.Replace(header);
        }

        return move(request);
    }
}
