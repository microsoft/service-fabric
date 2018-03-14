// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastManager;

    class BroadcastReplyContext : public IMultipleReplyContext, public std::enable_shared_from_this<BroadcastReplyContext>
    {
    public:
        BroadcastReplyContext(
            BroadcastManager & manager,
            Transport::MessageUPtr && original,
            Transport::MessageId broadcastId,
            Common::TimeSpan retryInterval);

        ~BroadcastReplyContext();

        Common::AsyncOperationSPtr BeginReceive(Common::TimeSpan timeout, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReceive(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & message, __out NodeInstance & replySender);

        void OnReply(Transport::MessageUPtr && message, NodeIdRange const & range);

        // Success or broadcast failed
        void Close();

        void Start();

        void Cancel();

    private:
        void OnTimerCallback(Common::TimerSPtr const & timer);

        BroadcastManager & manager_;
        Transport::MessageUPtr original_;
        Transport::MessageId broadcastId_;
        Common::TimeSpan retryInterval_;

        Common::ExclusiveLock lock_;
        NodeIdRangeTable rangeTable_;
        std::shared_ptr<Common::ReaderQueue<Transport::Message>> replies_;
        Common::atomic_long replyCount_;
        Common::atomic_long readCount_;
        Common::TimerSPtr retryTimer_;
        bool closed_;
    };

    typedef std::shared_ptr<BroadcastReplyContext> BroadcastReplyContextSPtr;
}
