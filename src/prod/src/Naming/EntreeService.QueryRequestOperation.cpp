// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Query;

     EntreeService::QueryRequestOperation::QueryRequestOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, move(receivedMessage), timeout, callback, parent)
      , queryGateway_(properties.QueryGateway)      
    {
    }

    void EntreeService::QueryRequestOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = queryGateway_.BeginProcessIncomingQuery(
            *ReceivedMessage,
            ActivityHeader,
            RemainingTime,
            [this](AsyncOperationSPtr const & operation) { FinishProcessIncomingQuery(operation, false); },
            thisSPtr);
        FinishProcessIncomingQuery(operation, true);
    }

    void EntreeService::QueryRequestOperation::FinishProcessIncomingQuery(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = queryGateway_.EndProcessIncomingQuery(operation, reply);
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(operation->Parent, move(reply), error);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }
    
    void EntreeService::QueryRequestOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        OnStart(thisSPtr);
    }
}
