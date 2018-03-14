// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace Common;
    using namespace Transport;
    using namespace std;
    using namespace ClientServerTransport;

    FabricClientImpl::RequestReplyAsyncOperation::RequestReplyAsyncOperation(
        __in FabricClientImpl & client,
        __in NamingUri const& associatedName,
        __in_opt ClientServerRequestMessageUPtr && message,
        __in TimeSpan timeout,
        __in AsyncCallback const& callback,
        __in AsyncOperationSPtr const& parent,
        __in_opt ErrorCode && passThroughError)
        : ClientAsyncOperationBase(
            client, 
            message ? Transport::FabricActivityHeader(message->ActivityId) : FabricActivityHeader(Guid::Empty()),
            move(passThroughError), 
            timeout, 
            callback, 
            parent)
        , message_(move(message))
        , reply_()
        , associatedName_(associatedName)
    {
        this->Trace.RequestReplyCtor(
            this->TraceContext,
            this->ActivityId,
            timeout);
    }

    ErrorCode FabricClientImpl::RequestReplyAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out ClientServerReplyMessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<RequestReplyAsyncOperation>(asyncOperation); 

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

    ErrorCode FabricClientImpl::RequestReplyAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out ClientServerReplyMessageUPtr & reply,
        __out Common::ActivityId & activityId)
    {
        auto casted = AsyncOperation::End<RequestReplyAsyncOperation>(asyncOperation); 

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        activityId = casted->ActivityId;
        return casted->Error;
    }

    ErrorCode FabricClientImpl::RequestReplyAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out ClientServerReplyMessageUPtr & reply,
        __out NamingUri & associatedName,
        __out FabricActivityHeader & activityHeader)
    {
        auto casted = AsyncOperation::End<RequestReplyAsyncOperation>(asyncOperation); 

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        associatedName = move(casted->associatedName_);
        activityHeader = move(casted->ActivityHeader);
        return casted->Error;
    }

    void FabricClientImpl::RequestReplyAsyncOperation::OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->Client.IsOpened())
        {
            this->SendRequest(thisSPtr);
        }
        else
        {
            ErrorCode error(ErrorCodeValue::ObjectClosed);
            this->Trace.EndRequestFail(this->TraceContext, this->ActivityId, error);
            this->TryComplete(thisSPtr, error);
        }
    }

    void FabricClientImpl::RequestReplyAsyncOperation::SendRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->Client.role_ != RoleMask::None && this->Client.IsClientRoleAuthorized)
        {
            message_->Headers.Replace(ClientRoleHeader(this->Client.role_));
        }

        auto inner = this->Client.Connection->BeginSendToGateway(
            std::move(message_),
            this->RemainingTime,
            [this] (AsyncOperationSPtr const & operation) 
            { 
                this->OnSendRequestComplete(operation, false);
            },
            thisSPtr);
        this->OnSendRequestComplete(inner, true);
    }

    void FabricClientImpl::RequestReplyAsyncOperation::OnSendRequestComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ClientServerReplyMessageUPtr reply;
        auto error = this->Client.Connection->EndSendToGateway(operation, reply);

        if (error.IsSuccess())
        {
            reply_ = move(reply);
        }
        else
        {
            if (LruClientCacheManager::ErrorIndicatesInvalidService(error.ReadValue()))
            {
                this->Client.Cache.ClearCacheEntriesWithName(associatedName_, this->ActivityId);
            }

            if (error.IsError(ErrorCodeValue::NodeIsStopped))
            {   
                this->Client.Connection->ForceDisconnect();

                // If the current node is in zombie mode, then disconnect and let
                // the connection manager connect to a different node.
                // Return a generic retryable error to the application.
                //
                error = ErrorCodeValue::OperationCanceled;
            }
        }

        this->TryComplete(operation->Parent, error);
    }
}
