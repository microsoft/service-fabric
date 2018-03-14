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
    using namespace Reliability;
    
    StringLiteral const TraceComponent("ProcessRequest.PropertyBatch");

    EntreeService::PropertyBatchAsyncOperation::PropertyBatchAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : NamingRequestAsyncOperationBase(properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }

    void EntreeService::PropertyBatchAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        if (!ReceivedMessage->GetBody(batchRequest_))
        {
            TryComplete(shared_from_this(), ErrorCodeValue::InvalidMessage);
            return;
        }

        Name = batchRequest_.NamingUri;
        OnRetry(thisSPtr);
    }

    void EntreeService::PropertyBatchAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        StartStoreCommunication(thisSPtr, Name);
    }

    void EntreeService::PropertyBatchAsyncOperation::OnStoreCommunicationFinished(AsyncOperationSPtr const & thisSPtr, MessageUPtr && storeReply)
    {
        if (storeReply->GetBody(batchResult_))
        {
            this->SetReplyAndComplete(
                thisSPtr, 
                NamingMessage::GetPropertyBatchReply(NamePropertyOperationBatchResult(std::move(batchResult_))), 
                ErrorCodeValue::Success);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
        }
    }

    MessageUPtr EntreeService::PropertyBatchAsyncOperation::CreateMessageForStoreService()
    {
        // We need to make a copy of the operations in case we retry.
        auto operations = batchRequest_.Operations;

        return NamingMessage::GetPeerPropertyBatch(
            NamePropertyOperationBatch(Name, std::move(operations)));
    }
}
