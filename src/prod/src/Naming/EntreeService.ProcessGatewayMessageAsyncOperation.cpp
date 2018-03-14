// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Naming;
using namespace Common;
using namespace Transport;

StringLiteral const RequestDispatchingTraceComponent("RequestDispatching");

void EntreeService::ProcessGatewayMessageAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    auto actor = requestMessage_->Actor;
    {
        AcquireReadLock readLock(owner_.registeredHandlersMapLock_);
        if (owner_.registeredHandlersMap_.count(actor) == 0)
        {
            WriteWarning(RequestDispatchingTraceComponent, "Unknown message actor: '{0}'", actor);            
            TryComplete(thisSPtr, ErrorCodeValue::MessageHandlerDoesNotExistFault);
            return;
        }

        beginFunction_ = owner_.registeredHandlersMap_[actor].first;
        endFunction_ = owner_.registeredHandlersMap_[actor].second;
    }

    MessageUPtr msg = move(requestMessage_);
    beginFunction_(
        msg,
        timeout_,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnComplete(operation);
        },
        thisSPtr);
}

void EntreeService::ProcessGatewayMessageAsyncOperation::OnComplete(AsyncOperationSPtr const& operation)
{
    auto error = endFunction_(operation, replyMessage_);
    TryComplete(operation->Parent, error);
}

ErrorCode EntreeService::ProcessGatewayMessageAsyncOperation::End(AsyncOperationSPtr const& operation, MessageUPtr &message)
{
    auto thisPtr = AsyncOperation::End<ProcessGatewayMessageAsyncOperation>(operation);
    if (thisPtr->Error.IsSuccess())
    {
        message = move(thisPtr->replyMessage_);
    }

    return thisPtr->Error;
}
