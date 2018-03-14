// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class RequestResponseAsyncOperationBase
            : public Common::TimedAsyncOperation
            , public Common::TextTraceComponent<Common::TraceTaskCodes::BA>
        {
        public:
            RequestResponseAsyncOperationBase(
                Transport::MessageUPtr && message,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ~RequestResponseAsyncOperationBase();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply);
                
            __declspec(property(get = get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
            Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

            __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
            Common::ActivityId const & get_ActivityId() const { return activityId_; }

            __declspec(property(get = get_Reply, put = set_Reply)) Transport::MessageUPtr & Reply;
            Transport::MessageUPtr const & get_Reply() const { return reply_; }
            void set_Reply(Transport::MessageUPtr && value) { reply_ = std::move(value); }

            void SetReplyAndComplete(
                Common::AsyncOperationSPtr const & thisSPtr,
                Transport::MessageUPtr && reply,
                Common::ErrorCode const & error);

            Transport::MessageUPtr reply_;      // TODO: This should be private

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCompleted() override;

            Transport::MessageUPtr receivedMessage_;
        private:
            Common::ActivityId activityId_;
        };
    }
}
