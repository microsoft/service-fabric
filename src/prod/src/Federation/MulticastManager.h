// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastManager : public Common::TextTraceComponent<Common::TraceTaskCodes::Multicast>
    {
        DENY_COPY(MulticastManager);

    public:
        explicit MulticastManager(SiteNode & siteNode);

        ~MulticastManager();

        Common::AsyncOperationSPtr BeginMulticast(
            Transport::MessageUPtr && message,
            std::vector<NodeInstance> const & targets,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndMulticast(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<NodeInstance> & failedTargets,
            __out std::vector<NodeInstance> & pendingTargets);

        void Stop();

        void OnRoutedMulticastMessage(Transport::MessageUPtr & message, MulticastHeader && header, RoutingHeader const & routingHeader, RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessMulticastAck(
            Transport::MessageId const & contextId,
            NodeInstance const & target, 
            std::vector<NodeInstance> && failedTargets,
            std::vector<NodeInstance> && unknownTargets,
            Common::ErrorCode error,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout);

        void ProcessMulticastLocalAck(Transport::MessageId const & contextId, Common::ErrorCode error);

    private:
        static std::vector<MulticastForwardContext::SubTree> BuildSubtrees(std::vector<NodeInstance> const & targets);

        void ForwardMessage(
            Transport::MessageUPtr && message,
            Transport::MessageId const & multicastId, 
            std::vector<MulticastForwardContext::SubTree> const & subTrees,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout);

        MulticastHeader AddMulticastHeaders(Transport::Message & message) const;

        bool DispatchMulticastMessage(Transport::MessageUPtr & message, MulticastHeader const & header);

        bool MulticastWithAck(
            Transport::MessageUPtr & message,
            MulticastHeader const & header,
            std::vector<NodeInstance> && targets,
            bool includeLocal,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            RequestReceiverContextUPtr && requestContext,
            MulticastOperationSPtr const & operation);

        void OnTimer();

        SiteNode & siteNode_;
        Common::ExpiringSet<Transport::MessageId, MulticastForwardContext> multicastContexts_;
        Common::TimerSPtr timer_;
        bool closed_;
        RWLOCK(Federation.MulticastManager, lock_);
    };
}
