// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "Transport/Message.h"
#include "Transport/MessageHeader.h"
#include "Transport/MessageHeaderId.h"
#include "Transport/MessageHeaders.h"
#include "Federation/FederationMessage.h"
#include "Federation/Constants.h"

namespace Federation
{
    using namespace std;
    using namespace Common;
    using namespace Transport;

    Transport::Actor::Enum const FederationMessage::Actor(Actor::Federation);

    // Federation messages
    // Messages to be processed by the Join Manager.
    Global<FederationMessage> FederationMessage::NeighborhoodQueryRequest = make_global<FederationMessage>(L"NeighborhoodQueryRequest");
    Global<FederationMessage> FederationMessage::NeighborhoodQueryReply = make_global<FederationMessage>(L"NeighborhoodQueryReply");
    Global<FederationMessage> FederationMessage::ResumeJoin = make_global<FederationMessage>(L"ResumeJoin");
    Global<FederationMessage> FederationMessage::LockRequest = make_global<FederationMessage>(L"LockRequest");
    Global<FederationMessage> FederationMessage::LockGrant = make_global<FederationMessage>(L"LockGrant");
    Global<FederationMessage> FederationMessage::LockDeny = make_global<FederationMessage>(L"LockDeny");
    Global<FederationMessage> FederationMessage::LockTransferRequest = make_global<FederationMessage>(L"LockTransferRequest");
    Global<FederationMessage> FederationMessage::LockTransferReply = make_global<FederationMessage>(L"LockTransferReply");
    Global<FederationMessage> FederationMessage::LockRenew = make_global<FederationMessage>(L"LockRenew");
    Global<FederationMessage> FederationMessage::LockRelease = make_global<FederationMessage>(L"LockRelease");
    Global<FederationMessage> FederationMessage::UnlockRequest = make_global<FederationMessage>(L"UnlockRequest");
    Global<FederationMessage> FederationMessage::UnlockGrant = make_global<FederationMessage>(L"UnlockGrant");
    Global<FederationMessage> FederationMessage::UnlockDeny = make_global<FederationMessage>(L"UnlockDeny");


    Global<FederationMessage> FederationMessage::Depart = make_global<FederationMessage>(L"Depart");
    Global<FederationMessage> FederationMessage::NodeDoesNotMatchFault = make_global<FederationMessage>(L"NodeDoesNotMatchFault");
    Global<FederationMessage> FederationMessage::RejectFault = make_global<FederationMessage>(L"RejectFault");
    Global<FederationMessage> FederationMessage::TokenRequest = make_global<FederationMessage>(L"TokenRequest");
    Global<FederationMessage> FederationMessage::TokenTransfer = make_global<FederationMessage>(L"TokenTransfer");
    Global<FederationMessage> FederationMessage::TokenTransferAccept = make_global<FederationMessage>(L"TokenTransferAccept");
    Global<FederationMessage> FederationMessage::TokenTransferReject = make_global<FederationMessage>(L"TokenTransferReject");
    Global<FederationMessage> FederationMessage::EdgeProbe = make_global<FederationMessage>(L"EdgeProbe");
    Global<FederationMessage> FederationMessage::RingAdjust = make_global<FederationMessage>(L"RingAdjust");
    Global<FederationMessage> FederationMessage::TokenProbe = make_global<FederationMessage>(L"TokenProbe");
    Global<FederationMessage> FederationMessage::TokenEcho = make_global<FederationMessage>(L"TokenEcho");
    Global<FederationMessage> FederationMessage::Ping = make_global<FederationMessage>(L"Ping");
    Global<FederationMessage> FederationMessage::UpdateRequest = make_global<FederationMessage>(L"UpdateRequest");
    Global<FederationMessage> FederationMessage::UpdateReply = make_global<FederationMessage>(L"UpdateReply");

    // VoteManager messages
    Global<FederationMessage> FederationMessage::VotePing = make_global<FederationMessage>(L"VotePing");
    Global<FederationMessage> FederationMessage::VotePingReply = make_global<FederationMessage>(L"VotePingReply");
    Global<FederationMessage> FederationMessage::VoteTransferRequest = make_global<FederationMessage>(L"VoteTransferRequest");
    Global<FederationMessage> FederationMessage::VoteTransferReply = make_global<FederationMessage>(L"VoteTransferReply");
	Global<FederationMessage> FederationMessage::VoteRenewRequest = make_global<FederationMessage>(L"VoteRenewRequest");
	Global<FederationMessage> FederationMessage::VoteRenewReply = make_global<FederationMessage>(L"VoteRenewReply");

    // ArbitrationManager messages
    Global<FederationMessage> FederationMessage::ArbitrateRequest = make_global<FederationMessage>(L"ArbitrateRequest");
    Global<FederationMessage> FederationMessage::ExtendedArbitrateRequest = make_global<FederationMessage>(L"ExtendedArbitrateRequest");
    Global<FederationMessage> FederationMessage::ArbitrateReply = make_global<FederationMessage>(L"ArbitrateReply");
    Global<FederationMessage> FederationMessage::DelayedArbitrateReply = make_global<FederationMessage>(L"DelayedArbitrateReply");
    Global<FederationMessage> FederationMessage::ArbitrateKeepAlive = make_global<FederationMessage>(L"ArbitrateKeepAlive");

    // Neighborhood recovery messages
    Global<FederationMessage> FederationMessage::TicketGapRequest = make_global<FederationMessage>(L"TicketGapRequest");
    Global<FederationMessage> FederationMessage::TicketGapReply = make_global<FederationMessage>(L"TicketGapReply");

    Global<FederationMessage> FederationMessage::LivenessQueryRequest = make_global<FederationMessage>(L"LivenessQueryRequest");
    Global<FederationMessage> FederationMessage::LivenessQueryReply = make_global<FederationMessage>(L"LivenessQueryReply");

    // ACKs are used by high layers and we don't need them to carry liveness headers
    Global<FederationMessage> FederationMessage::RoutingAck = make_global<FederationMessage>(L"RoutingAck", Actor::Empty);
    Global<FederationMessage> FederationMessage::BroadcastAck = make_global<FederationMessage>(L"BroadcastAck", Actor::Empty);
    Global<FederationMessage> FederationMessage::MulticastAck = make_global<FederationMessage>(L"MulticastAck", Actor::Empty);

    Global<FederationMessage> FederationMessage::LivenessUpdate = make_global<FederationMessage>(L"LivenessUpdate");

    Global<FederationMessage> FederationMessage::ExternalRingPing = make_global<FederationMessage>(L"ExternalRingPing");
    Global<FederationMessage> FederationMessage::ExternalRingPingResponse = make_global<FederationMessage>(L"ExternalRingPingResponse");

    Global<FederationMessage> FederationMessage::VoterStoreConfigQueryRequest = make_global<FederationMessage>(L"VoterStoreConfigQueryRequest");
    Global<FederationMessage> FederationMessage::VoterStoreConfigQueryReply = make_global<FederationMessage>(L"VoterStoreConfigQueryReply");
    Global<FederationMessage> FederationMessage::VoterStoreIntroduceRequest = make_global<FederationMessage>(L"VoterStoreIntroduceRequest");
    Global<FederationMessage> FederationMessage::VoterStoreIntroduceReply = make_global<FederationMessage>(L"VoterStoreIntroduceReply");
    Global<FederationMessage> FederationMessage::VoterStoreBootstrapRequest = make_global<FederationMessage>(L"VoterStoreBootstrapRequest");
    Global<FederationMessage> FederationMessage::VoterStoreBootstrapReply = make_global<FederationMessage>(L"VoterStoreBootstrapReply");
    Global<FederationMessage> FederationMessage::VoterStoreLeaderProbeRequest = make_global<FederationMessage>(L"VoterStoreLeaderProbeRequest");
    Global<FederationMessage> FederationMessage::VoterStoreLeaderProbeReply = make_global<FederationMessage>(L"VoterStoreLeaderProbeReply");
    Global<FederationMessage> FederationMessage::VoterStoreJoin = make_global<FederationMessage>(L"VoterStoreJoin");
    Global<FederationMessage> FederationMessage::VoterStoreSyncRequest = make_global<FederationMessage>(L"VoterStoreSyncRequest");
    Global<FederationMessage> FederationMessage::VoterStoreSyncReply = make_global<FederationMessage>(L"VoterStoreSyncReply");
    Global<FederationMessage> FederationMessage::VoterStoreProgressRequest = make_global<FederationMessage>(L"VoterStoreProgressRequest");
    Global<FederationMessage> FederationMessage::VoterStoreProgressReply = make_global<FederationMessage>(L"VoterStoreProgressReply");
    Global<FederationMessage> FederationMessage::VoterStoreReadRequest = make_global<FederationMessage>(L"VoterStoreReadRequest");
    Global<FederationMessage> FederationMessage::VoterStoreReadReply = make_global<FederationMessage>(L"VoterStoreReadReply");
    Global<FederationMessage> FederationMessage::VoterStoreWriteRequest = make_global<FederationMessage>(L"VoterStoreWriteRequest");
    Global<FederationMessage> FederationMessage::VoterStoreWriteReply = make_global<FederationMessage>(L"VoterStoreWriteReply");

    Global<FederationMessage> FederationMessage::EmptyRequest = make_global<FederationMessage>(L"EmptyRequest");
    Global<FederationMessage> FederationMessage::EmptyReply = make_global<FederationMessage>(L"EmptyReply");

    Transport::MessageUPtr FederationMessage::CreateMessage() const
    {
        auto message = Common::make_unique<Message>();
        message->Headers.Add(ActorHeader(actor_));
        message->Headers.Add(actionHeader_);
        message->Headers.Add(HighPriorityHeader(true));
        return std::move(message);
    }

    bool FederationMessage::IsFederationMessage(Message const & msg)
    {
        return msg.Actor == Actor;
    }

    FederationRoutingTokenHeader::FederationRoutingTokenHeader(NodeIdRange const & range, uint64 sourceVersion, uint64 const & targetVersion)
        : range_(range), sourceVersion_(sourceVersion), targetVersion_(targetVersion)
    {
        expiryTime_ = Common::Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
    }

    void FederationRoutingTokenHeader::WriteTo(TextWriter& w, FormatOptions const &) const
    { 
        w.Write("[{0}:{1:x},{2:x}]", range_, sourceVersion_, targetVersion_); 
    }
}
