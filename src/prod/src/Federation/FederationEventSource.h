// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Federation/VoteEntry.h"
#include "Federation/VoteManager.h"
#pragma once

namespace Federation
{
    class SiteNodeEventSource
    {
    public:
        Common::TraceEventWriter<std::wstring, uint64> LeaseFailed;
        Common::TraceEventWriter<std::wstring, uint64> NeighborhoodLost;
        Common::TraceEventWriter<std::wstring> NeighborhoodChanged;
        Common::TraceEventWriter<std::wstring> RoutingTokenChanged;
        Common::TraceEventWriter<std::wstring, uint64> GlobalLeaseQuorumLost;
        Common::TraceEventWriter<std::wstring, uint64> RestartInstance;

        SiteNodeEventSource() :
            LeaseFailed(Common::TraceTaskCodes::SiteNode, 4, "LeaseFailed", Common::LogLevel::Error, "{0}:{1} OnLeaseFailed", "id", "instanceId"),
            NeighborhoodLost(Common::TraceTaskCodes::SiteNode, 5, "NeighborhoodLost", Common::LogLevel::Error, "{0}:{1} OnNeighborhoodLost", "id", "instanceId"),
            NeighborhoodChanged(Common::TraceTaskCodes::SiteNode, 6, "NeighborhoodChanged", Common::LogLevel::Info, "{0} OnNeighborhoodChanged", "id"),
            RoutingTokenChanged(Common::TraceTaskCodes::SiteNode, 7, "RoutingTokenChanged", Common::LogLevel::Info, "{0} OnRoutingTokenChanged", "id"),
            GlobalLeaseQuorumLost(Common::TraceTaskCodes::SiteNode, 8, "GlobalLeaseQuorumLost", Common::LogLevel::Error, "{0}:{1} OnGlobalLeaseQuorumLost", "id", "instanceId"),
            RestartInstance(Common::TraceTaskCodes::SiteNode, 9, "_Nodes_RestartInstance", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Restarting new instance {1}", "id", "instanceId")
        {
        }

        static Common::Global<SiteNodeEventSource> Events;
    };

    class ArbitrationEventSource
    {
    public:
        Common::TraceEventWriter<std::wstring, uint64, std::wstring, uint64, std::wstring, int64, int64, std::string> Failure;

        ArbitrationEventSource() :
            Failure(Common::TraceTaskCodes::Arbitration, 4, "_incidents_Failure", Common::LogLevel::Error, Common::TraceChannelType::Debug, Common::TraceKeywords::ForQuery, "{0}:{1} arbitration with {2}:{3} failed, lease instance: {5}/{6}, type={7}", "localLeaseAgentAddress", "localLeaseAgentInstance", "remoteLeaseAgentAddress", "remoteLeaseAgentInstance", "incidentType", "monitorLeaseInstance", "subjectLeaseInstance", "arbitrationType")
        {
        }

        static Common::Global<ArbitrationEventSource> Events;
    };

    class VoteManagerEventSource
    {
    public:
        Common::TraceEventWriter<Federation::NodeId, Federation::VoteManager, int64> UpdateGlobalTickets;
        Common::TraceEventWriter<Federation::NodeId, std::wstring, int, int64> TTL;
        Common::TraceEventWriter<std::wstring, Common::TimeSpan, int64, Common::TimeSpan, Common::TimeSpan> InvalidLowerTimeRange;
        Common::TraceEventWriter<std::wstring, Common::TimeSpan, int64, Common::TimeSpan, Common::TimeSpan> InvalidUpperTimeRange;
        Common::TraceEventWriter<uint16, Common::StringLiteral, std::vector<Federation::VoteEntryUPtr>> VoteManager;
        Common::TraceEventWriter<uint16, Federation::NodeId, Federation::NodeId, bool, std::wstring, int64> VoteEntry;

        VoteManagerEventSource() :
            VoteManager(Common::TraceTaskCodes::VoteManager, 4, "VoteManager", Common::LogLevel::Info, "{1}\r\n{2}\r\n", "contextSequenceId", "tag", "entries"),
            VoteEntry(Common::TraceTaskCodes::VoteManager, 5, "VoteEntry", Common::LogLevel::Info, "{1}@{2}:{3},{4}/{5}\r\n", "contextSequenceId", "nodeId", "siteNodeId", "isAcquired", "time", "ticks"),
            InvalidLowerTimeRange(Common::TraceTaskCodes::VoteManager, 6, "_incidents_InvalidLowerTimeRange", Common::LogLevel::Warning, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Trying to set lower limit to {1} on {0} with {2}:{3}/{4}", "id", "input", "epoch", "lower", "upper"),
            InvalidUpperTimeRange(Common::TraceTaskCodes::VoteManager, 7, "_incidents_InvalidUpperTimeRange", Common::LogLevel::Warning, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Trying to set upper limit to {1} on {0} with {2}:{3}/{4}", "id", "input", "epoch", "lower", "upper"),
            UpdateGlobalTickets(Common::TraceTaskCodes::VoteManager, 10, "UpdateGlobalTickets_Tickets", Common::LogLevel::Info, "{0} updated global tickets: {1}, ticks={2}", "siteNodeId", "votemgr", "ticks"),
            TTL(Common::TraceTaskCodes::VoteManager, 11, "VoteManager_TTL", Common::LogLevel::Info, "{0}: globalLeaseExpiration_ updated to {1}, quorumCount_ = {2}, ticket total = {3}", "siteNodeId", "globalLeaseExpiration", "quorumCount", "expirationSize")
        {
        }

        static Common::Global<VoteManagerEventSource> Events;
    };

    class VoteProxyEventSource
    {
    public:
        Common::TraceEventWriter<Federation::NodeId, Common::DateTime, Common::DateTime, Common::DateTime> VoteOwnerDataRead;
        Common::TraceEventWriter<Federation::NodeId, Common::DateTime, Common::DateTime, Common::DateTime, int64> VoteOwnerDataWrite;

    public:
        VoteProxyEventSource()
            : VoteOwnerDataRead(Common::TraceTaskCodes::VoteProxy, 4, "VoteOwnerDataRead", Common::LogLevel::Info, "Value read: {0},{1},{2},{3}", "id", "expiry", "globalTicket", "superTicket"),
              VoteOwnerDataWrite(Common::TraceTaskCodes::VoteProxy, 5, "VoteOwnerDataWrite", Common::LogLevel::Info, "Value to write: {0},{1},{2},{3} seq={4}", "id", "expiry", "globalTicket", "superTicket", "seq")
        {
        }

        static Common::Global<VoteProxyEventSource> Events;
    };

    class BroadcastEventSource
    {
    public:
        Common::TraceEventWriter<std::wstring, Transport::MessageId, Federation::NodeInstance, Federation::NodeInstance> ForwardToSuccessor;
        Common::TraceEventWriter<std::wstring, Transport::MessageId, Federation::NodeInstance, Federation::NodeInstance> ForwardToPredecessor;
        Common::TraceEventWriter<std::wstring, Transport::MessageId, Federation::NodeId, std::wstring> ForwardToRange;


    public:
        BroadcastEventSource() :
            ForwardToSuccessor(Common::TraceTaskCodes::Broadcast, 4, "ForwardToSuccessor_Forward", Common::LogLevel::Info, "Broadcasting message {0} for broadcast id {1} to succ: {2} from {3}", "action", "broadcastId", "successor", "siteNode"),
            ForwardToPredecessor(Common::TraceTaskCodes::Broadcast, 5, "ForwardToPredecessor_Forward", Common::LogLevel::Info, "Broadcasting message {0} for broadcast id {1} to pred: {2} from {3}", "action", "broadcastId", "successor", "siteNode"),
            ForwardToRange(Common::TraceTaskCodes::Broadcast, 6, "ForwardToRange_Forward", Common::LogLevel::Info, "Broadcasting message {0} for broadcast id {1} to node: {2} with range {3}", "action", "broadcastid", "nodeid", "range") 
        {
        }

        static Common::Global<BroadcastEventSource> Events;
    };
}
