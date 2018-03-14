// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;
using namespace Transport;
using namespace Api;

static const StringLiteral TraceType("ServiceCommunicationClient.SendRequestAsyncOperation");

ServiceCommunicationClient::SendRequestAsyncOperation::SendRequestAsyncOperation(
    __in ServiceCommunicationClient& owner,
    MessageUPtr&& message,
    TimeSpan const& timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    timeoutHelper_(timeout),
    request_(move(message))
{
}

ErrorCode ServiceCommunicationClient::SendRequestAsyncOperation::End(AsyncOperationSPtr const& operation, __out MessageUPtr& reply)
{
    auto casted = AsyncOperation::End<SendRequestAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        reply = move(casted->reply_);
    }

    return casted->Error;
}

void ServiceCommunicationClient::SendRequestAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
        //Explicit Connect required . Users needs to call BeginOpen Api to connect first before sending request
    if (this->owner_.explicitConnect_ && (this->owner_.GetState() == this->owner_.Opened))
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::ServiceCommunicationCannotConnect);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<TryConnectAsyncOperation>(
        this->owner_,
        this->owner_.connectTimeout_,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnConnected(operation, false);
    },
        thisSPtr);

       this->OnConnected(operation, true);
}


void ServiceCommunicationClient::SendRequestAsyncOperation::OnConnected(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }
    
    auto error = TryConnectAsyncOperation::End(operation);
    //if connection succceded , Send actual message , else return error
    if (error.IsSuccess())
    {
        this->SendRequest(operation->Parent);
    }
    else
    {
        this->TryComplete(operation->Parent, error);
    }

 }

void ServiceCommunicationClient::SendRequestAsyncOperation::OnSendRequestComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = this->owner_.requestReply_.EndRequest(operation, this->reply_);

    if (error.IsSuccess())
    {
        ServiceCommunicationErrorHeader errorHeader;
        if (reply_->Headers.TryReadFirst(errorHeader))
        {
            error = ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(errorHeader.ErrorCodeValue));
            if (!error.IsSuccess())
            {
                WriteWarning(TraceType, this->owner_.clientId_, "Error While Receiving Reply Message : {0}", error);
                if (ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(error))
                {
                    error = ErrorCodeValue::ServiceCommunicationCannotConnect;
                }
            }
        }
    }
    else
    {
        WriteWarning(TraceType, this->owner_.clientId_, "Error While Sending Message : {0}", error);

        if (ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(error))
        {
            error = ErrorCodeValue::ServiceCommunicationCannotConnect;
        }
    }

    this->TryComplete(operation->Parent, error);
}

void ServiceCommunicationClient::SendRequestAsyncOperation::SendRequest(AsyncOperationSPtr const& thisSPtr)
{
    auto operation = this->owner_.requestReply_.BeginRequest(move(this->request_),
        this->owner_.serverSendTarget_,
        TransportPriority::Normal,
        this->timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnSendRequestComplete(operation, false);
    },
        thisSPtr);
    this->OnSendRequestComplete(operation, true);
}

