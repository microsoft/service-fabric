// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastManager : public Common::TextTraceComponent<Common::TraceTaskCodes::Broadcast>
    {
        DENY_COPY(BroadcastManager);

    public:
        explicit BroadcastManager(SiteNode & siteNode);

        ~BroadcastManager();

        void Broadcast(Transport::MessageUPtr && message);

        Common::AsyncOperationSPtr BeginBroadcast(Transport::MessageUPtr && message, bool toAllRings, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndBroadcast(Common::AsyncOperationSPtr const & operation);

        IMultipleReplyContextSPtr BroadcastRequest(Transport::MessageUPtr && message, Common::TimeSpan retryInterval);

        void Stop();

        void OnPToPOneWayMessage(__in Transport::MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext);

        void OnPToPRequestMessage(__in Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext);

        void OnRoutedBroadcastMessage(__in Transport::MessageUPtr & message, BroadcastHeader && header, RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessBroadcastAck(Transport::MessageId const & contextId, std::wstring const & ringName, NodeIdRange range, Common::ErrorCode error);

        void ProcessBroadcastLocalAck(Transport::MessageId const & broadcastId, Common::ErrorCode error);

    private:
        class ReliableOneWayBroadcastOperation;

        void InternalBroadcast(Transport::MessageUPtr && message, BroadcastHeader const & header);

        BroadcastHeader AddBroadcastHeaders(__in Transport::Message & message, bool expectsReply, bool expectsAck);

        void ProcessBroadcastMessage(__in Transport::MessageUPtr & message, PartnerNodeSPtr const & hopFrom);

        bool DispatchBroadcastMessage(__in Transport::MessageUPtr & message, BroadcastHeader const & broadcastHeader, RequestReceiverContextUPtr && routedRequestContext = nullptr);

        void ForwardAndDispatch(Transport::MessageUPtr & message, PartnerNodeSPtr const & hopFrom, BroadcastHeader const & header, RequestReceiverContextUPtr && requestContext);

        void BroadcastToSuccessorAndPredecessor(Transport::MessageUPtr & message, PartnerNodeSPtr const & hopFrom, int broadcastStepCount, BroadcastHeader const & header);

        void BroadcastMessageToRange(Transport::MessageUPtr & message, NodeIdRange const & range, BroadcastHeader const & header);

        bool BroadcastWithAck(Transport::MessageUPtr & message, bool toAllRings, NodeIdRange const & range, BroadcastHeader const & header, RequestReceiverContextUPtr && requestContext, Common::AsyncOperationSPtr const & operation);

        void RouteBroadcastMessageToHole(Transport::MessageUPtr && message, Transport::MessageId const & broadcastId, NodeIdRange nodeIdRange, Common::TimeSpan timeout);

        void DispatchReplyMessage(__in Transport::Message & reply, Transport::MessageId const & broadcastId, NodeIdRange const & range);

        void OnBroadcastCompleted(Transport::MessageId const & broadcastId);

        void OnTimer();

        SiteNode & siteNode_;
        Common::ExpiringSet<Transport::MessageId> broadcastMessagesAlreadySeen_;
        Common::ExpiringSet<Transport::MessageId, BroadcastForwardContext> reliableBroadcastContexts_;
        Common::SynchronizedMap<Transport::MessageId, BroadcastReplyContextSPtr> requestTable_;
        Common::TimerSPtr timer_;
        bool closed_;
        RWLOCK(Federation.BroadcastManager, lock_);

        friend class BroadcastReplyContext;
    };
}
