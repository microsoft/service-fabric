// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class ArbitrationOperation : public Common::TextTraceComponent<Common::TraceTaskCodes::Arbitration>
    {
        DENY_COPY(ArbitrationOperation);

    public:
        ArbitrationOperation(
            SiteNodeSPtr const & siteNode,
            std::vector<VoteProxySPtr> && votes,
            LeaseWrapper::LeaseAgentInstance const & local,
            LeaseWrapper::LeaseAgentInstance const & remote,
            Common::TimeSpan remoteTTL,
            Common::TimeSpan historyNeeded,
            int64 monitorLeaseInstance,
            int64 subjectLeaseInstance,
            int64 extraData,
            ArbitrationType::Enum type);

        void Start(std::shared_ptr<ArbitrationOperation> const & thisSPtr);

        bool Matches(DelayedArbitrationReplyBody const & body);
        void ProcessDelayedReply(ArbitrationOperationSPtr const & thisSPtr, DelayedArbitrationReplyBody const & body, NodeInstance const & voteInstance);

        static bool HasReply(ArbitrationType::Enum type);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        struct ArbitrationVote
        {
            DENY_COPY(ArbitrationVote);

        public:
            ArbitrationVote(VoteProxySPtr && proxy);

            bool HasResult() const;

            bool CanRevert(ArbitrationType::Enum type) const;

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            VoteProxySPtr proxy_;
            Common::StopwatchTime replyReceivedTime_;
            ArbitrationReplyBody result_;
            int count_;
            bool pending_;
            bool reverted_;
        };

        typedef std::unique_ptr<ArbitrationVote> ArbitrationVoteUPtr;

        static bool IsLocalPreferred(LeaseWrapper::LeaseAgentInstance const & local, LeaseWrapper::LeaseAgentInstance const & remote);

        void StartArbitration(ArbitrationOperationSPtr const & thisSPtr);
        void StartVoteArbitration(ArbitrationOperationSPtr const & thisSPtr, ArbitrationVote & vote, ArbitrationType::Enum type);

        ArbitrationVote* GetVoter(NodeInstance const & voteInstance);
        void ProcessResult(ArbitrationOperationSPtr const & thisSPtr, Common::AsyncOperationSPtr const & operation, ArbitrationVote & vote);
        void ProcessResult(ArbitrationOperationSPtr const & thisSPtr, ArbitrationVote & vote, Common::ErrorCode error, ArbitrationReplyBody const & result);
        void OnTimer(ArbitrationOperationSPtr const & thisSPtr);
        void RunStateMachine(ArbitrationOperationSPtr const & thisSPtr);
        void CheckState();
        void GenerationActions(ArbitrationOperationSPtr const & thisSPtr, bool partialCompleted, bool completed);
        void ReportResult(ArbitrationOperationSPtr const & thisSPtr, bool isDelayed);
        void ReportResultToLeaseAgent(bool isDelayed);
        bool CheckDueTime(Common::StopwatchTime dueTime, Common::StopwatchTime now, Common::StopwatchTime & nextCheck);
        void CleanupTimer();
        Common::TimeSpan GetRemoteTTL() const;

        SiteNodeSPtr siteNode_;
        LeaseWrapper::LeaseAgentInstance local_;
        LeaseWrapper::LeaseAgentInstance remote_;
        Common::StopwatchTime startTime_;
        Common::StopwatchTime remoteExpireTime_;
        Common::TimeSpan historyNeeded_;
        int64 monitorLeaseInstance_;
        int64 subjectLeaseInstance_;
        int64 extraData_;
        ArbitrationType::Enum type_;
        Common::StopwatchTime expireTime_;
        RWLOCK(Federation.ArbitrationOperation, lock_);;
        std::vector<ArbitrationVoteUPtr> votes_;
        Common::TimeSpan localResult_;
        Common::TimeSpan remoteResult_;
        ArbitrationType::Enum revertType_;
        Common::TimerSPtr timer_;
        Common::StopwatchTime nextCheck_;
        Common::StopwatchTime lastResponseTime_;
        Common::StopwatchTime checkRevertTime_;
        bool canRetry_;
        bool partialCompleted_;
        bool completed_;
        bool isLocalPreferred_;
    };
}
