// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class VoteManager : public Common::TextTraceComponent<Common::TraceTaskCodes::VoteManager>
    {
        DENY_COPY(VoteManager);
        friend class VoteManagerStateMachineTests;
    public:
        VoteManager(SiteNode & siteNode);

        bool Initialize();
        void Start();
        void Stop();

        void UpdateGlobalTickets(__in Transport::Message & message, NodeInstance const & from);
        bool UpdateGlobalTickets(GlobalLease & globalLease, NodeInstance const & from);
        void GenerateGlobalLease(std::vector<VoteTicket> & tickets);
        GlobalLease GenerateGlobalLease(NodeInstance const & target);
        Common::TimeSpan GetDelta(NodeInstance const & target);

        int GetGlobalLeaseLevel() const;
        bool IsGlobalLeaseAbandoned() const;

        bool CheckGapRecovery(NodeIdRange const & range, std::vector<NodeId> & pendingVotes);

        void VotePingHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void VotePingReplyHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void VoteTransferRequestHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void VoteTransferReplyHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void VoteRenewRequestHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void VoteRenewReplyHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void DelayedArbitrateReplyHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void ArbitrateOneWayMessageHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void ArbitrateRequestHandler(__in Transport::Message & message, RequestReceiverContext & requestReceiverContext);
        void TicketGapRequestHandler(__in Transport::Message & message, RequestReceiverContext & requestReceiverContext);

        void OnRoutingTokenChanged();

        void Arbitrate(
            LeaseWrapper::LeaseAgentInstance const & local,
            LeaseWrapper::LeaseAgentInstance const & remote,
            Common::TimeSpan remoteTTL,
            Common::TimeSpan historyNeeded,
            int64 monitorLeaseInstance,
            int64 subjectLeaseInstance,
            int64 extraData,
            ArbitrationType::Enum type);

        void RemoveArbitrationOperation(ArbitrationOperationSPtr const & operation);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        int GetSeedNodes(std::vector<NodeId> & seedNodes) const;

        static bool IsSeedNode(NodeId nodeId, std::wstring const & ringName);

    private:
        enum GlobalLeaseState
        {
            GlobalLeaseValid,
            GlobalLeaseExpired,
            GlobalLeaseAbandoned,
        };

        struct DeltaTableEntry
        {
            DeltaTableEntry(int64 instance, Common::TimeSpan delta, Common::StopwatchTime now)
                : Instance(instance), Delta(delta), LastUpdated(now)
            {
            }

            uint64 Instance;
            Common::TimeSpan Delta;
            Common::StopwatchTime LastUpdated;
        };

        bool AddVote(VoteEntryUPtr && vote, bool isUpdate);
        void RemoveVote(NodeId voteId);
        VoteEntry* GetVote(NodeId voteId) const;

        int GetSeedNodesCallerHoldingLock(std::vector<NodeId> & seedNodes) const;

        void CompleteBootstrap(bool notifySeedNodes);

        bool OnMessageReceived(NodeInstance const & from);

        void OnGlobalLeaseLost();

        void ConvertSuperTickets();

        void PrepareTicketsForTransfer(VoteRequest const & request, __out std::vector<TicketTransfer> & transfers, __in NodeInstance const & destination);
        void AcceptTransferredTickets(std::vector<TicketTransfer> tickets, __in NodeInstance const & from);

        void UpdateGlobalLeaseExpirationCallerHoldingLock();
        void WriteGlobalLeaseExpirationToLeaseAgentCallerHoldingLock();

        void InternalGenerateGlobalLease(std::vector<VoteTicket> & tickets);
        Common::TimeSpan InternalGetDelta(NodeInstance const & target);
        bool UpdateDeltaTable(Common::StopwatchTime requestTime, Common::StopwatchTime now, NodeInstance const & from);

        bool LoadConfig(bool isUpdate);

        Common::TimeSpan CheckBootstrapState(StateMachineActionCollection & actions, NodePhase::Enum & phase);
        Common::TimeSpan CheckRoutingState(StateMachineActionCollection & actions);
        void RunStateMachine();

        SiteNode & siteNode_;
        std::vector<VoteEntryUPtr> entries_;
        std::vector<VoteEntryUPtr> staleEntries_;
        int count_;
        int quorumCount_;
        MUTABLE_RWLOCK(Federation.VoteManager, lock_);
        Common::StopwatchTime globalLeaseExpiration_;
        GlobalLeaseState globalLeaseState_;
        Common::TimerSPtr timer_;
        mutable ArbitrationTable arbitrationTable_;
        Common::StopwatchTime globalLeaseExpirationWrittenToLeaseAgent_;
        Common::HHandler configChangeHandler_;
        std::vector<ArbitrationOperationSPtr> arbitrationOperations_;
        Common::StopwatchTime lastArbitrationTime_;
        Common::StopwatchTime lastTwoWayArbitrationTime_;
        int continousArbitrations_;
        bool started_;
        std::map<NodeId, DeltaTableEntry> deltaTable_;
        Common::StopwatchTime lastDeltaTableCompact_;
    };
}
