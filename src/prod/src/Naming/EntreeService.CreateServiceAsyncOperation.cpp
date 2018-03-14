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

    EntreeService::CreateServiceAsyncOperation::CreateServiceAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::CreateServiceAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (ReceivedMessage->GetBody(createServiceRequest_))
        {
            TimedAsyncOperation::OnStart(thisSPtr);
            Name = createServiceRequest_.NamingUri;
            StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    void EntreeService::CreateServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name.GetAuthorityName());
    }

    void EntreeService::CreateServiceAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr &&)
    {
        this->Properties.PsdCache.TryRemove(this->Name.Name); // removing because the old entry is now stale
        this->SetReplyAndComplete(thisSPtr, NamingMessage::GetServiceOperationReply(), ErrorCode::Success());
    }

    MessageUPtr EntreeService::CreateServiceAsyncOperation::CreateMessageForStoreService()
    {
        return NamingMessage::GetPeerCreateService(createServiceRequest_);
    }
}
