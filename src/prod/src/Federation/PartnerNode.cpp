// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Transport;
    using namespace Common;

    StringLiteral const TraceUnknown("Unknown");

    // TODO: header can be &&
    PartnerNode::PartnerNode(FederationPartnerNodeHeader const& value, SiteNode & siteNode)
        : siteId_(siteNode.Id),
        nodeInstance_(value.Instance),
        phase_(value.Phase),
        address_(value.Address),
        leaseAgentAddress_(value.LeaseAgentAddress),
        leaseAgentInstanceId_(value.LeaseAgentInstanceId),
        idString_(value.Instance.Id.ToString()),
        lastUpdated_(Stopwatch::Now()),
        lastAccessed_(lastUpdated_),
        lastConsider_(lastUpdated_),
        isUnknown_(false),
        unknownStart_(StopwatchTime::MaxValue),
        nextLivenessUpdate_(StopwatchTime::MaxValue),
        target_(value.Phase == NodePhase::Shutdown ? nullptr : siteNode.ResolveTarget(value.Address, value.Instance.Id.ToString(), nodeInstance_.InstanceId)),
        nodeFaultDomainId_(value.NodeFaultDomainId),
        token_(value.Token),
        globalTimeUpperLimit_(TimeSpan::MaxValue),
        lastGlobalTimeRefresh_(StopwatchTime::Zero),
        ringName_(value.RingName),
        flags_(value.Flags)
    {
    }

    PartnerNode::PartnerNode(NodeConfig const& nodeConfig, SiteNode & siteNode)
        :   siteId_(siteNode.Id),
            nodeInstance_(nodeConfig.Id, 0),
            phase_(NodePhase::Booting),
            address_(nodeConfig.Address),
            leaseAgentAddress_(nodeConfig.LeaseAgentAddress),
            leaseAgentInstanceId_(0),
            idString_(nodeConfig.Id.ToString()),
            lastUpdated_(Stopwatch::Now()),
            lastAccessed_(lastUpdated_),
            lastConsider_(lastUpdated_),
            isUnknown_(false),
            unknownStart_(StopwatchTime::MaxValue),
            nextLivenessUpdate_(StopwatchTime::MaxValue),
            target_(siteNode.ResolveTarget(nodeConfig.Address, nodeConfig.Id.ToString(), nodeInstance_.InstanceId)),
            token_(),
            globalTimeUpperLimit_(TimeSpan::MaxValue),
            lastGlobalTimeRefresh_(StopwatchTime::Zero),
            ringName_(nodeConfig.RingName),
            flags_(1)
    {
    }
    
    PartnerNode::PartnerNode(PartnerNode const & other)
        :   siteId_(other.siteId_),
            nodeInstance_(other.nodeInstance_),
            phase_(other.phase_),
            address_(other.address_),
            leaseAgentAddress_(other.leaseAgentAddress_),
            leaseAgentInstanceId_(other.leaseAgentInstanceId_),
            idString_(other.idString_),
            lastUpdated_(other.lastUpdated_),
            lastAccessed_(other.lastAccessed_),
            lastConsider_(other.lastConsider_),
            isUnknown_(other.isUnknown_),
            unknownStart_(other.unknownStart_),
            nextLivenessUpdate_(other.nextLivenessUpdate_),
            target_(other.target_),
            token_(other.token_),
            globalTimeUpperLimit_(other.globalTimeUpperLimit_),
            lastGlobalTimeRefresh_(other.lastGlobalTimeRefresh_),
            ringName_(other.ringName_)
    {
    }

    PartnerNode::PartnerNode(PartnerNodeSPtr const& oldNode, NodePhase::Enum phase, RoutingToken token)
        :   siteId_(oldNode->siteId_),
            nodeInstance_(oldNode->nodeInstance_),
            phase_(phase),
            address_(oldNode->address_),
            leaseAgentAddress_(oldNode->LeaseAgentAddress),
            leaseAgentInstanceId_(oldNode->LeaseAgentInstanceId),
            idString_(oldNode->idString_),
            lastUpdated_(Stopwatch::Now()),
            lastAccessed_(lastUpdated_),
            lastConsider_(lastUpdated_),
            isUnknown_(false),
            unknownStart_(StopwatchTime::MaxValue),
            nextLivenessUpdate_(StopwatchTime::MaxValue),
            target_(phase == NodePhase::Shutdown? nullptr : oldNode->target_),
            nodeFaultDomainId_(oldNode->nodeFaultDomainId_),
            token_(token),
            globalTimeUpperLimit_(oldNode->globalTimeUpperLimit_),
            lastGlobalTimeRefresh_(oldNode->lastGlobalTimeRefresh_),
            ringName_(oldNode->ringName_),
            flags_(oldNode->flags_)
    {
        if (phase == NodePhase::Shutdown)
        {
            if (oldNode->target_)
            {
                oldNode->target_->TargetDown(oldNode->nodeInstance_.InstanceId);
            }
        }
    }

    // TODO: target to self?    
    PartnerNode::PartnerNode(
        NodeId const& id,
        uint64 instanceId,
        wstring const& address,
        wstring const &leaseAgentAddress,
        Uri const & nodeFaultDomainId,
        wstring const & ringName)
        :   siteId_(id),
            nodeInstance_(id, instanceId),
            address_(address),
            leaseAgentAddress_(leaseAgentAddress),
            leaseAgentInstanceId_(0),
            phase_(NodePhase::Booting),
            idString_(id.ToString()),
            lastUpdated_(Stopwatch::Now()),
            lastAccessed_(lastUpdated_),
            lastConsider_(lastUpdated_),
            isUnknown_(false),
            unknownStart_(StopwatchTime::MaxValue),
            nextLivenessUpdate_(StopwatchTime::MaxValue),
            nodeFaultDomainId_(nodeFaultDomainId),
            token_(NodeIdRange::Empty, 0),
            globalTimeUpperLimit_(TimeSpan::MaxValue),
            lastGlobalTimeRefresh_(StopwatchTime::Zero),
            ringName_(ringName),
            flags_(1)
    {
    }

    PartnerNode::~PartnerNode()
    {
    }

    void PartnerNode::SetAsUnknown() const
    {
        unknownStart_ = Stopwatch::Now();

        if (!isUnknown_)
        {
            isUnknown_ = true;
            RoutingTable::WriteInfo(TraceUnknown, "{0} set {1} to unknown", siteId_, nodeInstance_);
        }
    }

    bool PartnerNode::getIsUnknown() const
    {
        if (unknownStart_ == StopwatchTime::MaxValue)
        {
            return false;
        }

        StopwatchTime now = Stopwatch::Now();
        if (unknownStart_ > now)
        {
            return false;
        }
        
        if (!isUnknown_)
        {
            RoutingTable::WriteInfo(TraceUnknown, "{0} set {1} as unknown starting at {2}",
                siteId_, nodeInstance_, unknownStart_);
            isUnknown_ = true;
        }

        return true;
    }

    void PartnerNode::OnSend(bool expectsReply) const
    {
        nextLivenessUpdate_ = StopwatchTime::MaxValue;
        if (expectsReply && unknownStart_ == StopwatchTime::MaxValue)
        {
            unknownStart_ = Stopwatch::Now() + (FederationConfig::GetConfig().RoutingRetryTimeout - StopwatchTime::TimerMargin);
        }
    }

    void PartnerNode::OnReceive(bool expectsReply) const
    {
        if (isUnknown_)
        {
            RoutingTable::WriteInfo(TraceUnknown, "{0} set {1} with unknown start time as {2} to known",
                siteId_, nodeInstance_, unknownStart_);
            isUnknown_ = false;
        }

        unknownStart_ = StopwatchTime::MaxValue;  

        if (expectsReply && nextLivenessUpdate_ == StopwatchTime::MaxValue)
        {
            nextLivenessUpdate_ = Stopwatch::Now() + FederationConfig::GetConfig().LivenessUpdateInterval;
        }
    }

    void PartnerNode::IncrementInstanceId()
    {
        nodeInstance_.IncrementInstanceId();
    }

    TimeSpan PartnerNode::getGlobalTimeUpperLimit() const
    {
        StopwatchTime now = Stopwatch::Now();
        FederationConfig const & config = FederationConfig::GetConfig();
        if (globalTimeUpperLimit_ != TimeSpan::MaxValue)
        {
            TimeSpan elapsed = now - lastGlobalTimeRefresh_;
            int64 delta = elapsed.Ticks / config.GlobalTimeClockDriftRatio;
            if (delta > 0)
            {
                globalTimeUpperLimit_ = globalTimeUpperLimit_ + TimeSpan::FromTicks(delta);
                lastGlobalTimeRefresh_ = lastGlobalTimeRefresh_ + TimeSpan::FromTicks(delta * config.GlobalTimeClockDriftRatio);
            }
        }

        return globalTimeUpperLimit_;
    }

    void PartnerNode::SetGlobalTimeUpperLimit(TimeSpan value) const
    {
        if (value < getGlobalTimeUpperLimit())
        {
            globalTimeUpperLimit_ = value;
            lastGlobalTimeRefresh_ = Stopwatch::Now();
        }
    }

    void PartnerNode::IncreaseGlobalTimeUpperLimit(TimeSpan value) const
    {
        globalTimeUpperLimit_ = globalTimeUpperLimit_.AddWithMaxAndMinValueCheck(value);
    }

    void PartnerNode::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("[{0},{1},{2},{3},{4},{5}]", nodeInstance_, phase_, address_, token_, unknownStart_.Ticks, lastAccessed_);
    }
}
