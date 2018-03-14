// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::RequestReplyAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
    public:
        RequestReplyAsyncOperation(
            __in FabricClientImpl & client,
            __in Common::NamingUri const& associatedName,
            __in ClientServerRequestMessageUPtr && message,
            __in Common::TimeSpan timeout,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent,
            __in_opt Common::ErrorCode && passThroughError = Common::ErrorCode::Success());

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ClientServerReplyMessageUPtr & reply);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ClientServerReplyMessageUPtr & reply,
            __out Common::ActivityId & activityId);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ClientServerReplyMessageUPtr & reply,
            __out Common::NamingUri & associatedName,
            __out Transport::FabricActivityHeader & activityHeader);

    protected:
        virtual void OnStartOperation(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void SendRequest(
            Common::AsyncOperationSPtr const & thisSPtr);

        void OnSendRequestComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        ClientServerRequestMessageUPtr message_;
        ClientServerReplyMessageUPtr reply_;
        Common::NamingUri associatedName_;
    };
}
