// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class UpdateManager
        : public std::enable_shared_from_this<UpdateManager>,
        public Common::TextTraceComponent<Common::TraceTaskCodes::SiteNode>
    {
        DENY_COPY(UpdateManager);

    public: 
        UpdateManager(SiteNode & siteNode);
        void Start();
        void Stop();
        void ProcessUpdateRequestMessage(_In_ Transport::Message & message, RequestReceiverContext & requestReceiverContext);

    private:
        struct TimedRange
        {
            TimedRange(NodeIdRange const & range, Common::StopwatchTime now) : Range(range), ReceiveTime(now)
            {
            }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const & option) const;

            NodeIdRange Range;
            Common::StopwatchTime ReceiveTime;
        };

        void TimerCallback();
        void ProcessUpdateReply(_In_ Transport::Message & message);

        void RouteCallback(Common::AsyncOperationSPtr const & contextSPtr, NodeId targetLocation);
        void RouteUpdateRequest(Transport::MessageUPtr && updateRequestMessage, NodeId targetLocation);

        void ProcessIncomingRange(NodeIdRange const & range);
        size_t FindGapEndGreaterThan(NodeId target);
        void AddGap(NodeIdRange const & gap);
        void AddRange(NodeIdRange const & range, bool newRange);
        bool TryGetNonExponentialTarget(NodeIdRange const & neighborhoodRange, _Out_ NodeId & updateTarget);

        SiteNode & siteNode_;

		RWLOCK(Federation.UpdateManager, thisLock_);
        Common::TimerSPtr timerSPtr_;
        bool stopped_;
        bool lastExponential_;
        std::vector<NodeId> targets_;
        size_t exponentialIndex_;
        std::vector<TimedRange> ranges_;
        std::vector<NodeIdRange> gaps_;
        Common::StopwatchTime lastSnapshot_;
        Common::TimeSpan snapshotInterval_;
    };
}
