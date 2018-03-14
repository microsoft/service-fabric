// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace ClientServerTransport;

    FabricClientImpl::ForwardToServiceAsyncOperation::ForwardToServiceAsyncOperation(
        __in FabricClientImpl & client,
        __in_opt ClientServerRequestMessageUPtr && message,
        NamingUri const & name,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent,
        __in_opt ErrorCode && passThroughError)
        : ClientAsyncOperationBase(
            client,
            message ? FabricActivityHeader(message->ActivityId) : FabricActivityHeader(Guid::Empty()),
            move(passThroughError), 
            timeout, 
            callback, 
            parent)
        , message_(move(message))
        , action_(message_? message_->Action : wstring())
        , name_(name)
    {
    }

    void FabricClientImpl::ForwardToServiceAsyncOperation::OnStartOperation(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Client.BeginInternalForwardToService(
            move(message_),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) 
            {
                this->OnForwardToServiceComplete(operation, false /*expectedCompletedSynchronously*/); 
            },
            thisSPtr);
        this->OnForwardToServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void FabricClientImpl::ForwardToServiceAsyncOperation::OnForwardToServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = this->Client.EndInternalForwardToService(operation, reply_);

        TryComplete(operation->Parent, error);
    }

    ErrorCode FabricClientImpl::ForwardToServiceAsyncOperation::End(AsyncOperationSPtr const & operation, __out ClientServerReplyMessageUPtr & reply, __out Common::ActivityId & activityId)
    {
        return ForwardToServiceAsyncOperation::InternalEnd(operation, reply, activityId);
    }

    ErrorCode FabricClientImpl::ForwardToServiceAsyncOperation::End(AsyncOperationSPtr const & operation, __out ClientServerReplyMessageUPtr & reply)
    {
        Common::ActivityId activityId;
        return ForwardToServiceAsyncOperation::InternalEnd(operation, reply, activityId);
    }

    ErrorCode FabricClientImpl::ForwardToServiceAsyncOperation::InternalEnd(AsyncOperationSPtr const & operation, __out ClientServerReplyMessageUPtr & reply, __out Common::ActivityId & activityId)
    {
        auto casted = AsyncOperation::End<ForwardToServiceAsyncOperation>(operation);
        reply.swap(casted->reply_);
        activityId = casted->ActivityId;

        if (casted->Action == ClusterManagerTcpMessage::DeleteServiceAction)
        {
            casted->Client.Cache.ClearCacheEntriesWithName(casted->Name, casted->ActivityId);
        }

        return casted->Error;
    }
}
