// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class FederationMessage
    {
    public:
        FederationMessage(std::wstring const & action, Transport::Actor::Enum actor = Actor)
            : actionHeader_(action), actor_(actor)
        {
        }

        static FederationMessage const & GetLockRequest() { return LockRequest; }
        static FederationMessage const & GetLockGrant() { return LockGrant; }
        static FederationMessage const & GetLockDeny() { return LockDeny; }
        static FederationMessage const & GetLockTransferRequest() { return LockTransferRequest; }
        static FederationMessage const & GetLockTransferReply() { return LockTransferReply; }
        static FederationMessage const & GetLockRenew() { return LockRenew; }
        static FederationMessage const & GetLockRelease() { return LockRelease; }
        static FederationMessage const & GetUnlockRequest() { return UnlockRequest; }
        static FederationMessage const & GetUnlockGrant() { return UnlockGrant; }
        static FederationMessage const & GetUnlockDeny() { return UnlockDeny; }
        static FederationMessage const & GetDepart() { return Depart; };
        static FederationMessage const & GetNodeDoesNotMatchFault() { return NodeDoesNotMatchFault; };
        static FederationMessage const & GetRejectFault() { return RejectFault; };
        static FederationMessage const & GetTokenRequest() { return TokenRequest; };
        static FederationMessage const & GetTokenTransfer() { return TokenTransfer; };
        static FederationMessage const & GetTokenTransferAccept() { return TokenTransferAccept; };
        static FederationMessage const & GetTokenTransferReject() { return TokenTransferReject; };
        static FederationMessage const & GetTokenProbe() { return TokenProbe; };
        static FederationMessage const & GetTokenEcho() { return TokenEcho; };
        static FederationMessage const & GetEdgeProbe() { return EdgeProbe; };
        static FederationMessage const & GetRingAdjust() { return RingAdjust; };
        static FederationMessage const & GetPing() { return Ping; }
        static FederationMessage const & GetUpdateRequest() { return UpdateRequest; }
        static FederationMessage const & GetUpdateReply() { return UpdateReply; }

        static FederationMessage const & GetRoutingAck() { return RoutingAck; }

        static FederationMessage const & GetVotePing() { return VotePing; }
        static FederationMessage const & GetVotePingReply() { return VotePingReply; }
        static FederationMessage const & GetVoteTransferRequest() { return VoteTransferRequest; }
        static FederationMessage const & GetVoteTransferReply() { return VoteTransferReply; }
		static FederationMessage const & GetVoteRenewRequest() { return VoteRenewRequest; }
		static FederationMessage const & GetVoteRenewReply() { return VoteRenewReply; }

        static FederationMessage const & GetArbitrateRequest() { return ArbitrateRequest; }
        static FederationMessage const & GetExtendedArbitrateRequest() { return ExtendedArbitrateRequest; }
        static FederationMessage const & GetArbitrateReply() { return ArbitrateReply; }
        static FederationMessage const & GetDelayedArbitrateReply() { return DelayedArbitrateReply; }
        static FederationMessage const & GetArbitrateKeepAlive() { return ArbitrateKeepAlive; }

        static FederationMessage const & GetTicketGapRequest() { return TicketGapRequest; }
        static FederationMessage const & GetTicketGapReply() { return TicketGapReply; }
        static FederationMessage const & GetNeighborhoodQueryRequest() {return NeighborhoodQueryRequest; }
        static FederationMessage const & GetNeighborhoodQueryReply() {return NeighborhoodQueryReply; }
        static FederationMessage const & GetResumeJoin() { return ResumeJoin; }

        static FederationMessage const & GetLivenessQueryRequest() {return LivenessQueryRequest; }
        static FederationMessage const & GetLivenessQueryReply() {return LivenessQueryReply; }

        static FederationMessage const & GetBroadcastAck() {return BroadcastAck; }
        static FederationMessage const & GetMulticastAck() {return MulticastAck; }

        static FederationMessage const & GetLivenessUpdate() {return LivenessUpdate; }

        static FederationMessage const & GetExternalRingPing() {return ExternalRingPing; }
        static FederationMessage const & GetExternalRingPingResponse() {return ExternalRingPingResponse; }

        static FederationMessage const & GetVoterStoreConfigQueryRequest() { return VoterStoreConfigQueryRequest; }
        static FederationMessage const & GetVoterStoreConfigQueryReply() { return VoterStoreConfigQueryReply; }
        static FederationMessage const & GetVoterStoreIntroduceRequest() { return VoterStoreIntroduceRequest; }
        static FederationMessage const & GetVoterStoreIntroduceReply() { return VoterStoreIntroduceReply; }
        static FederationMessage const & GetVoterStoreBootstrapRequest() { return VoterStoreBootstrapRequest; }
        static FederationMessage const & GetVoterStoreBootstrapReply() { return VoterStoreBootstrapReply; }
        static FederationMessage const & GetVoterStoreLeaderProbeRequest() { return VoterStoreLeaderProbeRequest; }
        static FederationMessage const & GetVoterStoreLeaderProbeReply() { return VoterStoreLeaderProbeReply; }
        static FederationMessage const & GetVoterStoreJoin() { return VoterStoreJoin; }
        static FederationMessage const & GetVoterStoreSyncRequest() { return VoterStoreSyncRequest; }
        static FederationMessage const & GetVoterStoreSyncReply() { return VoterStoreSyncReply; }
        static FederationMessage const & GetVoterStoreProgressRequest() { return VoterStoreProgressRequest; }
        static FederationMessage const & GetVoterStoreProgressReply() { return VoterStoreProgressReply; }
        static FederationMessage const & GetVoterStoreReadRequest() { return VoterStoreReadRequest; }
        static FederationMessage const & GetVoterStoreReadReply() { return VoterStoreReadReply; }
        static FederationMessage const & GetVoterStoreWriteRequest() { return VoterStoreWriteRequest; }
        static FederationMessage const & GetVoterStoreWriteReply() { return VoterStoreWriteReply; }

        static FederationMessage const & GetEmptyRequest() { return EmptyRequest; }
        static FederationMessage const & GetEmptyReply() { return EmptyReply; }

        static const Transport::Actor::Enum Actor;

        static bool IsFederationMessage(Transport::Message const & msg);

        Transport::MessageUPtr CreateMessage() const;

        template <class T>
        Transport::MessageUPtr CreateMessage(T const& t, bool isBodyComplete = true) const
        {
            Transport::MessageUPtr message = Common::make_unique<Transport::Message>(t, isBodyComplete);
            message->Headers.Add(Transport::ActorHeader(actor_));
            message->Headers.Add(actionHeader_);
            message->Headers.Add(Transport::HighPriorityHeader(true));
            return std::move(message);
        }
        
        __declspec (property(get=getAction)) std::wstring const & Action;
        std::wstring const & getAction() const { return actionHeader_.Action; } ;

    private:
        static Common::Global<FederationMessage> LockRequest;
        static Common::Global<FederationMessage> LockGrant;
        static Common::Global<FederationMessage> LockDeny;
        static Common::Global<FederationMessage> LockTransferRequest;
        static Common::Global<FederationMessage> LockTransferReply;
        static Common::Global<FederationMessage> LockRenew;
        static Common::Global<FederationMessage> LockRelease;
        static Common::Global<FederationMessage> UnlockRequest;
        static Common::Global<FederationMessage> UnlockGrant;
        static Common::Global<FederationMessage> UnlockDeny;
        static Common::Global<FederationMessage> Depart;
        static Common::Global<FederationMessage> NodeDoesNotMatchFault;
        static Common::Global<FederationMessage> RejectFault;
        static Common::Global<FederationMessage> TokenRequest;
        static Common::Global<FederationMessage> TokenTransfer;
        static Common::Global<FederationMessage> TokenTransferAccept;
        static Common::Global<FederationMessage> TokenTransferReject;
        static Common::Global<FederationMessage> TokenProbe;
        static Common::Global<FederationMessage> TokenEcho;
        static Common::Global<FederationMessage> EdgeProbe;
        static Common::Global<FederationMessage> RingAdjust;
        static Common::Global<FederationMessage> Ping;
        static Common::Global<FederationMessage> RoutingAck;
        static Common::Global<FederationMessage> UpdateRequest;
        static Common::Global<FederationMessage> UpdateReply;
        static Common::Global<FederationMessage> ArbitrateRequest;
        static Common::Global<FederationMessage> ExtendedArbitrateRequest;
        static Common::Global<FederationMessage> ArbitrateReply;
        static Common::Global<FederationMessage> DelayedArbitrateReply;
        static Common::Global<FederationMessage> ArbitrateKeepAlive;
        static Common::Global<FederationMessage> VotePing;
        static Common::Global<FederationMessage> VotePingReply;
        static Common::Global<FederationMessage> VoteTransferRequest;
        static Common::Global<FederationMessage> VoteTransferReply;
		static Common::Global<FederationMessage> VoteRenewRequest;
		static Common::Global<FederationMessage> VoteRenewReply;
        static Common::Global<FederationMessage> TicketGapRequest;
        static Common::Global<FederationMessage> TicketGapReply;
        static Common::Global<FederationMessage> NeighborhoodQueryRequest;
        static Common::Global<FederationMessage> NeighborhoodQueryReply;
        static Common::Global<FederationMessage> ResumeJoin;
        static Common::Global<FederationMessage> LivenessQueryRequest;
        static Common::Global<FederationMessage> LivenessQueryReply;
        static Common::Global<FederationMessage> BroadcastAck;
        static Common::Global<FederationMessage> MulticastAck;
        static Common::Global<FederationMessage> LivenessUpdate;
        static Common::Global<FederationMessage> ExternalRingPing;
        static Common::Global<FederationMessage> ExternalRingPingResponse;
        static Common::Global<FederationMessage> VoterStoreConfigQueryRequest;
        static Common::Global<FederationMessage> VoterStoreConfigQueryReply;
        static Common::Global<FederationMessage> VoterStoreIntroduceRequest;
        static Common::Global<FederationMessage> VoterStoreIntroduceReply;
        static Common::Global<FederationMessage> VoterStoreBootstrapRequest;
        static Common::Global<FederationMessage> VoterStoreBootstrapReply;
        static Common::Global<FederationMessage> VoterStoreLeaderProbeRequest;
        static Common::Global<FederationMessage> VoterStoreLeaderProbeReply;
        static Common::Global<FederationMessage> VoterStoreJoin;
        static Common::Global<FederationMessage> VoterStoreSyncRequest;
        static Common::Global<FederationMessage> VoterStoreSyncReply;
        static Common::Global<FederationMessage> VoterStoreProgressRequest;
        static Common::Global<FederationMessage> VoterStoreProgressReply;
        static Common::Global<FederationMessage> VoterStoreReadRequest;
        static Common::Global<FederationMessage> VoterStoreReadReply;
        static Common::Global<FederationMessage> VoterStoreWriteRequest;
        static Common::Global<FederationMessage> VoterStoreWriteReply;
        static Common::Global<FederationMessage> EmptyRequest;
        static Common::Global<FederationMessage> EmptyReply;

        Transport::ActionHeader actionHeader_;
        Transport::Actor::Enum actor_;
    };
}
