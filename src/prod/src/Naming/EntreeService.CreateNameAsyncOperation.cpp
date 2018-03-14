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

    EntreeService::CreateNameAsyncOperation::CreateNameAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::CreateNameAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
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

    void EntreeService::CreateNameAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
    }

    void EntreeService::CreateNameAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr &&)
    {
        this->SetReplyAndComplete(thisSPtr, NamingMessage::GetNameOperationReply(), ErrorCode::Success());
    }

    MessageUPtr EntreeService::CreateNameAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerNameCreate(Name);
    }
}
