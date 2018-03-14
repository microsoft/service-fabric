// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeServiceProxy::RequestAsyncOperationBase 
        : public Common::TimedAsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::EntreeServiceProxy>
    {
    public:
        RequestAsyncOperationBase(
            __in EntreeServiceProxy &owner,
            Transport::MessageUPtr && receivedMessage,
            Transport::ISendTarget::SPtr const &to,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        ~RequestAsyncOperationBase() {};

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Transport::MessageUPtr & reply);

    protected:
        __declspec(property(get = get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
        Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

        __declspec(property(get = get_Owner)) EntreeServiceProxy & Owner;
        EntreeServiceProxy const & get_Owner() const { return owner_; }

        void SetReplyAndComplete(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr && reply,
            Common::ErrorCode const & error);

        virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

    private:
        void OnRequestComplete(Common::AsyncOperationSPtr const &operation, bool);
        void OnNotificationComplete(Common::AsyncOperationSPtr const &operation, bool);
        EntreeServiceProxy &owner_;
        Transport::MessageUPtr receivedMessage_;
        Transport::ISendTarget::SPtr to_;
        Transport::MessageUPtr reply_;
    };
}
