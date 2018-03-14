// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/TicketGapRequest.h"

namespace Federation
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace LeaseWrapper;

    static const StringLiteral RoutingTableTimerTag("RoutingTable");

    StringLiteral const TraceSuspect("Suspect");
    StringLiteral const TraceEdge("Edge");
    StringLiteral const TraceUnknown("Unknown");
    StringLiteral const TraceDown("Down");
    StringLiteral const TraceNeighbor("Neighbor");
    StringLiteral const TraceConsider("Consider");
    StringLiteral const TraceState("State");
    StringLiteral const TraceHealth("Health");
    StringLiteral const TraceGap("Gap");
    StringLiteral const TraceCompact("Compact");
    StringLiteral const TraceEdgeProbe("EdgeProbe");

    TextTraceWriter WriteTokenInfo(TraceTaskCodes::Token, LogLevel::Info);
    TextTraceWriter WriteTokenWarning(TraceTaskCodes::Token, LogLevel::Warning);
    StringLiteral const TraceUpdate("Update");
    StringLiteral const TraceReceive("Receive");
    StringLiteral const TraceAccept("Accept");
    StringLiteral const TraceReject("Reject");
    StringLiteral const TraceSplit("Split");
    StringLiteral const TraceRelease("Release");
    StringLiteral const TraceRecovery("Recovery");
    StringLiteral const TraceTransfer("Transfer");
    StringLiteral const TraceRequest("Request");
    StringLiteral const TraceProbe("Probe");

    StringLiteral const TraceImplicitLease("Implicit");
    StringLiteral const TraceNeighborLease("Lease");

    class RoutingTable::WriteLock
    {
        DENY_COPY(WriteLock)

    public:
        WriteLock(RoutingTable & table)
            : grab_(table.lock_), table_(table)
        {
            oldTokenVersion_ = table.site_.Token.Version;
            oldNeighborhoodVersion_ = table.neighborhoodVersion_;
        }

        ~WriteLock()
        {
            if (!table_.isTestMode_)
            {
                if (table_.neighborhoodVersion_ != oldNeighborhoodVersion_)
                {
                    table_.OnNeighborhoodChanged(oldTokenVersion_ == table_.site_.Token.Version);
                }

                if (oldTokenVersion_ != table_.site_.Token.Version)
                {
                    table_.OnRoutingTokenChanged((static_cast<uint>(oldTokenVersion_)) != table_.site_.Token.TokenVersion);
                }
            }
        }

    private:
        AcquireWriteLock grab_;
        RoutingTable & table_;
        uint64 oldTokenVersion_;
        uint oldNeighborhoodVersion_;
    };

    class GapRequestAction : public StateMachineAction
    {
        DENY_COPY(GapRequestAction)

    public:
        GapRequestAction(MessageUPtr && message, NodeId targetId)
            :   message_(move(message)),
                targetId_(targetId)
        {
        }

        virtual void Execute(SiteNode & siteNode)
        {
            auto siteNodeSPtr = siteNode.GetSiteNodeSPtr();
            FederationConfig & config = FederationConfig::GetConfig();
            siteNode.BeginRouteRequest(
                move(message_), 
                targetId_, 
                0, 
                true,
                config.MessageTimeout, 
                config.GlobalTicketLeaseDuration,
                [this, siteNodeSPtr] (AsyncOperationSPtr const & operation) { GapRequestAction::ProcessReply(siteNodeSPtr, operation); }, 
                siteNode.CreateAsyncOperationRoot());
        }

        static void ProcessReply(SiteNodeSPtr const & siteNode, AsyncOperationSPtr const & operation)
        {
            MessageUPtr reply;
            ErrorCode error = siteNode->EndRouteRequest(operation, reply);
            if (error.IsSuccess())
            {
                siteNode->GetVoteManager().UpdateGlobalTickets(*reply, siteNode->Instance);
            }
        }

        wstring ToString()
        {
            return wformatString("{0}:{1}", message_->Action, targetId_);
        }

        MessageUPtr message_;
        NodeId targetId_;
    };

    class LivenessQueryAction : public StateMachineAction
    {
        DENY_COPY(LivenessQueryAction)

    public:
        LivenessQueryAction(PartnerNodeSPtr const & target)
            : target_(*target)
        {
        }

        virtual void Execute(SiteNode & siteNode)
        {
            siteNode.LivenessQuery(target_);
        }

        wstring ToString()
        {
            return wformatString("LivenessQuery:{0}", target_.Instance);
        }

    private:
        FederationPartnerNodeHeader target_;
    };

    class GapCheckAction : public StateMachineAction
    {
        DENY_COPY(GapCheckAction)

    public:
        GapCheckAction(NodeIdRange const & range)
			: range_(range)
        {
        }

        virtual void Execute(SiteNode & siteNode)
        {
			siteNode.getRoutingTable().CheckGapRecovery(range_);
        }

        wstring ToString()
        {
			return wformatString("GapCheck: {0}", range_);
        }

	private:
		NodeIdRange range_;
    };

    PartnerNodeSPtr const RoutingTable::NullNode = PartnerNodeSPtr(nullptr);

    RoutingTable::RoutingTable(SiteNodeSPtr const& site, int hoodSize)
        : site_(*(site.get())),
        hoodSize_(hoodSize),
        safeHoodSize_(hoodSize / 2),
        ring_(PartnerNodeSPtr(site)),
        knownTable_(PartnerNodeSPtr(site), hoodSize),
        lock_(),
        timer_(),
        lastSuccEdgeProbe_(StopwatchTime::Zero),
        lastPredEdgeProbe_(StopwatchTime::Zero),
        lastSuccTokenProbe_(StopwatchTime::Zero),
        lastPredTokenProbe_(StopwatchTime::Zero),
        tokenProbeThrottle_(FederationConfig::GetConfig().TokenProbeThrottleThreshold, FederationConfig::GetConfig().TokenProbeInterval, FederationConfig::GetConfig().TokenProbeIntervalUpperBound),
        succProbeTarget_(site->Id),
        predProbeTarget_(site->Id),
        ackedProbe_(site->Id),
        probeResult_(false),
        succRecoveryTime_(StopwatchTime::Zero),
        predRecoveryTime_(StopwatchTime::Zero),
        gapReportTime_(StopwatchTime::Zero),
        lastCompactTime_(StopwatchTime::Zero),
        lastTraceTime_(StopwatchTime::Zero),
        tokenAcquireDeadline_(StopwatchTime::MaxValue),
        neighborhoodVersion_(0),
        isRangeConsistent_(false),
        isNeighborLostReported_(false),
        rangeLockExpiryTime_(StopwatchTime::Zero),
        succPendingTransfer_(),
        predPendingTransfer_(),
        succEchoList_(),
        predEchoList_(),
        isHealthy_(false),
        isTestMode_(false),
        globalTimeManager_(*site, lock_),
        lastGlobalTimeUncertaintyIncreaseTime_(Stopwatch::Now()),
        implicitLeaseContext_(*site)
    {
        timer_ = Timer::Create(
            RoutingTableTimerTag,
            [this, site] (TimerSPtr const &)
            {
                this->OnTimer();
            },
            true);
    }

    RoutingTable::~RoutingTable()
    {
    }

    int RoutingTable::GetRoutingNodeCount() const
    {
        AcquireReadLock grab(lock_);
        return ring_.GetRoutingNodeCount();
    }

    PartnerNodeSPtr RoutingTable::FindClosest(NodeId const& value, wstring const & toRing) const
    {
        AcquireReadLock grab(lock_);
        return InternalFindClosest(value, toRing, false);
    }

    PartnerNodeSPtr RoutingTable::GetRoutingHop(NodeId const& value, wstring const & toRing, bool safeMode, bool& ownsToken) const
    {
        AcquireReadLock grab(lock_);
        
        ownsToken = (site_.IsRingNameMatched(toRing) && site_.Token.Contains(value));
        if (ownsToken)
        {
            return knownTable_.ThisNodePtr;
        }

        return InternalFindClosest(value, toRing, safeMode);
    }

    PartnerNodeSPtr const& RoutingTable::InternalFindClosest(NodeId const& value, wstring const & toRing, bool safeMode) const
    {
        if (site_.IsRingNameMatched(toRing))
        {
            return InternalFindClosest(value, ring_);
        }
        else
        {
            auto it = externalRings_.find(toRing);
            if (it != externalRings_.end())
            {
                if (safeMode)
                {
                    PartnerNodeSPtr const & result = it->second.GetRoutingSeedNode();
                    if (result)
                    {
                        return result;
                    }
                }

                return InternalFindClosest(value, it->second);
            }

            return knownTable_.ThisNodePtr;
        }
    }

    PartnerNodeSPtr const& RoutingTable::InternalFindClosest(NodeId const& value, NodeRingBase const & ring) const
    {
        if (ring.Size == 0)
        {
            return knownTable_.ThisNodePtr;
        }

        bool isLocal = (&ring == &ring_);
        size_t succOrSame = ring.FindSuccOrSamePosition(value);
        size_t pred = ring.GetPred(succOrSame);

        // to save the first routing (but may be unknown) node on both side, initialize them to avoid warning
        size_t savedSuccOrSame = succOrSame;
        size_t savedPred = pred;

        // try to find an routing and known node in the successor side, 
        // also save the first routing (but maybe unknown) node we found
        bool found = false;
        bool foundSuccRouting = false;
        for (size_t i = 0; i < ring.Size; i++)
        {
            PartnerNodeSPtr const& currentNode = ring.GetNode(succOrSame);

            if (currentNode->IsRouting)
            {
                if (!foundSuccRouting)
                {
                    savedSuccOrSame = succOrSame;
                    foundSuccRouting = true;
                }

                if (!currentNode->IsUnknown || (isLocal && site_.IsAvailable && knownTable_.WithinHoodRange(currentNode->Id)))
                {
                    found = true;
                    break;
                }
            }

            succOrSame = ring.GetSucc(succOrSame);
        }

        if (!foundSuccRouting)
        {
            // no routing node in the routing table
            return (isLocal ? NullNode : knownTable_.ThisNodePtr);
        }

        // try to find an routing and known node in the predecessor side, 
        // also save the first routing (but maybe unknown) node we found
        bool foundPredRouting = false;

        for (size_t i = 0; i < ring.Size; i++)
        {
            PartnerNodeSPtr const& currentNode = ring.GetNode(pred);

            if (currentNode->IsRouting)
            {
                if (!foundPredRouting)
                {
                    savedPred = pred;
                    foundPredRouting = true;
                }

                if (!currentNode->IsUnknown || (isLocal && site_.IsAvailable && knownTable_.WithinHoodRange(currentNode->Id)))
                {
                    break;
                }
            }

            pred = ring.GetPred(pred);
        }

        ASSERT_IF(!foundPredRouting, "Found routing node in successor side but not in predecessor side");

        if (found)
        {
            // found on successor side is equivalent to found on both side
            PartnerNodeSPtr const& predNode = ring.GetNode(pred);
            PartnerNodeSPtr const& succOrSameNode = ring.GetNode(succOrSame);

            // Check which one has smallest distance,
            // if the distances are same, return the predecessor.
            // If the node to return equal to "this" node, don't return here
            // but will check whether it is better than the saved unknown nodes
            if (value.PredDist(predNode->Id) <= value.SuccDist(succOrSameNode->Id))
            {
                if (!isLocal || pred != ring_.ThisNode)
                {
                    return predNode;
                }
            }
            else
            {
                if (!isLocal || succOrSame != ring_.ThisNode)
                {
                    return succOrSameNode;
                }
            }
        }

        // if no routing and known node found, or the best routing and known node is "this" node
        // we return the best one from the unknown routing nodes and "this" node
        PartnerNodeSPtr const& savedPredNode = ring.GetNode(savedPred);
        PartnerNodeSPtr const& savedSuccOrSameNode = ring.GetNode(savedSuccOrSame);
        return (value.PredDist(savedPredNode->Id) <= 
            value.SuccDist(savedSuccOrSameNode->Id)) ? 
            savedPredNode : savedSuccOrSameNode;
    }

    PartnerNodeSPtr RoutingTable::Get(NodeInstance const & value) const
    {
        AcquireReadLock grab(lock_);
        return GetInternal(value);
    }

    PartnerNodeSPtr RoutingTable::Get(NodeInstance const & value, wstring const & ringName) const
    {
        AcquireReadLock grab(lock_);
        PartnerNodeSPtr result = GetInternal(value.Id, ringName);
        return (result && result->Instance.InstanceId == value.InstanceId ? result : nullptr);
    }

    PartnerNodeSPtr const & RoutingTable::GetInternal(NodeInstance const & value) const
    {
        PartnerNodeSPtr const & result = GetInternal(value.Id, &knownTable_);
        return (result && result->Instance.InstanceId == value.InstanceId ? result : NullNode);
    }

    PartnerNodeSPtr RoutingTable::Get(NodeId value) const
    {
        AcquireReadLock grab(lock_);
        return GetInternal(value, &knownTable_);
    }

    PartnerNodeSPtr RoutingTable::Get(NodeId value, wstring const & ringName) const
    {
        AcquireReadLock grab(lock_);
        return GetInternal(value, ringName);
    }

    PartnerNodeSPtr const & RoutingTable::GetInternal(NodeId value, wstring const & ringName) const
    {
        NodeRingBase const * ring;
        if (ringName.empty() || ringName == site_.RingName)
        {
            ring = &knownTable_;
        }
        else
        {
            auto it = externalRings_.find(ringName);
            if (it == externalRings_.end())
            {
                return NullNode;
            }

            ring = &(it->second);
        }

        return GetInternal(value, ring);
    }

    bool RoutingTable::InternalIsDown(NodeInstance const & nodeInstance) const
    {
        PartnerNodeSPtr node = GetInternal(nodeInstance.Id, &knownTable_);
        return ((node != nullptr) &&
                ((node->Instance.InstanceId > nodeInstance.InstanceId) ||
                 (node->Instance.InstanceId == nodeInstance.InstanceId && node->IsShutdown)));
    }

    bool RoutingTable::IsDown(NodeInstance const & nodeInstance) const
    {
        AcquireReadLock grab(lock_);
        return InternalIsDown(nodeInstance);
    }

    PartnerNodeSPtr const & RoutingTable::GetInternal(NodeId value, NodeRingBase const* ring) const
    {
        if (ring->Size > 0)
        {
            size_t succOrSame = ring->FindSuccOrSamePosition(value);
            PartnerNodeSPtr const& succOrSameNode = ring->GetNode(succOrSame);
            if (succOrSameNode->Id == value)
            {
                return succOrSameNode;
            }
        }

        return NullNode;
    }

    void RoutingTable::Consider(FederationPartnerNodeHeader const & nodeInfo, bool isInserting)
    {
        WriteLock grab(*this);
        InternalConsider(nodeInfo, isInserting);
    }

    PartnerNodeSPtr RoutingTable::ConsiderAndReturn(FederationPartnerNodeHeader const & nodeInfo, bool isInserting)
    {
        WriteLock grab(*this);

        return InternalConsider(nodeInfo, isInserting);
    }

    PartnerNodeSPtr const& RoutingTable::InternalConsider(FederationPartnerNodeHeader const & nodeInfo, bool isInserting, int64 now)
    {
        if (nodeInfo.Instance.Id == site_.Id && nodeInfo.RingName == site_.RingName)
        {
            return knownTable_.ThisNodePtr;
        }

        if (!site_.IsRingNameMatched(nodeInfo.RingName))
        {
            return InternalConsiderExternalNode(nodeInfo);
        }

        size_t knownPosition = knownTable_.FindSuccOrSamePosition(nodeInfo.Instance.Id);
        PartnerNodeSPtr const& knownPtr = knownTable_.GetNode(knownPosition);
        bool existInKnown = (knownPtr->Id == nodeInfo.Instance.Id);
        size_t routingPosition = ring_.FindSuccOrSamePosition(nodeInfo.Instance.Id);
        PartnerNodeSPtr const& routingPtr = ring_.GetNode(routingPosition);
        bool existInRouting = (routingPtr->Id == nodeInfo.Instance.Id);

        ASSERT_IF(existInRouting && !existInKnown,
            "{0} in routing but not in known ring {1}",
            nodeInfo.Instance, *this);

        if (existInKnown &&
            ((knownPtr->Instance.InstanceId > nodeInfo.Instance.InstanceId) ||
             ((knownPtr->Instance.InstanceId == nodeInfo.Instance.InstanceId) &&
              (knownPtr->Phase > nodeInfo.Phase))))
        {
            // stale information, return directly
            knownPtr->UpdateLastAccess(now ? StopwatchTime(now) : Stopwatch::Now());
            return knownPtr;
        }

        if (nodeInfo.IsAvailable &&
            (!existInRouting || routingPtr->Instance.InstanceId < nodeInfo.Instance.InstanceId) &&
            knownTable_.WithinHoodRange(nodeInfo.Instance.Id) &&
            site_.IsAvailable)
        {
            // if the incoming node info is routing or inserting (but not from a
            // inserting message), and the node id is in our neighborhood
            // range, but the node is not in the routing table (new node), ignore it.
            // This is because by our consistency rule such node can't exist and must
            // be that we are encountering a stale information after we forget a
            // shutdown node
            bool setShutdown = false;
            bool ignoreIt = false;

            if (nodeInfo.Phase == NodePhase::Routing)
            {
                if (site_.IsRouting && !site_.IsLeaseExpired())
                {
                    setShutdown = true;
                }
                else
                {
                    ignoreIt = true;
                }
            }
            else if (!isInserting)
            {
                if (site_.IsRouting && !site_.IsLeaseExpired() && (!existInKnown || knownPtr->Instance.InstanceId < nodeInfo.Instance.InstanceId))
                {
                    setShutdown = true;
                }
                else
                {
                    ignoreIt = true;
                }
            }

            if (setShutdown)
            {
                WriteWarning(TraceSuspect,
                    "{0} set suspected stale header {1} to shutdown",
                    site_.Id, nodeInfo);
                return InternalConsider(FederationPartnerNodeHeader(nodeInfo.Instance, NodePhase::Shutdown, nodeInfo.Address, nodeInfo.LeaseAgentAddress, nodeInfo.LeaseAgentInstanceId, nodeInfo.Token, nodeInfo.NodeFaultDomainId, false, nodeInfo.RingName, nodeInfo.Flags));
            }
            else if (ignoreIt)
            {
                WriteInfo(TraceSuspect,
                    "{0} ignore suspected stale header {1}",
                    site_.Id, nodeInfo);
                return NullNode;
            }
        }

        // first check the known table
        size_t newKnownPosition = knownPosition;
        bool tokenUpdateOnly = false;

        if (!existInKnown)
        {
            // no such node, add node
            newKnownPosition = knownTable_.AddNode(make_shared<PartnerNode>(nodeInfo, site_), knownPosition);
        }
        else if (knownPtr->Instance.InstanceId < nodeInfo.Instance.InstanceId)
        {
            if (knownPtr->Target)
            {
                knownPtr->Target->TargetDown(knownPtr->Instance.InstanceId);
            }
            knownTable_.ReplaceNode(knownPosition, make_shared<PartnerNode>(nodeInfo, site_));
        }
        else
        {
            if (knownPtr->Token.Version < nodeInfo.Token.Version && knownPtr->Phase < nodeInfo.Phase)
            {
                knownTable_.ReplaceNode(knownPosition, make_shared<PartnerNode>(knownPtr, nodeInfo.Phase, nodeInfo.Token));
            }
            else if (knownPtr->Token.Version < nodeInfo.Token.Version)
            {
                knownTable_.ReplaceNode(knownPosition, make_shared<PartnerNode>(knownPtr, knownPtr->Phase, nodeInfo.Token));
                tokenUpdateOnly = true;
            }
            else if (knownPtr->Phase < nodeInfo.Phase)
            {
                knownTable_.ReplaceNode(knownPosition, make_shared<PartnerNode>(knownPtr, nodeInfo.Phase, knownPtr->Token));
            }
            else
            {
                StopwatchTime nowTime = (now ? StopwatchTime(now) : Stopwatch::Now());
                if ((nodeInfo.Token.Version != 0 && knownPtr->Token.Version != nodeInfo.Token.Version) ||
                    knownPtr->Phase != nodeInfo.Phase)
                {
                    knownPtr->UpdateLastAccess(nowTime);
                }
                else
                {
                    knownPtr->UpdateLastConsider(nowTime);
                }

                return knownPtr;
            }
        }

        PartnerNodeSPtr const& newPtrAdded = knownTable_.GetNode(newKnownPosition);

        // next consider update routing table
        bool neighborhoodChanged = false;
        if (existInRouting)
        {
            neighborhoodChanged = knownTable_.WithinHoodRange(nodeInfo.Instance.Id);

            if (nodeInfo.IsAvailable)
            {
                if (neighborhoodChanged && knownPtr->Instance < newPtrAdded->Instance)
                {
                    site_.leaseAgentUPtr_->Terminate(knownPtr->Instance.ToString());
                }

                ring_.ReplaceNode(routingPosition, newPtrAdded);

                neighborhoodChanged = neighborhoodChanged && !tokenUpdateOnly;
            }
            else
            {
                if (neighborhoodChanged)
                {
                    site_.leaseAgentUPtr_->Terminate(knownPtr->Instance.ToString());
                }
                ring_.RemoveNode(routingPosition);
            }
        }
        else if (nodeInfo.IsAvailable)
        {
            ring_.AddNode(newPtrAdded, routingPosition);

            if (!knownTable_.CompleteHoodRange)
            {
                while (knownTable_.CanShrinkPredHoodRange())
                {
                    PartnerNodeSPtr const & edge = knownTable_.GetNode(knownTable_.PredHoodEdge);
                    if (edge->IsAvailable)
                    {
                        site_.leaseAgentUPtr_->Terminate(edge->Instance.ToString());
                    }

                    knownTable_.ShrinkPredHoodRange();
                }

                while (knownTable_.CanShrinkSuccHoodRange())
                {
                    PartnerNodeSPtr const & edge = knownTable_.GetNode(knownTable_.SuccHoodEdge);
                    if (edge->IsAvailable)
                    {
                        site_.leaseAgentUPtr_->Terminate(edge->Instance.ToString());
                    }

                    knownTable_.ShrinkSuccHoodRange();
                }

                neighborhoodChanged = knownTable_.WithinHoodRange(nodeInfo.Instance.Id);
            }
            else
            {
                knownTable_.BreakCompleteRing(hoodSize_);
                neighborhoodChanged = true;
            }

            if (neighborhoodChanged && site_.Phase >= NodePhase::Inserting)
            {
                LEASE_DURATION_TYPE leaseTimeoutType;
                Common::TimeSpan leaseTimeout = site_.GetLeaseDuration(*newPtrAdded, leaseTimeoutType);

                site_.leaseAgentUPtr_->Establish(
                    newPtrAdded->Instance.ToString(),
                    newPtrAdded->NodeFaultDomainId.ToString(),
                    newPtrAdded->LeaseAgentAddress, 
                    newPtrAdded->LeaseAgentInstanceId,
                    leaseTimeoutType);
            }
        }

        if (neighborhoodChanged)
        {
            neighborhoodVersion_++;
        }

        // TODO: move logic to test code after code is stable.
        VerifyConsistency();

        // TODO: compact the ring when the ring exceed the capacity

        WriteInfo(TraceConsider, "{0} consider {1} result {2}", site_.Id, nodeInfo, newPtrAdded);

        return newPtrAdded;
    }

    PartnerNodeSPtr const& RoutingTable::InternalConsiderExternalNode(FederationPartnerNodeHeader const & nodeInfo)
    {
        auto it = externalRings_.find(nodeInfo.RingName);
        if (it == externalRings_.end())
        {
            return NullNode;
        }

        NodeRingBase & externalRing = it->second;
        size_t position = 0;
        size_t newPosition = 0;
        bool exist = false;

        if (externalRing.Size > 0)
        {
            position = externalRing.FindSuccOrSamePosition(nodeInfo.Instance.Id);
            PartnerNodeSPtr const& existingNode = externalRing.GetNode(position);
            if (existingNode->Id == nodeInfo.Instance.Id)
            {
                if (existingNode->Instance.InstanceId < nodeInfo.Instance.InstanceId)
                {
                    if (existingNode->Target)
                    {
                        existingNode->Target->TargetDown(existingNode->Instance.InstanceId);
                    }
                    externalRing.ReplaceNode(position, make_shared<PartnerNode>(nodeInfo, site_));
                }
                else if (existingNode->Phase < nodeInfo.Phase)
                {
                    externalRing.ReplaceNode(position, make_shared<PartnerNode>(existingNode, nodeInfo.Phase, nodeInfo.Token));
                }
                else
                {
                    if (existingNode->Instance.InstanceId != nodeInfo.Instance.InstanceId ||
                        existingNode->Phase != nodeInfo.Phase)
                    {
                        existingNode->UpdateLastAccess(Stopwatch::Now());
                    }
                    return existingNode;
                }

                newPosition = position;
                exist = true;
            }
        }

        if (!exist)
        {
            newPosition = externalRing.AddNode(make_shared<PartnerNode>(nodeInfo, site_), position);
        }

        PartnerNodeSPtr const& newPtrAdded = externalRing.GetNode(newPosition);
        WriteInfo(TraceConsider, "{0} consider {1} from {2} result {3}", site_.Id, nodeInfo, nodeInfo.RingName, newPtrAdded);

        return newPtrAdded;
    }

    NodeIdRange RoutingTable::GetHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        AcquireReadLock grab(lock_);

        return knownTable_.GetHood(vecNode);
    }

    NodeIdRange RoutingTable::GetHoodRange() const
    {
        AcquireReadLock grab(lock_);

        return knownTable_.GetRange();
    }

    NodeIdRange RoutingTable::GetCombinedNeighborHoodTokenRange() const
    {
        AcquireReadLock grab(lock_);

        NodeIdRange result = knownTable_.GetRange();
        PartnerNodeSPtr const & predEdge = knownTable_.GetNode(knownTable_.PredHoodEdge);
        if (predEdge->IsAvailable && !predEdge->Token.Range.IsEmpty && !predEdge->Token.Range.Disjoint(result))
        {
            result = NodeIdRange::Merge(result, predEdge->Token.Range);
        }

        PartnerNodeSPtr const & succEdge = knownTable_.GetNode(knownTable_.SuccHoodEdge);
        if (succEdge->IsAvailable && !succEdge->Token.Range.IsEmpty && !succEdge->Token.Range.Disjoint(result))
        {
            result = NodeIdRange::Merge(result, succEdge->Token.Range);
        }

        return result;
    }

    void RoutingTable::GetPingTargets(vector<PartnerNodeSPtr>& vecNode) const
    {
        AcquireReadLock grab(lock_);

        knownTable_.GetPingTargets(vecNode);

        if (predRecoveryTime_ != StopwatchTime::Zero && predProbeTarget_ != site_.Id)
        {
            PartnerNodeSPtr const & target = GetInternal(predProbeTarget_, &knownTable_);
            if (target)
            {
                vecNode.push_back(target);
            }
        }

        if (succRecoveryTime_ != StopwatchTime::Zero && succProbeTarget_ != site_.Id)
        {
            PartnerNodeSPtr const & target = GetInternal(succProbeTarget_, &knownTable_);
            if (target)
            {
                vecNode.push_back(target);
            }
        }
    }
    
    void RoutingTable::InternalGetExtendedHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        int predCount, succCount;
        if (static_cast<int>(ring_.Size) > hoodSize_ + hoodSize_)
        {
            predCount = succCount = hoodSize_;
        }
        else
        {
            predCount = static_cast<int>(ring_.Size) - 1;
            succCount = 0;
        }

        size_t index = ring_.ThisNode;
        for (int i = 0; i < predCount; i++)
        {
            index = ring_.GetPred(index);
            vecNode.push_back(ring_.GetNode(index));
        }

        index = ring_.ThisNode;
        for (int i = 0; i < succCount; i++)
        {
            index = ring_.GetSucc(index);
            vecNode.push_back(ring_.GetNode(index));
        }
    }

    void RoutingTable::GetExtendedHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        AcquireReadLock grab(lock_);
        InternalGetExtendedHood(vecNode);
    }

    PartnerNodeSPtr RoutingTable::GetPredecessor() const
    {
        AcquireReadLock grab(lock_);

        size_t index = ring_.ThisNode;
        index = ring_.GetPred(index);
        return ring_.GetNode(index);
    }

    PartnerNodeSPtr RoutingTable::GetSuccessor() const
    {
        AcquireReadLock grab(lock_);

        size_t index = ring_.ThisNode;
        index = ring_.GetSucc(index);
        return ring_.GetNode(index);
    }

    void RoutingTable::InternalExtendHood(NodeIdRange const& range, set<NodeInstance> const& shutdownNodes, set<NodeInstance> const* availableNodes, bool versionMatched)
    {
        bool predExtended = TryExtendPredHood(range, shutdownNodes, availableNodes, versionMatched);
        bool succExtended = TryExtendSuccHood(range, shutdownNodes, availableNodes, versionMatched);

        if (predExtended || succExtended)
        {
            Compact();
            neighborhoodVersion_++;
        }
    }

    bool RoutingTable::TryExtendPredHood(NodeIdRange const& range, set<NodeInstance> const& shutdownNodes, set<NodeInstance> const* availableNodes, bool versionMatched)
    {
        if (knownTable_.IsPredHoodComplete())
        {
            return false;
        }

        if (!versionMatched)
        {
            vector<PartnerNodeSPtr> vecNode;
            knownTable_.GetPredHood(vecNode);

            for (PartnerNodeSPtr const & node : vecNode)
            {
                if ((node->IsShutdown) &&
                    (shutdownNodes.find(node->Instance) == shutdownNodes.end()))
                {
                    if (knownTable_.PredHoodCount == 0)
                    {
                        WriteInfo(TraceEdge, "{0} unable to extend pred edge because down node {1} not found", site_.Id, node->Instance);
                    }

                    return false;
                }
            }
        }

        bool result = false;
        do
        {
            PartnerNodeSPtr const& newEdge = knownTable_.GetNode(knownTable_.GetPred(knownTable_.PredHoodEdge));
            if (newEdge->Id == site_.Id && !range.IsFull)
            {
                return result;
            }

            if (!range.Contains(NodeIdRange(newEdge->Id, versionMatched ? knownTable_.PredHoodEdgeId : site_.Id)))
            {
                return result;
            }

            if (newEdge->IsAvailable && availableNodes && availableNodes->find(newEdge->Instance) == availableNodes->end())
            {
                return result;
            }

            knownTable_.ExtendPredHoodEdge();
            result = true;

            if (!versionMatched && newEdge->IsShutdown && (shutdownNodes.find(newEdge->Instance) == shutdownNodes.end()))
            {
                return result;
            }

            if (newEdge->IsAvailable)
            {
                WriteInfo(TraceEdge, "{0} extended Pred Edge to {1}", site_.Id, newEdge->Instance);

                if (newEdge->Id != site_.Id && site_.Phase >= NodePhase::Inserting)
                {
                    LEASE_DURATION_TYPE leaseTimeoutType;
                    Common::TimeSpan leaseTimeout = site_.GetLeaseDuration(*newEdge, leaseTimeoutType);

                    site_.leaseAgentUPtr_->Establish(
                        newEdge->Instance.ToString(),
                        newEdge->NodeFaultDomainId.ToString(),
                        newEdge->LeaseAgentAddress,
                        newEdge->LeaseAgentInstanceId,
                        leaseTimeoutType);
                }
            }
        } while (!knownTable_.IsPredHoodComplete());

        return result;
    }

    bool RoutingTable::TryExtendSuccHood(NodeIdRange const& range, set<NodeInstance> const& shutdownNodes, set<NodeInstance> const* availableNodes, bool versionMatched)
    {
        if (knownTable_.IsSuccHoodComplete())
        {
            return false;
        }

        if (!versionMatched)
        {
            vector<PartnerNodeSPtr> vecNode;
            knownTable_.GetSuccHood(vecNode);

            for (PartnerNodeSPtr const & node : vecNode)
            {
                if ((node->IsShutdown) &&
                    (shutdownNodes.find(node->Instance) == shutdownNodes.end()))
                {
                    if (knownTable_.SuccHoodCount == 0)
                    {
                        WriteInfo(TraceEdge, "{0} unable to extend succ edge because down node {1} not found", site_.Id, node->Instance);
                    }

                    return false;
                }
            }
        }

        bool result = false;
        do
        {
            PartnerNodeSPtr const& newEdge = knownTable_.GetNode(knownTable_.GetSucc(knownTable_.SuccHoodEdge));
            if (newEdge->Id == site_.Id && !range.IsFull)
            {
                return result;
            }

            if (!range.Contains(NodeIdRange(versionMatched ? knownTable_.SuccHoodEdgeId : site_.Id, newEdge->Id)))
            {
                return false;
            }

            if (newEdge->IsAvailable && availableNodes && availableNodes->find(newEdge->Instance) == availableNodes->end())
            {
                return result;
            }

            knownTable_.ExtendSuccHoodEdge();
            result = true;

            if (!versionMatched && newEdge->IsShutdown && (shutdownNodes.find(newEdge->Instance) == shutdownNodes.end()))
            {
                return result;
            }

            if (newEdge->IsAvailable)
            {
                WriteInfo(TraceEdge, "{0} extended Succ Edge to {1}", site_.Id, newEdge->Instance);

                if (newEdge->Id != site_.Id && site_.Phase >= NodePhase::Inserting)
                {
                    LEASE_DURATION_TYPE leaseTimeoutType;
                    Common::TimeSpan leaseTimeout = site_.GetLeaseDuration(*newEdge, leaseTimeoutType);

                    site_.leaseAgentUPtr_->Establish(
                        newEdge->Instance.ToString(),
                        newEdge->NodeFaultDomainId.ToString(),
                        newEdge->LeaseAgentAddress,
                        newEdge->LeaseAgentInstanceId,
                        leaseTimeoutType);
                }
            }
        } while (!knownTable_.IsSuccHoodComplete());

        return result;
    }

    void RoutingTable::SetShutdown(NodeInstance const& shutdownNode, wstring const & ringName)
    {
        WriteLock grab(*this);
        InternalSetShutdown(shutdownNode, ringName);
    }

    void RoutingTable::InternalSetShutdown(NodeInstance const& shutdownNode, wstring const & ringName)
    {
        // first check the known table
        PartnerNodeSPtr const& knownPtr = GetInternal(shutdownNode.Id, ringName);
        if (knownPtr && (knownPtr->Instance.InstanceId <= shutdownNode.InstanceId) && !knownPtr->IsShutdown)
        {
            SetShutdown(knownPtr);
        }
    }

    void RoutingTable::SetShutdown(PartnerNodeSPtr const & shutdownNode)
    {
        WriteInfo(TraceDown, "{0} set {1} to shutdown", site_.Id, *shutdownNode);
        InternalConsider(FederationPartnerNodeHeader(shutdownNode->Instance, NodePhase::Shutdown, shutdownNode->Address, shutdownNode->LeaseAgentAddress, shutdownNode->LeaseAgentInstanceId, shutdownNode->Token, shutdownNode->NodeFaultDomainId, false, shutdownNode->RingName, shutdownNode->Flags));
    }

    void RoutingTable::SetUnknown(NodeInstance const& unknownNode, wstring const & ringName)
    {
        AcquireReadLock grab(lock_);
        PartnerNodeSPtr const& knownPtr = GetInternal(unknownNode.Id, ringName);
        if (knownPtr && knownPtr->Instance.InstanceId == unknownNode.InstanceId)
        {
            knownPtr->SetAsUnknown();
        }
    }

    void RoutingTable::SetUnknown(wstring const& unreachableAddress)
    {
        AcquireReadLock grab(lock_);

        WriteInfo(TraceUnknown, "Address {0} becomes unknown on {1}", unreachableAddress, site_.Id);

        knownTable_.SetUnknown(unreachableAddress);
    }

    void RoutingTable::TestCompact()
    {
        if (knownTable_.CompleteHoodRange)
        {
            return;
        }

        WriteLock grab(*this);

        vector<PartnerNodeSPtr> vecPredHood;
        vector<PartnerNodeSPtr> vecSuccHood;
        knownTable_.GetPredHood(vecPredHood);
        knownTable_.GetSuccHood(vecSuccHood);

        knownTable_.Clear();
        ring_.Clear();

        for (size_t i = 0; i < vecPredHood.size(); i++)
        {
            PartnerNodeSPtr node = vecPredHood[i];
            knownTable_.AddNode(node);
            if (i == 0)
            {
                knownTable_.ExtendPredHoodEdge();
            }

            if (node->IsAvailable)
            {
                ring_.AddNode(node);
            }
        }
        for (size_t i = 0; i < vecSuccHood.size(); i++)
        {
            PartnerNodeSPtr node = vecSuccHood[i];
            knownTable_.AddNode(node);
            if (i == 0)
            {
                knownTable_.ExtendSuccHoodEdge();
            }

            if (node->IsAvailable)
            {
                ring_.AddNode(node);
            }
        }
    }

    bool RoutingTable::ChangePhase(NodePhase::Enum phase)
    {
        WriteLock grab(*this);

        if (site_.Phase == phase)
        {
            return false;
        }

        site_.Phase = phase;

        if (phase >= NodePhase::Inserting)
        {
            implicitLeaseContext_.Start();
        }

        return true;
    }

    void RoutingTable::RestartInstance()
    {
        WriteLock grab(*this);

        site_.IncrementInstanceId();
        site_.Phase = NodePhase::Joining;
        site_.SetLeaseAgentInstanceId(site_.GetLeaseAgent().InstanceId);
    }

    void RoutingTable::Bootstrap()
    {
        WriteLock grab(*this);
        isRangeConsistent_ = true;
        knownTable_.SetCompleteKnowledge();
        site_.SetPhase(NodePhase::Routing);
        site_.GetTokenForUpdate().Update(NodeIdRange::Full);
        implicitLeaseContext_.Start();
    }

    void RoutingTable::StartRouting()
    {
		WriteLock grab(*this);

        isRangeConsistent_ = true;
		CheckHealth();
	}

    void RoutingTable::SetInitialLeasePartners(vector<PartnerNodeSPtr> const & partners)
    {
        WriteLock grab(*this);

        for (PartnerNodeSPtr const & partner : partners)
        {
            initialLeasePartners_.push_back(partner->Instance);
        }
    }

    void RoutingTable::CheckInitialLeasePartners()
    {
        for (NodeInstance const & node : initialLeasePartners_)
        {
            if (!knownTable_.WithinHoodRange(node.Id))
            {
                WriteInfo("Lease",
                    "{0} terminating initial lease partner {1}",
                    site_.Id, node);
                site_.leaseAgentUPtr_->Terminate(node.ToString());
            }
        }

        initialLeasePartners_.clear();
    }

    //Should always be called under Lock
    void RoutingTable::OnNeighborhoodChanged(bool healthCheckNeeded)
    {
        WriteInfo(TraceNeighbor, site_.IdString,
            "Neighborhood changed to {0}:{1}",
            knownTable_, neighborhoodVersion_);

         implicitLeaseContext_.Update(
            knownTable_.SuccHoodCount > 0 ? ring_.Next : knownTable_.ThisNodePtr,
            knownTable_.PredHoodCount > 0 ? ring_.Prev : knownTable_.ThisNodePtr);

        if (healthCheckNeeded)
        {
            CheckHealth();
        }

        if (initialLeasePartners_.size() > 0 &&
            knownTable_.IsPredHoodComplete() &&
            knownTable_.IsSuccHoodComplete())
        {
            CheckInitialLeasePartners();
        }

        site_.OnNeighborhoodChanged();
    }

    //Should always be called under Lock
    void RoutingTable::OnRoutingTokenChanged(bool tokenVersionChanged)
    {
        WriteTokenInfo(TraceUpdate, site_.IdString,
            "{0} token changed to {1}",
            site_.Id, site_.Token);

        if (!tokenVersionChanged)
        {
            return;
        }

        lastSuccTokenProbe_ = StopwatchTime::Zero;
        lastPredTokenProbe_ = StopwatchTime::Zero;

        CheckHealth();

        auto rootSPtr = site_.GetSiteNodeSPtr();
        Threadpool::Post([rootSPtr] { rootSPtr->OnRoutingTokenChanged(); });
    }

    void RoutingTable::CloseSiteNode()
    {
        WriteLock grab(*this);

        timer_->Cancel();

        PartnerNodeSPtr const& ptr = knownTable_.ThisNodePtr;
        PartnerNodeSPtr newPtr = make_shared<PartnerNode>(*ptr);
        knownTable_.ReplaceNode(knownTable_.ThisNode, newPtr);
        ring_.ReplaceNode(ring_.ThisNode, newPtr);
    }

    PartnerNodeSPtr RoutingTable::ProcessNeighborHeaders(Message & message, NodeInstance const & from, wstring const & fromRing, bool instanceMatched)
    {
        PartnerNodeSPtr fromNode;

        //TODO: possible to always transfer one token only?
        FederationRoutingTokenHeader incomingTokens[2];
        vector<FederationRoutingTokenHeader> rejectedTokens;
        bool accepted[2];
        int incomingTokenCount = 0;
        StopwatchTime now = Stopwatch::Now();

        WriteLock grab(*this);

        bool isInserting = (message.Action == FederationMessage::GetUnlockRequest().Action);

        set<NodeInstance> shutdownNodes;
        set<NodeInstance> availableNodes;
        FederationNeighborhoodRangeHeader neighborhoodRange;
        bool rangeFound = false;
        bool versionMatched = false;
        bool NeighborHeadersIgnored = false;

        for (auto it = message.Headers.begin(); it != message.Headers.end();)
        {
            bool remove = false;
            if (it->Id() == MessageHeaderId::FederationPartnerNode)
            {
                FederationPartnerNodeHeader header;
                if (!it->TryDeserialize<FederationPartnerNodeHeader>(header))
                {
                    return nullptr;
                }

                bool isFromNode = (header.Instance.Id == from.Id);
                PartnerNodeSPtr const & node = InternalConsider(header, isInserting && isFromNode, now.Ticks);
                if (!node)
                {
                    NeighborHeadersIgnored = true;
                }
                else if (isFromNode)
                {
                    fromNode = node;
                }

                if (header.Phase == NodePhase::Shutdown)
                {
                    shutdownNodes.insert(header.Instance);
                }
                else if (header.Phase == NodePhase::Inserting || header.Phase == NodePhase::Routing)
                {
                    availableNodes.insert(header.Instance);
                }

                remove = !header.IsEndToEnd();
            }
            else if (it->Id() == MessageHeaderId::FederationNeighborhoodRange && instanceMatched)
            {
                if (!it->TryDeserialize<FederationNeighborhoodRangeHeader>(neighborhoodRange))
                {
                    return nullptr;
                }

                ASSERT_IF(rangeFound, "multiple range headers found in msg");
                rangeFound = true;
            }
            else if (it->Id() == MessageHeaderId::FederationNeighborhoodVersion && instanceMatched)
            {
                FederationNeighborhoodVersionHeader versionHeader;
                if (!it->TryDeserialize<FederationNeighborhoodVersionHeader>(versionHeader))
                {
                    return nullptr;
                }

                versionMatched = (versionHeader.Version == neighborhoodVersion_);
            }
            else if (it->Id() == MessageHeaderId::FederationRoutingToken && instanceMatched)
            {
                if(!site_.IsRouting)
                {
                    WriteTokenInfo(TraceReceive,
                        "{0} received token from {1} in {2} stage",
                        site_.Id, from.Id, site_.Phase);
                }

                ASSERT_IF(incomingTokenCount >= 2,
                    "more than 2 tokens transferred in {0} message",
                    message.Action);

                if (!it->TryDeserialize<FederationRoutingTokenHeader>(incomingTokens[incomingTokenCount++]))
                {
                    return nullptr;
                }
            }

            if (remove)
            {
                message.Headers.Remove(it);
            }
            else
            {
                ++it;
            }
        }

        if (NeighborHeadersIgnored)
        {
            message.AddProperty(true, Constants::NeighborHeadersIgnoredKey);
        }

        if (rangeFound)
        {
            if (versionMatched)
            {
                vector<NodeInstance> nodes;
                ring_.GetNodesInRange(knownTable_.GetRange(), nodes);

                for (NodeInstance const & node : nodes)
                {
                    if (neighborhoodRange.Range.Contains(node.Id) &&
                        availableNodes.find(node) == availableNodes.end() &&
                        node.Id != site_.Id)
                    {
                        Trace.WriteInfo(TraceDown, 
                            "{0} set {1} as shutdown with range {2} from {3}",
                            site_.Id, node, neighborhoodRange, from);

                        InternalSetShutdown(node, site_.RingName);
                    }
                }
            }

            InternalExtendHood(neighborhoodRange.Range, shutdownNodes, &availableNodes, versionMatched);
        }

        if (incomingTokenCount > 0 && (!site_.Token.IsEmpty || now < tokenAcquireDeadline_))
        {
            RoutingToken & token = site_.GetTokenForUpdate();

            for (int i = 0; i < incomingTokenCount; i++)
            {
                RoutingToken oldToken(site_.Token);
                accepted[i] =  token.Accept(incomingTokens[i].Token, site_.Id);

                if (accepted[i])
                {
                    WriteTokenInfo(TraceAccept,
                        "{0} accepted token {1} and update token from {2} to {3}",
                        site_.Id, incomingTokens[i], oldToken, token);
                }
                else
                {
                    WriteTokenInfo(TraceReject,
                        "{0} with token {1} rejected token {2} from {3}",
                        site_.Id, oldToken, incomingTokens[i], from);
                }
            }
        }

        if (!fromNode)
        {
            fromNode = GetInternal(from.Id, fromRing);
        }

        if (!fromNode)
        {
            //Not having an entry for the FromNode in the routing table will be a valid scenario when we actually have Compact() method
            if (incomingTokenCount > 0)
            {
                WriteTokenInfo(TraceReject,
                    "Node {0} which sent a TokenTransfer message is not in the routing table anymore so droping reply", 
                    from.Id);
            }

            return nullptr;
        }

        if (incomingTokenCount > 0 && message.Action == FederationMessage::GetTokenTransfer().Action)
        {
            if (!accepted[0] || (incomingTokenCount == 2 && !accepted[1]))
            {
                if (!accepted[0])
                {
                    rejectedTokens.push_back(incomingTokens[0]);
                }

                if (incomingTokenCount == 2 && !accepted[1])
                {
                    rejectedTokens.push_back(incomingTokens[1]);
                }

                message.AddProperty(rejectedTokens, Constants::RejectedTokenKey);
            }
        }

        return fromNode;
    }

    PartnerNodeSPtr RoutingTable::ProcessNodeHeaders(Message & message, NodeInstance const & from, wstring const & fromRing)
    {
        PartnerNodeSPtr fromNode;
        FederationPartnerNodeHeader header;
        bool found = false;

        MessageHeaders::HeaderIterator it = message.Headers.begin();
        for (; !found && it != message.Headers.end(); ++it)
        {
            if (it->Id() == MessageHeaderId::FederationPartnerNode)
            {
                if (!it->TryDeserialize<FederationPartnerNodeHeader>(header))
                {
                    return nullptr;
                }

                found = true;
            }
        }

        {
            bool needUpdate = false;

            AcquireReadLock grab(lock_);

            while (!needUpdate && (found || it != message.Headers.end()))
            {
                if (!found)
                {
                    if (it->Id() == MessageHeaderId::FederationPartnerNode)
                    {
                        if (!it->TryDeserialize<FederationPartnerNodeHeader>(header))
                        {
                            return nullptr;
                        }

                        found = true;
                    }

                    ++it;
                }

                if (found)
                {
                    PartnerNodeSPtr const & node = GetInternal(header.Instance.Id, fromRing);
                    if (!node || node->Instance.InstanceId < header.Instance.InstanceId)
                    {
                        needUpdate = true;
                    }
                    else if ((node->Instance.InstanceId == header.Instance.InstanceId) &&
                             ((node->Phase < header.Phase) ||
                              (node->Token.Version < header.Token.Version)))
                    {
                        needUpdate = true;
                    }
                    else if (header.Instance.Id == from.Id)
                    {
                        fromNode = node;
                    }

                    found = false;
                }
            }

            if (!needUpdate)
            {
                if (!fromNode)
                {
                    fromNode = GetInternal(from.Id, fromRing);
                }

                return fromNode;
            }
        }

        StopwatchTime now = Stopwatch::Now();

        WriteLock grab(*this);

        PartnerNodeSPtr const & updateNode = InternalConsider(header, false, now.Ticks);
        if (header.Instance.Id == from.Id)
        {
            fromNode = updateNode;
        }

        for (; it != message.Headers.end(); ++it)
        {
            if (it->Id() == MessageHeaderId::FederationPartnerNode)
            {
                if (!it->TryDeserialize<FederationPartnerNodeHeader>(header))
                {
                    return nullptr;
                }

                PartnerNodeSPtr const & node = InternalConsider(header, false, now.Ticks);
                if (header.Instance.Id == from.Id)
                {
                    fromNode = node;
                }
            }
        }

        if (!fromNode)
        {
            fromNode = GetInternal(from.Id, fromRing);
        }

        return fromNode;
    }

    bool RoutingTable::LockRange(NodeInstance const & owner, PartnerNodeSPtr const & node, StopwatchTime expiryTime, NodeIdRange & range, FederationPartnerNodeHeader & ownerHeader)
    {
        WriteLock grab(*this);

        node->UpdateLastAccess(Stopwatch::Now());

        if (owner.Id != site_.Id && rangeLockExpiryTime_ > Stopwatch::Now())
        {
            PartnerNodeSPtr ownerSPtr = GetInternal(owner);
            if (ownerSPtr)
            {
                ownerHeader = FederationPartnerNodeHeader(*ownerSPtr);
                return false;
            }
        }

        if (!knownTable_.CompleteHoodRange &&
            (knownTable_.SuccHoodCount <= safeHoodSize_ || knownTable_.PredHoodCount <= safeHoodSize_))
        {
            ownerHeader = FederationPartnerNodeHeader(site_);
            return false;
        }

        rangeLockExpiryTime_ = expiryTime;
        range = knownTable_.GetRange();

        return true;
    }

    void RoutingTable::ReleaseRangeLock()
    {
        WriteLock grab(*this);

        rangeLockExpiryTime_ = StopwatchTime::Zero;
    }

    void RoutingTable::UpdateNeighborhoodExchangeInterval(TimeSpan interval)
    {
        knownTable_.NeighborhoodExchangeInterval = interval;
    }

    void RoutingTable::AddNeighborHeaders(Message & message, bool isRingToRing)
    {
        bool addNeighborHeaders = FederationMessage::IsFederationMessage(message) && !site_.IsLeaseExpired();

		// Avoid taking routing table lock when possible
		if (!addNeighborHeaders && site_.Phase == NodePhase::Routing && !message.Headers.Contains(MessageHeaderId::BroadcastRelatesTo))
		{
			message.Headers.Add(FederationPartnerNodeHeader(site_.Instance, NodePhase::Routing, site_.Address, site_.LeaseAgentAddress, site_.LeaseAgentInstanceId, RoutingToken(), site_.NodeFaultDomainId, true, site_.RingName, site_.Flags));
			return;
		}

        AcquireReadLock grab(lock_);
        if (addNeighborHeaders)
        {
            bool addRangeHeader;
            bool addExtendedNeighborhood;
            bool addFullNeighborhood;
            
            if (isRingToRing)
            {
                addRangeHeader = addExtendedNeighborhood = addFullNeighborhood = false;
            }
            else
            {
                addRangeHeader = isRangeConsistent_;
                if (rangeLockExpiryTime_ > StopwatchTime::Zero)
                {
                    if (rangeLockExpiryTime_ < Stopwatch::Now())
                    {
                        rangeLockExpiryTime_ = StopwatchTime::Zero;
                    }
                    else
                    {
                        addRangeHeader = false;
                    }
                }

                addExtendedNeighborhood = (
                    !isRangeConsistent_ ||
                    message.Action == FederationMessage::GetLockTransferReply().Action ||
                    message.Action == FederationMessage::GetEdgeProbe().Action);
                addFullNeighborhood = (addExtendedNeighborhood ?
                    (ring_.Size < hoodSize_ + hoodSize_) : knownTable_.CompleteHoodRange);
            }

            knownTable_.AddNeighborHeaders(message, addRangeHeader && !site_.IsShutdown, addExtendedNeighborhood && !site_.IsShutdown, addFullNeighborhood);
        }

        message.Headers.Add(FederationPartnerNodeHeader(*ring_.ThisNodePtr, true));
    }

    void RoutingTable::WriteTo(Common::TextWriter& w, Common::FormatOptions const& option) const
    {
        if (option.formatString == "l")
        {
            w.Write("Up:\t{0:l}\nAll:\t{1:l}\nGlobal Time: {2}",
                ring_, knownTable_, globalTimeManager_);
        }
        else
        {
            w.Write("Up:\t{0}\nAll:\t{1}", ring_, knownTable_);
        }

        for (auto it = externalRings_.begin(); it != externalRings_.end(); ++it)
        {
            w.Write("\nRing {0}: {1}", it->first, it->second); 
        }
    }

    void RoutingTable::VerifyConsistency() const
    {
        ring_.VerifyConsistency(site_.Id);
        knownTable_.VerifyConsistency(site_.Id);
    }

#pragma region Token Related methods

    //This method should be called under routing table lock
    void RoutingTable::TrySplitToken(PartnerNodeSPtr const & neighbor, vector<FederationRoutingTokenHeader> & tokens)
    {
        RoutingToken oldToken(site_.Token);
        Split(neighbor, site_.Id, tokens, neighbor->Token.Version);

        if (oldToken.Version != site_.Token.Version)
        {
            WriteTokenInfo(TraceSplit,
                "{0} split token {1} to {2} for {3}",
                site_.Id, oldToken, site_.Token, neighbor->Id);
        }
    }

    void RoutingTable::CreateDepartMessages(vector<Transport::MessageUPtr> & messages, vector<PartnerNodeSPtr> & targets)
    {
        WriteLock grab(*this);

        knownTable_.GetHood(targets);

        for (auto it = targets.begin(); it != targets.end();)
        {
            if ((*it)->IsRouting)
            {
                MessageUPtr departMessage = FederationMessage::GetDepart().CreateMessage();
                TryReleaseToken(*departMessage, *it);

                messages.push_back(move(departMessage));
                ++it;
            }
            else
            {
                it = targets.erase(it);
            }
        }

        if (!site_.Token.IsEmpty)
        {
            site_.GetTokenForUpdate().SetEmpty();
        }
    }

    void RoutingTable::TryReleaseToken(Message & message, PartnerNodeSPtr const & partner)
    {
        if (partner->Id == ring_.Next->Id)
        {
            FederationRoutingTokenHeader tokenToBePassed = site_.GetTokenForUpdate().ReleaseSuccToken(ring_.Prev->Id, site_.Id, ring_.Next);
            if (!tokenToBePassed.IsEmpty)
            {
                message.Headers.Add(tokenToBePassed);
                WriteTokenInfo(TraceRelease,
                    "{0} releasing token successor {1} to {2}",
                    site_.Id, tokenToBePassed, partner->Id);
            }
        }

        if (partner->Id == ring_.Prev->Id)
        {
            FederationRoutingTokenHeader tokenToBePassed = site_.GetTokenForUpdate().ReleasePredToken(ring_.Prev, site_.Id, ring_.Next->Id);
            if (!tokenToBePassed.IsEmpty)
            {
                message.Headers.Add(tokenToBePassed);
                WriteTokenInfo(TraceRelease,
                    "{0} releasing token predecessor {1} to {2}",
                    site_.Instance, tokenToBePassed, partner->Id);
            }
        }
    }

#pragma endregion Token Related methods

    void RoutingTable::OnTimer()
    {
        WriteLock grab(*this);
        CheckHealth();

        StopwatchTime now = Stopwatch::Now();
        FederationConfig const & config = FederationConfig::GetConfig();
        if (site_.IsRouting && now > lastTraceTime_ + config.PeriodicTraceInterval)
        {
            WriteInfo(TraceState, site_.IdString, "IsHealthy: {0}, token: {1}, neighbors: {2}:{3}, size={4},{5}",
                isHealthy_, site_.Token, knownTable_, neighborhoodVersion_, ring_.Size, knownTable_.Size);
            lastTraceTime_ = now;
        }
    }

    void RoutingTable::CheckHealth()
    {
        if (!site_.IsRouting || isNeighborLostReported_)
        {
            return;
        }

        StateMachineActionCollection actions;

        StopwatchTime nextCheck = CheckSuccEdge(actions);
        nextCheck = min(nextCheck, CheckPredEdge(actions));
        nextCheck = min(nextCheck, CheckToken(actions));
        nextCheck = min(nextCheck, CheckInsertingNode(actions));

        if (nextCheck == StopwatchTime::MaxValue)
        {
            if (!isHealthy_)
            {
                WriteInfo(TraceHealth, "{0} routing table is now healthy", site_.Id);
                isHealthy_ = true;
            }
        }
        else
        {
            isHealthy_ = false;
        }

        StopwatchTime now = Stopwatch::Now();
        FederationConfig const & config = FederationConfig::GetConfig();

        nextCheck = min(nextCheck, CheckLivenessUpdateAction(actions));
        nextCheck = min(nextCheck, now + config.RoutingTableHealthCheckInterval);

        for (auto it = externalRings_.begin(); it != externalRings_.end(); ++it)
        {
            it->second.CheckHealth(actions);
        }

        implicitLeaseContext_.RunStateMachine(actions);

        actions.Execute(site_);

        globalTimeManager_.CheckHealth();

        Compact();

        if (!isHealthy_)
        {
            WriteInfo(TraceHealth, "{0} schedule next health check at {1}, current size={2}, neighborhood={3}", site_.Id, nextCheck, knownTable_.Size, knownTable_.GetHoodCount());
        }

        timer_->ChangeWithLowerBoundDelay(nextCheck - Stopwatch::Now());
    }

    void RoutingTable::OnNeighborhoodLost()
    {
        if (!isNeighborLostReported_)
        {
            shared_ptr<SiteNode> site = site_.GetSiteNodeSPtr();
            Threadpool::Post([site] { site->OnNeighborhoodLost(); });
            isNeighborLostReported_ = true;
        }
    }

    void RoutingTable::AddEdgeProbeAction(StateMachineActionCollection & actions,
        PartnerNodeSPtr const & target, bool isSuccDirection)
    {
        int neighborhoodCount = (isSuccDirection ? knownTable_.SuccHoodCount : knownTable_.PredHoodCount);
        auto message = FederationMessage::GetEdgeProbe().CreateMessage(EdgeProbeMessageBody(neighborhoodVersion_, isSuccDirection, neighborhoodCount));
        actions.Add(make_unique<SendMessageAction>(move(message), target));
    }

    void RoutingTable::CheckGapRecovery(NodeIdRange const & range)
    {
        StateMachineActionCollection actions;
        {
            vector<NodeId> pendingVotes;
            bool completed = site_.GetVoteManager().CheckGapRecovery(range, pendingVotes);

            WriteLock grab(*this);
            if (range.Begin == site_.Id)
            {
                if (succRecoveryTime_ == StopwatchTime::Zero ||
                    knownTable_.SuccHoodCount > 0 ||
                    succProbeTarget_ != range.End)
                {
                    return;
                }
            }
            else if (range.End == site_.Id)
            {
                if (predRecoveryTime_ == StopwatchTime::Zero ||
                    knownTable_.PredHoodCount > 0 ||
                    predProbeTarget_ != range.Begin)
                {
                    return;
                }
            }
            else
            {
                return;
            }

            if (completed)
            {
                if (range.Begin == site_.Id)
                {
                    while (range.ProperContains(ring_.Next->Id))
                    {
                        PartnerNodeSPtr const & next = ring_.Next;
                        SetShutdown(next);
                    }

                    InternalExtendHood(range, set<NodeInstance>(), nullptr, true);

                    WriteInfo(TraceGap,
                        "Successor gap {0} recovered: {1}",
                        range, *this);

                    succRecoveryTime_ = StopwatchTime::Zero;
                    succProbeTarget_ = site_.Id;
                    lastSuccEdgeProbe_ = StopwatchTime::Zero;

                    ReportNeighborhoodRecovered();
                }
                else
                {
                    while (range.ProperContains(ring_.Prev->Id))
                    {
                        PartnerNodeSPtr const & prev = ring_.Prev;
                        SetShutdown(prev);
                    }

                    InternalExtendHood(range, set<NodeInstance>(), nullptr, true);

                    WriteInfo(TraceGap,
                        "Predecessor gap {0} recovered: {1}",
                        range, *this);

                    predRecoveryTime_ = StopwatchTime::Zero;
                    predProbeTarget_ = site_.Id;
                    lastPredEdgeProbe_ = StopwatchTime::Zero;

                    ReportNeighborhoodRecovered();
                }
            }
            else if (range.Begin == site_.Id)
            {
                for (NodeId vote : pendingVotes)
                {
                    MessageUPtr message = FederationMessage::GetTicketGapRequest().CreateMessage(TicketGapRequest(range));
                    actions.Add(make_unique<GapRequestAction>(move(message), vote));
                }
            }
        }

        actions.Execute(site_);
    }

    StopwatchTime RoutingTable::CheckSuccEdge(StateMachineActionCollection & actions)
    {
        FederationConfig & config = FederationConfig::GetConfig();

        StopwatchTime nextProbe = lastSuccEdgeProbe_ + config.EdgeProbeInterval;
        StopwatchTime now = Stopwatch::Now();
        bool canStartNextProbe = (now > nextProbe);

        if (canStartNextProbe)
        {
            if (knownTable_.SuccHoodCount == 0 && ackedProbe_ != site_.Id)
            {
                if (probeResult_)
                {
                    succRecoveryTime_ = now;
                    succProbeTarget_ = ackedProbe_;

                    WriteWarning(TraceGap,
                        "{0} start recovery on successor side with {1}",
                        site_.Id, succProbeTarget_);
                }
                else
                {
                    succProbeTarget_ = site_.Id;

                    WriteInfo(TraceEdgeProbe,
                        "{0} reset successor probe with {1}",
                        site_.Id, ackedProbe_);
                }
            }

            ackedProbe_ = site_.Id;
            probeResult_ = false;
        }

        if (succRecoveryTime_ != StopwatchTime::Zero)
        {
            if (knownTable_.SuccHoodCount > 0)
            {
                WriteInfo(TraceGap,
                    "Successor gap {0}-{1} removed: {2}",
                    site_.Id, succProbeTarget_, *this);

                ReportNeighborhoodRecovered();

                succRecoveryTime_ = StopwatchTime::Zero;
                succProbeTarget_ = site_.Id;
            }
            else
            {
                ReportNeighborhoodLost(wformatString("{0}-{1}", site_.Id, succProbeTarget_));
                actions.Add(make_unique<GapCheckAction>(NodeIdRange(site_.Id, succProbeTarget_)));

                return max(succRecoveryTime_ + config.GlobalTicketLeaseDuration, now + config.EdgeProbeInterval);
            }
        }

        if (knownTable_.IsSuccHoodComplete())
        {
            lastSuccEdgeProbe_ = StopwatchTime::Zero;
            return StopwatchTime::MaxValue;
        }

        if (!canStartNextProbe)
        {
            return nextProbe;
        }

        if (knownTable_.SuccHoodCount == 0)
        {
            size_t index = ring_.FindSuccOrSamePosition(succProbeTarget_);
            if (ring_.GetNode(index)->Id == succProbeTarget_)
            {
                index = ring_.GetSucc(index);
            }

            if (index == ring_.ThisNode)
            {
                OnNeighborhoodLost();
                return StopwatchTime::MaxValue;
            }

            for (int i = 0; i < hoodSize_; i++)
            {
                PartnerNodeSPtr const & target = ring_.GetNode(index);
                if (target->Id == site_.Id)
                {
                    break;
                }

                WriteInfo(TraceEdgeProbe, "{0} succ probe target {1}/{2}, ring\r\n{3}",
                    site_.Id, target->Id, succProbeTarget_, ring_);

                succProbeTarget_ = target->Id;
                AddEdgeProbeAction(actions, target, true);

                index = ring_.GetSucc(index);
            }
        }
        else
        {
            succProbeTarget_ = site_.Id;

            size_t index = knownTable_.SuccHoodEdge;
            bool found = false;
            while (!found)
            {
                ASSERT_IF(index == knownTable_.ThisNode,
                    "{0} no succ edge probe target found: {1}",
                    site_.Id, *this);
                    
                PartnerNodeSPtr const & target = knownTable_.GetNode(index);
                if (target->IsAvailable)
                {
                    AddEdgeProbeAction(actions, target, true);
                    found = true;
                }
                else
                {
                    index = knownTable_.GetPred(index);
                }
            }
        }

        lastSuccEdgeProbe_ = now;
        return now + config.EdgeProbeInterval;
    }

    StopwatchTime RoutingTable::CheckPredEdge(StateMachineActionCollection & actions)
    {
        FederationConfig & config = FederationConfig::GetConfig();
        StopwatchTime now = Stopwatch::Now();

        if (predRecoveryTime_ != StopwatchTime::Zero)
        {
            if (knownTable_.PredHoodCount > 0)
            {
                WriteInfo(TraceGap,
                    "Predecessor gap {0}-{1} removed: {2}",
                    predProbeTarget_, site_.Id, *this);

                ReportNeighborhoodRecovered();

                predRecoveryTime_ = StopwatchTime::Zero;
                predProbeTarget_ = site_.Id;
            }
            else
            {
                PartnerNodeSPtr const & predTarget = GetInternal(predProbeTarget_, site_.RingName);
                if (predTarget && predTarget->IsAvailable)
                {
                    actions.Add(make_unique<GapCheckAction>(NodeIdRange(predProbeTarget_, site_.Id)));
				}
                else
                {
                    predRecoveryTime_ = StopwatchTime::Zero;
                    predProbeTarget_ = site_.Id;
                }
            }
        }

        if (knownTable_.IsPredHoodComplete())
        {
            lastPredEdgeProbe_ = StopwatchTime::Zero;
            return StopwatchTime::MaxValue;
        }

        StopwatchTime nextProbe = lastPredEdgeProbe_ + config.EdgeProbeInterval;
        if (nextProbe > now)
        {
            return nextProbe;
        }

        if (knownTable_.PredHoodCount == 0)
        {
            size_t index = ring_.FindSuccOrSamePosition(predProbeTarget_);
            if (predRecoveryTime_ == StopwatchTime::Zero || ring_.GetNode(index)->Id != predProbeTarget_)
            {
                index = ring_.GetPred(index);
            }

            if (index == ring_.ThisNode)
            {
                OnNeighborhoodLost();
                return StopwatchTime::MaxValue;
            }

            PartnerNodeSPtr const & target = ring_.GetNode(index);
            WriteInfo(TraceEdgeProbe, "{0} pred probe target {1}/{2}, ring\r\n{3}",
                site_.Id, target->Id, predProbeTarget_, ring_);

            predProbeTarget_ = target->Id;

            AddEdgeProbeAction(actions, target, false);
        }
        else
        {
            predProbeTarget_ = site_.Id;

            size_t index = knownTable_.PredHoodEdge;
            bool found = false;
            while (!found)
            {
                ASSERT_IF(index == knownTable_.ThisNode,
                    "{0} no pred edge probe target found: {1}",
                    site_.Id, *this);
                    
                PartnerNodeSPtr const & target = knownTable_.GetNode(index);
                if (target->IsAvailable)
                {
                    AddEdgeProbeAction(actions, target, false);
                    found = true;
                }
                else
                {
                    index = knownTable_.GetSucc(index);
                }
            }
        }

        lastPredEdgeProbe_ = now;
        return lastPredEdgeProbe_ + config.EdgeProbeInterval;
    }

    void RoutingTable::EdgeProbeOneWayMessageHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        NodeId from = oneWayReceiverContext.From->Id;
        oneWayReceiverContext.Accept();

        EdgeProbeMessageBody body;
        if (!message.GetBody<EdgeProbeMessageBody>(body))
        {
            return;
        }

        WriteInfo(TraceEdgeProbe, "{0} received EdgeProbe {1} from {2}",
            site_.Id, body, from);

        if (body.GetNeighborhoodCount() <= safeHoodSize_)
        {
            site_.joinLockManagerUPtr_->ResetLock();
        }

        int neighborhoodCount;

        {
            WriteLock grab(*this);
            if (body.IsSuccDirection())
            {
                neighborhoodCount = knownTable_.PredHoodCount;
                bool start = false;

                if (predRecoveryTime_ != StopwatchTime::Zero)
                {
                    LargeInteger dist = site_.Id.PredDist(from);
                    LargeInteger current = site_.Id.PredDist(predProbeTarget_);
                    if (dist < current)
                    {
                        start = true;
                    }
                }
                else if (neighborhoodCount == 0 && body.GetNeighborhoodCount() == 0)
                {
                    start = true;
                }

                if (start)
                {
                    predRecoveryTime_ = Stopwatch::Now();
                    predProbeTarget_ = from;

                    WriteWarning(TraceGap,
                        "{0} start recovery on predecessor side with {1}",
                        site_.Id, predProbeTarget_);
                }
            }
            else
            {
                neighborhoodCount = knownTable_.SuccHoodCount;
            }
        }

        auto reply = FederationMessage::GetRingAdjust().CreateMessage(EdgeProbeMessageBody(body.GetVersion(), body.IsSuccDirection(), neighborhoodCount));

        if (!message.TryGetProperty<bool>(Constants::NeighborHeadersIgnoredKey))
        {
            reply->Headers.Add(FederationNeighborhoodVersionHeader(body.GetVersion()));
        }
        else
        {
            Trace.WriteInfo(TraceEdgeProbe, "{0} can't acknowledge {1} from {2}",
                site_.Id, body.GetVersion(), from);
        }

        site_.Send(move(reply), oneWayReceiverContext.From);
    }

    void RoutingTable::RingAdjustOneWayMessageHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        NodeId from = oneWayReceiverContext.From->Id;
        oneWayReceiverContext.Accept();

        EdgeProbeMessageBody body;
        if (!message.GetBody<EdgeProbeMessageBody>(body))
        {
            return;
        }

        WriteInfo(TraceEdgeProbe, "{0} received RingAdjust from {1} body {2}",
            site_.Id, from, body);

        WriteLock grab(*this);

        if (body.IsSuccDirection())
        {
            if (succRecoveryTime_ != StopwatchTime::Zero)
            {
                return;
            }
            
            if (ackedProbe_ == site_.Id || site_.Id.SuccDist(ackedProbe_) > site_.Id.SuccDist(from))
            {
                ackedProbe_ = from;
                probeResult_ = (knownTable_.SuccHoodCount == 0 && body.GetNeighborhoodCount() == 0 && neighborhoodVersion_ == body.GetVersion());

                WriteInfo(TraceEdgeProbe, "{0} update probe result with {1}:{2}",
                    site_.Id, from, probeResult_);
            }
        }
        else if (predRecoveryTime_ == StopwatchTime::Zero)
        {
            if (knownTable_.PredHoodCount == 0 && body.GetNeighborhoodCount() == 0 && neighborhoodVersion_ == body.GetVersion())
            {
                predRecoveryTime_ = Stopwatch::Now();
                predProbeTarget_ = from;

                WriteWarning(TraceGap,
                    "{0} start recovery on predecessor side with {1}",
                    site_.Id, predProbeTarget_);
            }
            else
            {
                predProbeTarget_ = site_.Id;
            }
        }

        CheckHealth();
    }

    void RoutingTable::CheckEmptyNeighborhood()
    {
        if (knownTable_.CompleteHoodRange)
        {
            RoutingToken & token = site_.GetTokenForUpdate();
            if (!token.IsFull)
            {
                RoutingToken old(token);

                token.Update(NodeIdRange::Full, true);

                WriteTokenInfo(TraceRecovery,
                    "{0} recovered full token from {1} to {2}",
                    site_.Id, old, token);
            }
        }
    }

    StopwatchTime RoutingTable::RequestFirstToken(StateMachineActionCollection & actions)
    {
        // Before we get the first token, use lastSuccTokenProbe_ to maintain the
        // last token request time.
        StopwatchTime nextProbe = lastSuccTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval;
        if (nextProbe > Stopwatch::Now())
        {
            return nextProbe;
        }

        NodeId id = site_.Id;
        PartnerNodeSPtr const & next = ring_.Next;
        PartnerNodeSPtr const & prev = ring_.Prev;
        LargeInteger nextDist = id.SuccDist(next->Id);
        LargeInteger prevDist = id.PredDist(prev->Id);

        if ((next->IsRouting) &&
            (next->Token.Contains(id) || !prev->IsRouting || nextDist < prevDist))
        {
            actions.Add(make_unique<SendMessageAction>(FederationMessage::GetTokenRequest().CreateMessage(), next));
        }
        else if (prev->IsRouting)
        {
            actions.Add(make_unique<SendMessageAction>(FederationMessage::GetTokenRequest().CreateMessage(), prev));
        }
        else
        {
            return Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
        }

        // If we come to here a request action has been added.
        lastSuccTokenProbe_ = Stopwatch::Now();
        return lastSuccTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval;
    }

    StopwatchTime RoutingTable::CheckSuccToken(StateMachineActionCollection & actions)
    {
        RoutingToken & token = site_.GetTokenForUpdate();

        size_t index = ring_.GetSucc(ring_.ThisNode);
        PartnerNodeSPtr const & neighbor = ring_.GetNode(index);
        if (token.Contains(neighbor->Id) && neighbor->IsRouting)
        {
            WriteTokenInfo(TraceTransfer,
                "{0} transferring token to {1}",
                site_.Id, neighbor->Id);
            actions.Add(make_unique<SendMessageAction>(CreateTokenTransferMessage(neighbor), neighbor));
            return Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
        }

        if (neighbor->Token.IsEmpty)
        {
            do
            {
                index = ring_.GetSucc(index);
            } while (ring_.GetNode(index)->Token.IsEmpty);
        }

        PartnerNodeSPtr const & target = ring_.GetNode(index);
        RoutingToken const & targetToken = target->Token;

        if (token.IsSuccAdjacent(targetToken))
        {
            NodeId mid = site_.Id.GetSuccMidPoint(target->Id);
            if (token.Range.End == mid)
            {
                lastSuccTokenProbe_ = StopwatchTime::Zero;
                if (index == ring_.GetSucc(ring_.ThisNode))
                {
                    return StopwatchTime::MaxValue;
                }
            }
            else if (token.Contains(mid))
            {
                WriteTokenInfo(TraceTransfer,
                    "{0} transferring token to {1}",
                    site_.Id, target->Id);
                actions.Add(make_unique<SendMessageAction>(CreateTokenTransferMessage(target), target));
            }
            else if(lastSuccTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval < Stopwatch::Now())
            {
                lastSuccTokenProbe_ = Stopwatch::Now();
                WriteTokenInfo(TraceRequest,
                    "{0} requesting token from {1}",
                    site_.Id, target->Id);
                actions.Add(make_unique<SendMessageAction>(FederationMessage::GetTokenRequest().CreateMessage(), target));
            }

            return Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
        }

        if (token.IsFull)
        {
            ASSERT_IF(index != ring_.ThisNode, 
                "{0} full token and routing table contains other node with token {1}",
                site_.Id, *this);

            lastSuccTokenProbe_ = StopwatchTime::Zero;
            return StopwatchTime::MaxValue;
        }

        if (succPendingTransfer_)
        {
            StopwatchTime expiryTime = succPendingTransfer_->ExpiryTime;
            if (expiryTime > Stopwatch::Now() && token.Range.IsSuccAdjacent(succPendingTransfer_->Range))
            {
                return expiryTime;
            }

            succPendingTransfer_ = nullptr;
        }

        if (lastSuccTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval < Stopwatch::Now())
        {
            TimeSpan delay = tokenProbeThrottle_.CheckDelay();
            if (delay > TimeSpan::Zero)
            {
                return Stopwatch::Now() + delay;
            }

            PartnerNodeSPtr to;
            auto message = GetProbeMessage(true, to);
            if (to)
            {
                WriteTokenInfo(TraceProbe,
                    "{0} sending successor probe to {1}, last probe sent at {2}",
                    site_.Id, to->Id, lastSuccTokenProbe_);
                actions.Add(make_unique<SendMessageAction>(move(message), to));
                lastSuccTokenProbe_ = Stopwatch::Now();
                tokenProbeThrottle_.Add();
            }
            else
            {
                return StopwatchTime::MaxValue;
            }
        }

        return lastSuccTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval;
    }

    StopwatchTime RoutingTable::CheckPredToken(StateMachineActionCollection & actions)
    {
        RoutingToken & token = site_.GetTokenForUpdate();

        size_t index = ring_.GetPred(ring_.ThisNode);
        PartnerNodeSPtr const & neighbor = ring_.GetNode(index);
        if (token.Contains(neighbor->Id) && neighbor->IsRouting)
        {
            WriteTokenInfo(TraceTransfer,
                "{0} transferring token to {1}",
                site_.Id, neighbor->Id);
            actions.Add(make_unique<SendMessageAction>(CreateTokenTransferMessage(neighbor), neighbor));
            return Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
        }

        if (neighbor->Token.IsEmpty)
        {
            do
            {
                index = ring_.GetPred(index);
            } while (ring_.GetNode(index)->Token.IsEmpty);
        }

        PartnerNodeSPtr const & target = ring_.GetNode(index);
        RoutingToken const & targetToken = target->Token;

        if (token.IsPredAdjacent(targetToken))
        {
            NodeId mid = site_.Id.GetPredMidPoint(target->Id);
            if (token.Range.Begin == mid)
            {
                lastPredTokenProbe_ = StopwatchTime::Zero;
                if (index == ring_.GetPred(ring_.ThisNode))
                {
                    return StopwatchTime::MaxValue;
                }
            }
            else if (token.Contains(mid))
            {
                WriteTokenInfo(TraceTransfer,
                    "{0} transferring token to {1}",
                    site_.Id, target->Id);
                actions.Add(make_unique<SendMessageAction>(CreateTokenTransferMessage(target), target));
            }
            else if(lastPredTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval < Stopwatch::Now())
            {
                lastPredTokenProbe_ = Stopwatch::Now();
                WriteTokenInfo(TraceRequest,
                    "{0} requesting token from {1}",
                    site_.Id, target->Id);
                actions.Add(make_unique<SendMessageAction>(FederationMessage::GetTokenRequest().CreateMessage(), target));
            }

            return Stopwatch::Now() + FederationConfig::GetConfig().TokenProbeInterval;
        }

        if (token.IsFull)
        {
            ASSERT_IF(index != ring_.ThisNode, 
                "{0} full token and routing table contains other node with token {1}",
                site_.Id, *this);

            lastPredTokenProbe_ = StopwatchTime::Zero;
            return StopwatchTime::MaxValue;
        }

        if (predPendingTransfer_)
        {
            StopwatchTime expiryTime = predPendingTransfer_->ExpiryTime;
            if (expiryTime > Stopwatch::Now() && token.Range.IsPredAdjacent(predPendingTransfer_->Range))
            {
                return expiryTime;
            }

            predPendingTransfer_ = nullptr;
        }

        // This is needed only for backward compatibility with V1 since in V1 when there are
        // only 2 nodes, we are only sending probe from smaller node
        if (ring_.Size == 2 && site_.Id.IdValue < neighbor->Id.IdValue && lastPredTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval < Stopwatch::Now())
        {
            PartnerNodeSPtr to;
            auto message = GetProbeMessage(false, to);
            if (to)
            {
                WriteTokenInfo(TraceProbe,
                    "{0} sending pred probe to {1}, last probe sent at {2}",
                    site_.Id, to->Id, lastPredTokenProbe_);
                actions.Add(make_unique<SendMessageAction>(move(message), to));
                lastPredTokenProbe_ = Stopwatch::Now();
            }
            else
            {
                return StopwatchTime::MaxValue;
            }
        }

        return lastPredTokenProbe_ + FederationConfig::GetConfig().TokenProbeInterval;
    }

    StopwatchTime RoutingTable::CheckToken(StateMachineActionCollection & actions)
    {
        if (ring_.Size == 1)
        {
            CheckEmptyNeighborhood();
            return StopwatchTime::MaxValue;
        }

        if (site_.Token.IsEmpty)
        {
            if (Stopwatch::Now() > tokenAcquireDeadline_)
            {
                site_.CompleteOpenAsync(ErrorCodeValue::TokenAcquireTimeout);

                return StopwatchTime::MaxValue;
            }

            return RequestFirstToken(actions);
        }

        StopwatchTime succCheck = CheckSuccToken(actions);
        StopwatchTime predCheck = CheckPredToken(actions);

        return min(succCheck, predCheck);
    }

    void RoutingTable::TokenTransferRequestMessageHandler(Message &, OneWayReceiverContext & oneWayReceiverContext)
    {
        MessageUPtr tokenTransferMessage;
        {
            WriteLock grab(*this);
            tokenTransferMessage = CreateTokenTransferMessage(oneWayReceiverContext.From);
        }

        if(tokenTransferMessage)
        {
            site_.Send(move(tokenTransferMessage), oneWayReceiverContext.From);
        }
        else
        {
            WriteTokenInfo(TraceTransfer,
                "No token found to transfer from {0} to {1}",
                site_.Id,oneWayReceiverContext.From->Id);
        }

        oneWayReceiverContext.Accept();
    }

    void RoutingTable::TokenAcceptRejectMessageHandler(Message &, OneWayReceiverContext & oneWayReceiverContext)
    {
        oneWayReceiverContext.Accept();
    }

    void RoutingTable::TokenTransferMessageHandler(Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        if (oneWayReceiverContext.FromInstance == oneWayReceiverContext.From->Instance)
        {
            vector<FederationRoutingTokenHeader>* rejectedTokens = message.TryGetProperty<vector<FederationRoutingTokenHeader>>(Constants::RejectedTokenKey);
            if(rejectedTokens && rejectedTokens->size() > 0)
            {
                auto tokenTransferRejectMessage = FederationMessage::GetTokenTransferReject().CreateMessage();
                for (FederationRoutingTokenHeader const & header : *rejectedTokens)
                {
                    FederationRoutingTokenHeader tokenHeader(header.Range, site_.Token.Version, header.SourceVersion);
                    tokenTransferRejectMessage->Headers.Add<FederationRoutingTokenHeader>(tokenHeader);
                }

                site_.Send(move(tokenTransferRejectMessage), oneWayReceiverContext.From);
            }
            else
            {
                auto tokenTransferAcceptMessage = FederationMessage::GetTokenTransferAccept().CreateMessage();
                site_.Send(move(tokenTransferAcceptMessage), oneWayReceiverContext.From);
            }
        }

        oneWayReceiverContext.Accept();
    }

    void RoutingTable::UpdateProbeHeader(FederationTraceProbeHeader & probeHeader)
    {
        for (size_t i = probeHeader.TraceProbeNodes.size() - 1; i > 0; i--)
        {
            TraceProbeNode const & target = probeHeader.TraceProbeNodes[i - 1];
            PartnerNodeSPtr const & node = GetInternal(target.Instance.Id, &knownTable_);
            if (node)
            {
                if (node->Instance.InstanceId > target.Instance.InstanceId || (!target.IsShutdown && node->IsShutdown))
                {
                    probeHeader.Replace(i - 1, TraceProbeNode(node->Instance, node->IsShutdown));
                }

                if (node->IsAvailable)
                {
                    return;
                }
            }
        }
    }

    void RoutingTable::TokenProbeMessageHandler(Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        FederationTraceProbeHeader probeHeader;
        if (!message.Headers.TryReadFirst(probeHeader) || probeHeader.TraceProbeNodes.size() == 0)
        {
            WriteTokenWarning(TraceReject, "Rejecting TokenProbe message since it does not contain Probe header");
            oneWayReceiverContext.Reject(ErrorCode(ErrorCodeValue::InvalidMessage));
            return;
        }

        WriteTokenInfo(TraceProbe,
            "Received Token probe message at {0} from {1}",
            site_.Id, oneWayReceiverContext.From->Id);

        PartnerNodeSPtr to;
        MessageUPtr reply;
        {
            WriteLock grab(*this);
            if(site_.Token.IsEmpty)
            {
                //If Token is empty we forward the token.
                if (probeHeader.TraceProbeNodes.rbegin()->Instance != site_.Instance)
                {
                    WriteTokenWarning(TraceReject,
                        "{0} reject TokenProbe message for mismatching probe {1}",
                        site_.Instance, *probeHeader.TraceProbeNodes.rbegin());
                    oneWayReceiverContext.Reject(ErrorCode(ErrorCodeValue::InvalidMessage));
                    return;
                }

                UpdateProbeHeader(probeHeader);

                // Replace this node to give version information.
                TraceProbeNode traceProbeNode(site_.Instance, site_.Phase == NodePhase::Shutdown, neighborhoodVersion_);
                probeHeader.Replace(probeHeader.TraceProbeNodes.size() - 1, traceProbeNode);

                reply = PrepareProbeMessage(probeHeader, probeHeader.IsSuccDirection, to);
            }
            else
            {
                //If we already have a token we reply along the probe path with an Echo message
                if(probeHeader.TraceProbeNodes.size() == 0)
                {
                    WriteTokenWarning(TraceProbe,
                        "{0} dropping Probe message from {1} since probe header has no origin ",
                        site_.Id, oneWayReceiverContext.From->Id);
                    return;
                }

                UpdateProbeHeader(probeHeader);

                TraceProbeNode origin = probeHeader.TraceProbeNodes[0];
                probeHeader.Remove();
                VerifyProbePath(probeHeader, to, true);

                //The second condition verifies that there is a gap in token ange and recovery is needed
                if(!to || (!site_.Instance.Match(origin.Instance) && site_.Token.Contains(origin.Instance.Id)))
                {
                    return;
                }
                else
                {
                    reply = PrepareEchoMessage(probeHeader);

                    if(!reply)
                    {
                        //PrepareEchoMessage traces information
                        return;
                    }
                }
            }
        }

        if (reply && to)
        {
            site_.Send(move(reply), to);
        }

        oneWayReceiverContext.Accept();
    }

    void RoutingTable::TokenEchoMessageHandler(Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        FederationTraceProbeHeader probeHeader;
        if (!message.Headers.TryReadFirst(probeHeader) || probeHeader.TraceProbeNodes.size() == 0)
        {
            WriteTokenWarning(TraceProbe, "Rejecting Token echo message since it does not contain Probe header");
            oneWayReceiverContext.Reject(ErrorCode(ErrorCodeValue::InvalidMessage));
            return;
        }

        FederationTokenEchoHeader echoHeader;
        if (!message.Headers.TryReadFirst(echoHeader))
        {
            WriteTokenWarning(TraceProbe, "Rejecting Token echo message since it does not contain echo header");
            oneWayReceiverContext.Reject(ErrorCode(ErrorCodeValue::InvalidMessage));
            return;
        }

        WriteTokenInfo(TraceProbe, "Received Token echo message at {0} from {1}", site_.Id, oneWayReceiverContext.From->Id);

        PartnerNodeSPtr to;
        MessageUPtr reply;
        {
            WriteLock grab(*this);
            //This means that we are at the origin and should do token recovery
            TraceProbeNode originTraceProbe = probeHeader.TraceProbeNodes[0];
            if(probeHeader.TraceProbeNodes.size() == 1)
            {
                if(!originTraceProbe.Instance.Match(site_.Instance) || originTraceProbe.IsShutdown)
                {
                    WriteTokenWarning(TraceProbe, 
                        "Rejecting Token echo message at {0} since the origin does not match probeHeader: Instance {1}, IsShutdown {2}",
                        site_.Instance, originTraceProbe.Instance, originTraceProbe.IsShutdown);
                    oneWayReceiverContext.Reject(ErrorCode(ErrorCodeValue::InvalidMessage));
                    return;
                }

                if(!CanRecover(probeHeader.GetSourceVersion(), echoHeader.Origin.Id, probeHeader.IsSuccDirection))
                {
                    WriteTokenInfo(TraceProbe, 
                        "Rejecting Token echo message at {0} due to version mismatch. Original {1}, Current {2}",
                        site_.Instance, ((uint32)probeHeader.GetSourceVersion()), site_.Token.TokenVersion);
                    oneWayReceiverContext.Accept();
                    return;
                }

                vector<FederationRoutingTokenHeader> tokens;
                bool tokenRecovered = false;
                PartnerNodeSPtr origin = GetInternal(echoHeader.Origin);
                if (echoHeader.Origin.Match(site_.Instance))
                {
                    // If both ends of the probe is this node, there cannot
                    // be another routing node in the ring and therefore
                    // this node can recover the entire token.
                    if (!site_.Token.IsFull)
                    {
                        site_.GetTokenForUpdate().Update(NodeIdRange::Full);
                        tokenRecovered = true;
                    }
                }
                else if(probeHeader.IsSuccDirection)
                {
                    NodeId newEnd = echoHeader.Range.Begin - LargeInteger::One;
                    if (!site_.GetTokenForUpdate().Contains(newEnd))
                    {
                        // Extend token to cover the gap.
                        site_.GetTokenForUpdate().Update(NodeIdRange(site_.Token.Range.Begin, newEnd));
                        tokenRecovered = true;
                        FederationRoutingTokenHeader token = site_.GetTokenForUpdate().SplitSuccToken(origin, echoHeader.SourceVersion, site_.Id);
                        if (!token.IsEmpty)
                        {
                            succPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(token);
                            tokens.push_back(token);
                        }
                    }
                }
                else
                {
                    NodeId newBegin = echoHeader.Range.End + LargeInteger::One;
                    if (!site_.Token.Contains(newBegin))
                    {
                        // Extend token to cover the gap.
                        site_.GetTokenForUpdate().Update(NodeIdRange(newBegin, site_.Token.Range.End));
                        tokenRecovered = true;
                        FederationRoutingTokenHeader token = site_.GetTokenForUpdate().SplitPredToken(origin, echoHeader.SourceVersion, site_.Id);
                        if (!token.IsEmpty)
                        {
                            predPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(token);
                            tokens.push_back(token);
                        }
                    }
                }

                if(tokenRecovered)
                {
                    site_.GetTokenForUpdate().IncrementRecoveryVersion();
                    WriteTokenInfo(TraceRecovery, "Token {0} recovered at {1} from {2}",
                        site_.Token, site_.Id, echoHeader.Origin.Id);
                }

                if(tokens.size() != 0)
                {
                    reply = CreateTokenTransferMessage(origin, tokens);
                    to = origin;
                }
            }
            else
            {
                // This can only happen if this node gets a token between the
                // two passes.  Abort the recovery.
                // Note that it is probably possible to use this node as the
                // origin of the new echo origin.  But such case should be
                // rare so we are not doing this kind of optimization.
                if (!site_.Token.IsEmpty)
                {
                    WriteTokenInfo(TraceProbe, 
                        "{0} is new routing node along echo path from {1} so dropping message", 
                        site_.Id, echoHeader.Origin.Id);
                    oneWayReceiverContext.Accept();
                    return;
                }

                TraceProbeNode const & probeNode = (*probeHeader.TraceProbeNodes.rbegin());
                if (probeNode.Instance != site_.Instance)
                {
                    WriteTokenInfo(TraceProbe, 
                        "{0} reject echo message for mismatching probe {1}", 
                        site_.Instance, probeNode);
                    oneWayReceiverContext.Accept();
                    return;
                }

                if (probeNode.NeighborhoodVersion != neighborhoodVersion_)
                {
                    WriteTokenInfo(TraceProbe, 
                        "{0} reject echo message for mismatching version {1} vs. {2}", 
                        site_.Id, probeNode.NeighborhoodVersion, neighborhoodVersion_);
                    oneWayReceiverContext.Accept();
                    return;
                }

                probeHeader.Remove();
                VerifyProbePath(probeHeader, to, false);
                if(to)
                {
                    site_.GetTokenForUpdate().IncrementRecoveryVersion();
                    reply = FederationMessage::GetTokenEcho().CreateMessage();
                    reply->Headers.Add<FederationTokenEchoHeader>(echoHeader);
                    reply->Headers.Add<FederationTraceProbeHeader>(probeHeader);
                }
                else
                {
                    WriteTokenInfo(TraceProbe, 
                        "Could not verify probe path at {0} from {1} for origin {2} so dropping message", 
                        site_.Id, oneWayReceiverContext.From->Id, echoHeader.Origin.Id);
                }
            }
        }

        if(reply)
        {
            site_.Send(move(reply), to);
        }
        oneWayReceiverContext.Accept();
    }

    RoutingToken RoutingTable::GetRoutingToken() const
    {
        AcquireReadLock grab(lock_);
        return site_.Token;
    }

    RoutingToken RoutingTable::GetRoutingTokenAndShutdownNodes(vector<NodeInstance> & shutdownNodes) const
    {
        AcquireReadLock grab(lock_);

        vector<PartnerNodeSPtr> neighbors;
        knownTable_.GetHood(neighbors);

        for (PartnerNodeSPtr const & neighbor : neighbors)
        {
            if (neighbor->IsShutdown)
            {
                shutdownNodes.push_back(neighbor->Instance);
            }
        }

        return site_.Token;
    }

    bool RoutingTable::CanRecover(uint64 version, NodeId const& origin, bool succDirection)
    {
        LargeInteger distance;
        bool canRecover;
        if (succDirection)
        {
            distance = site_.Id.SuccDist(origin);
            canRecover = succEchoList_.CanRecover(version, distance);
        }
        else
        {
            distance = site_.Id.PredDist(origin);
            canRecover = predEchoList_.CanRecover(version, distance);
        }

        if (!canRecover)
        {
            WriteTokenInfo(TraceProbe, 
                "Echo rejected at {0} with version {1} dist {2} list {3} from {4}",
                site_.Id, version, distance, succDirection ? succEchoList_ : predEchoList_, origin);
            return false;
        }

        return site_.Token.IsRecoverySafe(version);
    }

    void RoutingTable::VerifyProbePath(FederationTraceProbeHeader & probeHeader, PartnerNodeSPtr & to, bool ignoreStaleInfo)
    {
        to = nullptr;
        vector<PartnerNodeSPtr> neighbors;
        if(probeHeader.IsSuccDirection)
        {
            knownTable_.GetPredHood(neighbors);
        }
        else
        {
            knownTable_.GetSuccHood(neighbors);
        }

        if(neighbors.size() == 0)
        {
            return;
        }

        size_t probeNodesIndex = probeHeader.TraceProbeNodes.size();
        size_t neighborIndex = neighbors.size();
        PartnerNodeSPtr neighbor;
        LargeInteger distanceToNeighbor;
        bool moveNext = true;
        StopwatchTime now = Stopwatch::Now();

        while(probeNodesIndex > 0)
        {
            probeNodesIndex--;
            TraceProbeNode const & probeNode = probeHeader.TraceProbeNodes[probeNodesIndex];
            LargeInteger distanceToProbeNode = probeHeader.IsSuccDirection ? site_.Id.PredDist(probeNode.Instance.Id) : site_.Id.SuccDist(probeNode.Instance.Id);

            if(moveNext)
            {
                if(neighborIndex > 0)
                {
                    neighborIndex--;
                    neighbor = neighbors[neighborIndex];
                }
                else
                {
                    WriteTokenInfo(TraceProbe, "Path verification on {0} failed as no forwarding node can be found",
                        site_.Id);
                    return;
                }
                distanceToNeighbor = probeHeader.IsSuccDirection ? site_.Id.PredDist(neighbor->Id) : site_.Id.SuccDist(neighbor->Id);
                moveNext = false;
            }

            if(probeNode.Instance.Id == neighbor->Id)
            {
                // If the current known state does not match what was known,
                // it is possible that the node has got a token and then sends
                // out a token between the two passes.  Not safe to proceed.
                if (!ignoreStaleInfo && !probeNode.Match(neighbor))
                {
                    WriteTokenInfo(TraceProbe, "Path verification on {0} failed as {1} does not match {2}",
                        site_.Id, neighbor, probeNode);
                    neighbor->UpdateLastAccess(now);
                    return;
                }

                // If the node is introduced, we should forward the echo to it
                // and there is no need to check for the rest.
                if(neighbor->IsAvailable)
                {
                    to = neighbor;
                    return;
                }

                // Remove this node from the path.
                probeHeader.Remove();

                // If the node is shutdown, we need to check for the next node.
                moveNext = true;
            }
            else if (distanceToProbeNode < distanceToNeighbor)
            {
                // A node recorded in the first pass is no longer in the neighborhood.
                // Either the node was down in the first pass (and therefore we forgot
                // about it), or we never know about such node yet (possible since it
                // is not introduced yet), in which case that node can't have got a
                // token and we can ignore it.
                probeHeader.Remove();
            }
            else
            {
                if (neighbor->IsAvailable)
                {
                    // We found a node not recorded in the first pass. It might
                    // have acquired the token in the mean time.  Abort to be safe.
                    WriteTokenInfo(TraceProbe, "Path verification on {0} failed as {1} is found, probe node {2}",
                        site_.Id, neighbor, probeNode);

                    neighbor->UpdateLastAccess(now);
                    return;
                }

                moveNext = true;
                probeNodesIndex++;
            }
        }
    }

    MessageUPtr RoutingTable::CreateTokenTransferMessage(PartnerNodeSPtr const & to, vector<FederationRoutingTokenHeader> const & tokens)
    {
        auto tokenTransferMessage = FederationMessage::GetTokenTransfer().CreateMessage();

        for (FederationRoutingTokenHeader const & token : tokens)
        {
            WriteTokenInfo(TraceTransfer,
                "{0} transferring token {1} to {2}",
                site_.Id, token, to->Id);

            tokenTransferMessage->Headers.Add<FederationRoutingTokenHeader>(token);
        }

        return move(tokenTransferMessage);
    }

    MessageUPtr RoutingTable::CreateTokenTransferMessage(PartnerNodeSPtr const & from)
    {
        vector<FederationRoutingTokenHeader> tokens;
        TrySplitToken(from, tokens);

        if (tokens.size() == 0)
        {
            return nullptr;
        }

        return CreateTokenTransferMessage(from, tokens);
    }

    MessageUPtr RoutingTable::GetProbeMessage(bool successorDirection, PartnerNodeSPtr & to)
    {
        TraceProbeNode traceProbeNode(site_.Instance, site_.Phase == NodePhase::Shutdown, neighborhoodVersion_);
        FederationTraceProbeHeader probeHeader(traceProbeNode, site_.Token.Version, successorDirection, site_.Token.Range);
        return PrepareProbeMessage(probeHeader, successorDirection, to);
    }

    MessageUPtr RoutingTable::PrepareProbeMessage(FederationTraceProbeHeader & header, bool successorDirection, PartnerNodeSPtr & to)
    {
        to = nullptr;

        auto probeMessage = FederationMessage::GetTokenProbe().CreateMessage();

        vector<PartnerNodeSPtr> nodes;
        if(successorDirection)
        {
            knownTable_.GetSuccHood(nodes);
        }
        else
        {
            knownTable_.GetPredHood(nodes);
        }

        vector<PartnerNodeSPtr>::reverse_iterator reverseIterator;
        for ( reverseIterator=nodes.rbegin() ; reverseIterator < nodes.rend(); ++reverseIterator )
        {
            if (header.TraceProbeNodes.size() < 100 || (*reverseIterator)->IsAvailable)
            {
                TraceProbeNode traceProbeNode((*reverseIterator)->Instance, (*reverseIterator)->Phase == NodePhase::Shutdown);
                header.AddNode(traceProbeNode);
            }

            if((*reverseIterator)->IsAvailable)
            {
                to = *reverseIterator;
                break;
            }
        }

        if(to)
        {
            probeMessage->Headers.Add<FederationTraceProbeHeader>(header);
        }

        return move(probeMessage);
    }

    MessageUPtr RoutingTable::PrepareEchoMessage(FederationTraceProbeHeader probeHeader)
    {
        RoutingToken & token = site_.GetTokenForUpdate();
        token.IncrementRecoveryVersion();
        auto tokenEchoMessage = FederationMessage::GetTokenEcho().CreateMessage();
        PartnerNodeSPtr origin = GetInternal(probeHeader.TraceProbeNodes[0].Instance);
        if(origin)
        {
            vector<FederationRoutingTokenHeader> tokens;
            TrySplitToken(origin, tokens);
            //TODO: verify if this is an assertion I can make
            //ASSERT_IF(tokens.size() > 1, "There should be only one token split on recovery");

            if(tokens.size() > 0)
            {
                WriteTokenInfo("Drop",
                    "Droping tokens found beyond midpoint between {0} and {1}",
                    site_.Id, probeHeader.TraceProbeNodes[0].Instance.Id);
            }
            else
            {
                if(probeHeader.IsSuccDirection)
                {
                    succPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(NodeIdRange(token.Range.End.SuccWalk(LargeInteger::One), 
                        probeHeader.Range.Begin.PredWalk(LargeInteger::One)),
                        token.Version,
                        probeHeader.GetSourceVersion());
                }
                else
                {
                    predPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(NodeIdRange(probeHeader.Range.End.SuccWalk(LargeInteger::One), 
                        token.Range.Begin.PredWalk(LargeInteger::One)),
                        token.Version,
                        probeHeader.GetSourceVersion());
                }
            }

            if(probeHeader.IsSuccDirection)
            {
                predEchoList_.Add(token.Version, site_.Id.PredDist(origin->Id));
            }
            else
            {
                succEchoList_.Add(token.Version, site_.Id.SuccDist(origin->Id));
            }

            tokenEchoMessage->Headers.Add<FederationTraceProbeHeader>(probeHeader);
            FederationTokenEchoHeader header(site_.Token.Range, site_.Token.Version, probeHeader.GetSourceVersion(), site_.Instance);
            tokenEchoMessage->Headers.Add<FederationTokenEchoHeader>(header);

            return move(tokenEchoMessage);
        }
        else
        {
            //Droping echo since we cannot find the origin.
            WriteTokenInfo(TraceProbe,
                "Droping echo at {0} since we cannot find the origin {1}",
                site_.Instance, probeHeader.TraceProbeNodes[0].Instance);
            return nullptr;
        }
    }

    void RoutingTable::Split(PartnerNodeSPtr const & neighbor, NodeId const & ownerId, vector<FederationRoutingTokenHeader> & tokens, uint64 targetVersion)
    {
        RoutingToken & token = site_.GetTokenForUpdate();
        RoutingToken neighborToken = neighbor->Token;
        FederationRoutingTokenHeader tokenHeader;

        /// Checks that the neighbors id is owned by this node. Hence this is the first transfer.
        bool isSync = token.Contains(neighbor->Id);
        if (isSync)
        {
            if (token.IsFull)
            {
                tokenHeader = token.SplitFullToken(neighbor, targetVersion, ownerId);
                succPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(tokenHeader);
                predPendingTransfer_= make_unique<FederationRoutingTokenHeader>(tokenHeader);
                tokens.push_back(tokenHeader);
            }
            else
            {
                LargeInteger succDist = ownerId.SuccDist(neighbor->Id);
                LargeInteger succToken = ownerId.SuccDist(token.Range.End);

                if (succDist <= succToken)
                {
                    tokenHeader = token.SplitSuccToken(neighbor, targetVersion, ownerId);
                    succPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(tokenHeader);
                }
                else
                {
                    tokenHeader = token.SplitPredToken(neighbor, targetVersion, ownerId);
                    predPendingTransfer_= make_unique<FederationRoutingTokenHeader>(tokenHeader);
                }

                ASSERT_IF(token.IsEmpty, "{0} split for {1} returns empty token",
                    *this, neighbor);
                tokens.push_back(tokenHeader);
            }
        }
        else
        {
            if (token.IsSuccAdjacent(neighborToken))
            {
                tokenHeader = token.SplitSuccToken(neighbor, targetVersion, ownerId);
                if (!token.IsEmpty)
                {
                    succPendingTransfer_ = make_unique<FederationRoutingTokenHeader>(tokenHeader);
                    tokens.push_back(tokenHeader);
                }
            }

            if (token.IsPredAdjacent(neighborToken))
            {
                tokenHeader = token.SplitPredToken(neighbor, targetVersion, ownerId);
                if (!token.IsEmpty)
                {
                    predPendingTransfer_= make_unique<FederationRoutingTokenHeader>(tokenHeader);
                    tokens.push_back(tokenHeader);
                }
            }
        }
    }

    bool RoutingTable::CheckLiveness(FederationPartnerNodeHeader const & target)
    {
        WriteLock grab(*this);

        if (site_.Token.Contains(target.Instance.Id) && site_.Instance != target.Instance)
        {
            WriteInfo(TraceDown, "{0} set {1} to shutdown", site_.Id, target);
            InternalConsider(FederationPartnerNodeHeader(target.Instance, NodePhase::Shutdown, target.Address, target.LeaseAgentAddress, target.LeaseAgentInstanceId, target.Token, target.NodeFaultDomainId, false, target.RingName, target.Flags));

            return false;
        }

        return true;
    }

    void RoutingTable::LivenessQueryRequestHandler(Message & message, RequestReceiverContext & requestReceiverContext)
    {
        FederationPartnerNodeHeader target;
        if (message.GetBody(target))
        {
            bool result = CheckLiveness(target);

            requestReceiverContext.Reply(FederationMessage::GetLivenessQueryReply().CreateMessage(LivenessQueryBody(result)));
        }
        else
        {
            requestReceiverContext.InternalReject(message.FaultErrorCodeValue);
        }
    }

    StopwatchTime RoutingTable::CheckInsertingNode(StateMachineActionCollection & actions)
    {
        vector<PartnerNodeSPtr> nodes;
        knownTable_.GetHood(nodes);

        TimeSpan timeout = FederationConfig::GetConfig().TokenAcquireTimeout;
        StopwatchTime now = Stopwatch::Now();
        bool found = false;

        for (PartnerNodeSPtr const & node : nodes)
        {
            if (node->Phase == NodePhase::Inserting)
            {
                found = true;

                if (node->LastUpdated + timeout < now)
                {
                    actions.Add(make_unique<LivenessQueryAction>(node));
                }
            }
        }

        return (found ? Stopwatch::Now() + timeout : StopwatchTime::MaxValue);
    }

    void RoutingTable::Compact()
    {
        FederationConfig const & config = FederationConfig::GetConfig();

        StopwatchTime now = Stopwatch::Now();

        bool capacityExceeded = (knownTable_.Size > static_cast<size_t>(config.RoutingTableCapacity));
        bool removeDownNeighbor = (capacityExceeded || knownTable_.GetHoodCount() > config.MaxNodesToKeepInNeighborhood);
        if (!capacityExceeded && !removeDownNeighbor && now < lastCompactTime_ + config.RoutingTableCompactInterval)
        {
            return;
        }

        lastCompactTime_ = now;

        WriteInfo(TraceCompact, site_.IdString,
            "Compacting routing table, current size={0}/{1}, neighborhood={2}", ring_.Size, knownTable_.Size, knownTable_.GetHoodCount());

        StopwatchTime timeBound = now - knownTable_.NeighborhoodExchangeInterval;

        size_t i = 0;
        size_t j = 0;
        while (i < knownTable_.Size)
        {
            PartnerNodeSPtr const & node = knownTable_.GetNode(i);
            if (i != knownTable_.PredHoodEdge && i != knownTable_.SuccHoodEdge)
            {
                bool remove;
                if (node->IsAvailable)
                {
                    if (!knownTable_.WithinHoodRange(node->Id) && node->LastConsider < timeBound)
                    {
                        while (j < ring_.Size && ring_.GetNode(j)->Id < node->Id)
                        {
                            j++;
                        }

                        ASSERT_IF(j >= ring_.Size || ring_.GetNode(j)->Id != node->Id,
                            "{0} inconsistent routing table at {1}",
                            site_.Id, node->Id);

                        ring_.RemoveNode(j);

                        remove = true;
                    }
                    else
                    {
                        remove = false;
                    }

                    if (node->Target && (remove || node->LastAccessed < timeBound))
                    {
                        node->Target->Reset();
                    }
                }
                else
                {
                    remove = (node->LastAccessed < timeBound) && (knownTable_.WithinHoodRange(node->Id) ? removeDownNeighbor : capacityExceeded);
                }

                if (remove)
                {
                    WriteInfo(TraceCompact, site_.IdString,
                        "Removing {0} {1} {2}", *node, node->LastConsider, node->LastAccessed);
                    knownTable_.RemoveNode(i);
                    i--;
                }
            }

            i++;
        }
    }

    void RoutingTable::InternalPartitionRanges(NodeIdRange const & range, size_t start, size_t end, size_t count, vector<PartnerNodeSPtr> & targets, vector<NodeIdRange> & subRanges) const
    {
        size_t nodeCount = (end >= start ? end - start + 1 : end + ring_.Size - start + 1);

        NodeId subStart = range.Begin;
        size_t index = start;
        for (size_t i = 0; i < count; i++)
        {
            size_t subCount = nodeCount / count;
            if (i < nodeCount % count)
            {
                subCount++;
            }

            size_t nextIndex = (index + subCount) % ring_.Size;
            NodeId subEnd;
            if (i < count - 1)
            {
                NodeId id1 = ring_.GetNode(ring_.GetPred(nextIndex))->Id;
                NodeId id2 = ring_.GetNode(nextIndex)->Id;
                subEnd = id1.GetSuccMidPoint(id2);
            }
            else
            {
                subEnd = range.End;
            }

            targets.push_back(ring_.GetNode((index + subCount / 2) % ring_.Size));
            subRanges.push_back(NodeIdRange(subStart, subEnd));

            index = nextIndex;
            subStart = subEnd + LargeInteger::One;
        }
    }

    NodeIdRange RoutingTable::PartitionRanges(NodeIdRange const & range, vector<PartnerNodeSPtr> & targets, vector<NodeIdRange> & subRanges, bool excludeNeighborhood) const
    {
        AcquireReadLock grab(lock_);

        NodeIdRange excludeRange;
        if (excludeNeighborhood)
        {
            excludeRange = knownTable_.GetRange();
        }
        else if (ring_.Size == 1)
        {
            excludeRange = NodeIdRange::Full;
        }
        else
        {
            excludeRange = NodeIdRange::Merge(NodeIdRange(ring_.Prev->Id + LargeInteger::One, ring_.Next->Id - LargeInteger::One), site_.Token.Range);
        }

        NodeIdRange range1, range2;
        range.Subtract(excludeRange, range1, range2);

        size_t start1, end1, start2, end2;
        size_t nodeCount1 = ring_.GetNodesInRange(range1, start1, end1);
        size_t nodeCount2 = ring_.GetNodesInRange(range2, start2, end2);
        if (nodeCount1 == 0 && nodeCount2 == 0)
        {
            return excludeRange;
        }

        size_t count = static_cast<size_t>(FederationConfig::GetConfig().BroadcastPropagationFactor);
        size_t nodeCount = nodeCount1 + nodeCount2;
        size_t count1, count2;
        if (nodeCount > count)
        {
            count1 = static_cast<size_t>(floor(static_cast<float>(count) * nodeCount1 / nodeCount + 0.5f));
            count2 = count - count1;
            if (count1 == 0 && nodeCount1 > 0)
            {
                count1 = 1;
                count2 = count - 1;
            }
            else if (count2 == 0 && nodeCount2 > 0)
            {
                count1 = count - 1;
                count2 = 1;
            }
        }
        else
        {
            count1 = nodeCount1;
            count2 = nodeCount2;
        }

        InternalPartitionRanges(range1, start1, end1, count1, targets, subRanges);
        InternalPartitionRanges(range2, start2, end2, count2, targets, subRanges);

        return excludeRange;
    }

    StopwatchTime RoutingTable::CheckLivenessUpdateAction(StateMachineActionCollection & actions)
    {
        StopwatchTime now = Stopwatch::Now();
        StopwatchTime result = now + FederationConfig::GetConfig().LivenessUpdateInterval;

        for (size_t i = 0; i < ring_.Size; i++)
        {
            PartnerNodeSPtr const & target = ring_.GetNode(i);
            if (!knownTable_.WithinHoodRange(target->Id))
            {
                if (now >= target->NextLivenessUpdate)
                {
                    actions.Add(make_unique<SendMessageAction>(FederationMessage::GetLivenessUpdate().CreateMessage(), target));
                }
                else if (target->NextLivenessUpdate < result)
                {
                    result = target->NextLivenessUpdate;
                }
            }
        }

        return result;
    }

    void RoutingTable::LivenessUpdateMessageHandler(Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        UNREFERENCED_PARAMETER(message);
        oneWayReceiverContext.Accept();
    }

    void RoutingTable::ReportNeighborhoodLost(wstring const & extraDescription)
    {
        TimeSpan ttl =  FederationConfig::GetConfig().GlobalTicketLeaseDuration;
        StopwatchTime now = Stopwatch::Now();
        TimeSpan timeout = TimeSpan::FromTicks(ttl.Ticks / 2);
        if (now > gapReportTime_ + timeout)
        {
            Client::HealthReportingComponentSPtr healthClient = site_.GetHealthClient();
            if (healthClient)
            {
                healthClient->AddHealthReport(ServiceModel::HealthReport::CreateSystemHealthReport(
                    SystemHealthReportCode::Federation_NeighborhoodLost,
                    ServiceModel::EntityHealthInformation::CreateClusterEntityHealthInformation(),
                    site_.IdString,
                    extraDescription,
                    ttl,
                    ServiceModel::AttributeList()));
            }

            WriteInfo(TraceGap, "{0} reporting neighborhood loss: {1}",
                site_.Id, extraDescription);

            gapReportTime_ = now;
        }
    }

    void RoutingTable::ReportNeighborhoodRecovered()
    {
        Client::HealthReportingComponentSPtr healthClient = site_.GetHealthClient();
        if (healthClient)
        {
            healthClient->AddHealthReport(ServiceModel::HealthReport::CreateSystemRemoveHealthReport(
                SystemHealthReportCode::Federation_NeighborhoodOk,
                ServiceModel::EntityHealthInformation::CreateClusterEntityHealthInformation(),
                site_.IdString));
        }

        WriteInfo(TraceGap, "{0} reporting neighborhood recovered", site_.Id);
        gapReportTime_ = StopwatchTime::Zero;
    }

    vector<NodeInstance> RoutingTable::RemoveDownNodes(vector<NodeInstance> & nodes)
    {
        vector<NodeInstance> downNodes;

        AcquireReadLock grab(lock_);

        for (int i = static_cast<int>(nodes.size() - 1); i >= 0; i--)
        {
            if (InternalIsDown(nodes[i]))
            {
                downNodes.push_back(nodes[i]);
                nodes.erase(nodes.begin() + i);
            }
        }

        return downNodes;
    }

    void RoutingTable::UpdateExternalRingVotes()
    {
        WriteLock grab(*this);

        VoteConfig const & votes = FederationConfig::GetConfig().Votes;
        for (auto it = votes.begin(); it != votes.end(); ++it)
        {
            if (!site_.IsRingNameMatched(it->RingName) && externalRings_.find(it->RingName) == externalRings_.end())
            {
                externalRings_.insert(make_pair(it->RingName, ExternalRing(site_, it->RingName)));
            }
        }

        for (auto it = externalRings_.begin(); it != externalRings_.end(); ++it)
        {
            it->second.LoadVotes();
        }
    }

    void RoutingTable::GetExternalRings(vector<wstring> & externalRings)
    {
        AcquireReadLock grab(lock_);

        for (auto it = externalRings_.begin(); it != externalRings_.end(); ++it)
        {
            externalRings.push_back(it->first);
        }
    }
    
    void RoutingTable::AddGlobalTimeExchangeHeader(Message & message, PartnerNodeSPtr const & target)
    {
        AcquireReadLock grab(lock_);
		globalTimeManager_.AddGlobalTimeExchangeHeader(message, target);
    }

    bool RoutingTable::ProcessGlobalTimeExchangeHeader(Message & message, NodeInstance const & fromInstance)
    {
        AcquireWriteLock grab(lock_);

        PartnerNodeSPtr from = GetInternal(fromInstance);
        return ProcessGlobalTimeExchangeHeaderInternal(message, from);
    }

    bool RoutingTable::ProcessGlobalTimeExchangeHeader(Message & message, PartnerNodeSPtr const & from)
    {
        AcquireWriteLock grab(lock_);
        return ProcessGlobalTimeExchangeHeaderInternal(message, from);
    }

    bool RoutingTable::ProcessGlobalTimeExchangeHeaderInternal(Message & message, PartnerNodeSPtr const & from)
    {
        GlobalTimeExchangeHeader header;
        if (!message.Headers.TryReadFirst(header))
        {
            return false;
        }

        globalTimeManager_.UpdateRange(header);
        if (from)
        {
            globalTimeManager_.UpdatePartnerNodeUpperLimit(header, from);
        }

        return true;
    }

    void RoutingTable::IncreaseGlobalTimeUpperLimitForNodes(TimeSpan delta)
    {
        for (size_t i = 0; i < knownTable_.Size; i++)
        {
            PartnerNodeSPtr const & node = knownTable_.GetNode(i);
            if (node->Id != site_.Id)
            {
                node->IncreaseGlobalTimeUpperLimit(delta);
            }
        }
    }

    void RoutingTable::Test_GetUpNodesOutsideNeighborhood(vector<NodeInstance> & nodes) const
    {
        AcquireReadLock grab(lock_);

        NodeIdRange neighborhoodRange = this->knownTable_.GetRange();
        NodeIdRange nonNeighborRange(neighborhoodRange.End, neighborhoodRange.Begin);
        this->knownTable_.GetNodesInRange(nonNeighborRange, nodes);
    }

    void RoutingTable::BecomeLeader()
    {
        globalTimeManager_.BecomeLeader();
    }
	
    int64 RoutingTable::GetGlobalTimeInfo(int64 & lowerLimit, int64 & upperLimit, bool & isAuthority)
    {
        return globalTimeManager_.GetInfo(lowerLimit, upperLimit, isAuthority);
    }

    bool RoutingTable::HasGlobalTime() const
    {
        return (globalTimeManager_.GetEpoch() > 0);
    }

    void RoutingTable::ReportImplicitArbitrationResult(bool result)
    {
        WriteLock grab(*this);
        implicitLeaseContext_.ReportImplicitArbitrationResult(result);
    }

    void RoutingTable::ReportArbitrationQueryResult(wstring const & address, int64 instance, StopwatchTime remoteResult)
    {
        WriteLock grab(*this);
        implicitLeaseContext_.ReportArbitrationQueryResult(address, instance, remoteResult);
    }

    void RoutingTable::EchoedProbeList::Add(uint64 version, LargeInteger distance)
    {
        // Add a new promise.  Since this code is always called
        // with the routing table lock, and before the call we
        // always increment token version by RECOVERY_INCREMENT,
        // it is guaranteed that the version passed in is greater
        // than any of the existing ones.

        // We search from the end to see whether this new promise
        // can be merged with any old ones.
        int index = static_cast<int>(echos_.size()) - 1;
        while(index >= 0)
        {
            EchoedProbe echo = echos_[index];

            // If the lower portion of the version is different,
            // there is no need to keep the old promises because
            // any recovery that will be allowed must match the
            // lower portion which means that those echo will be
            // originated after the promises were made, which will
            // always be safe.
            // If the promise at the end is for a node that is
            // closer than the current echo, we can't merge because
            // doing so would weaken the old promise.
            if (RoutingToken::TokenVersionMatch(echo.GetVersion(), version) &&
                echo.Closer(distance))
            {
                break;
            }

            // We don't need to remember this promise since the new one
            // is stronger.
            echos_.erase(echos_.begin() + index);
            index--;
        }

        // Append the new promise.  Note that by induction, we have the
        // following invariance:
        // Lower portion of version in all promises are equal.
        // The list is monotonically increasing for both version
        // and distance.
        echos_.push_back(EchoedProbe(version, distance));
    }

    bool RoutingTable::EchoedProbeList::CanRecover(uint64 version, LargeInteger dist)
    {
        while (echos_.size() > 0)
        {
            EchoedProbe echo = echos_[0];

            // If the current distance is no smaller than
            // the first, it must be that all promises are
            // made to nodes further away or to the same node.
            // Thus we are guaranteed not to break any promise.
            if (!echo.Closer(dist))
            {
                return (true);
            }

            // We have made a promise to a node that is closer,
            // and that promise was made after the token probe,
            // recovery is not safe.
            if (echo.GetVersion() > version)
            {
                return (false);
            }

            // This promise was made to a node that is closer,
            // but after that we send a token probe and got
            // the echo, this means that any effect of this
            // promise is already gone and we no longer need
            // to keep it.
            echos_.erase(echos_.begin());
        }

        // No effective promise, safe to recovery.
        return (true);
    }

    void RoutingTable::EchoedProbeList::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
    {
        w.Write("[count={0} {1}]", echos_.size(), echos_);
    }

    class ImplicitArbitrationAction : public StateMachineAction
    {
        DENY_COPY(ImplicitArbitrationAction)

    public:
        ImplicitArbitrationAction(ArbitrationType::Enum type, PartnerNodeSPtr const & remote, int count1, int count2)
            :   type_(type),
                remote_(remote->LeaseAgentAddress, remote->LeaseAgentInstanceId), 
                extraData_((min(count2, 15) << 4) + min(count1, 15))
        {
        }

        virtual void Execute(SiteNode & siteNode)
        {
            LeaseAgentInstance local(siteNode.LeaseAgentAddress, siteNode.LeaseAgentInstanceId);

            siteNode.GetVoteManager().Arbitrate(local, remote_, TimeSpan::Zero, TimeSpan::Zero, 0, 0, extraData_, type_);
        }

        wstring ToString()
        {
            return wformatString("{0} {1}", type_, remote_);
        }

    private:
        ArbitrationType::Enum type_; 
        LeaseAgentInstance remote_;
        int64 extraData_;
    };

    RoutingTable::ImplicitLeaseContext::ImplicitLeaseContext(SiteNode & site)
        :   site_(site),
            succInstance_(site.Instance),
            succStartTime_(StopwatchTime::Zero),
            succLease_(StopwatchTime::Zero),
            succQueryResult_(StopwatchTime::Zero),
            succArbitration_(0),
            succQuery_(false),
            predInstance_(site.Instance),
            predStartTime_(StopwatchTime::Zero),
            predLease_(StopwatchTime::Zero),
            predQueryResult_(StopwatchTime::Zero),
            predArbitration_(0),
            predQuery_(false),
            arbitrationStartTime_(StopwatchTime::Zero),
            arbitrationEndTime_(StopwatchTime::Zero),
            queryStartTime_(StopwatchTime::Zero),
            cycleStartTime_(StopwatchTime::Zero)
    {
    }

    void RoutingTable::ImplicitLeaseContext::Start()
    {
        if (cycleStartTime_ == StopwatchTime::Zero)
        {
            cycleStartTime_ = Stopwatch::Now();
        }
    }

    void RoutingTable::ImplicitLeaseContext::GetRemoteLeaseExpirationTime(PartnerNodeSPtr const & remote, StopwatchTime & monitorExpire, StopwatchTime & subjectExpire)
    {
        site_.GetLeaseAgent().GetRemoteLeaseExpirationTime(remote->Instance.ToString(), monitorExpire, subjectExpire);

        if (!remote->IsRouting)
        {
            subjectExpire = StopwatchTime::Zero;
        }

        WriteNoise(TraceNeighborLease, site_.IdString, "RemoteLeaseExpirationTime with {0}: {1}/{2}", remote->Instance, monitorExpire, subjectExpire);
    }

    void RoutingTable::ImplicitLeaseContext::Update(PartnerNodeSPtr const & succ, PartnerNodeSPtr const & pred)
    {
        if (succ->Instance != succInstance_)
        {
            succQueryResult_ = StopwatchTime::Zero;
            succQuery_ = false;
            succStartTime_ = Stopwatch::Now();

            if (!succ_)
            {
                succLease_ = succStartTime_;
                succArbitration_ = 0;
            }
            else
            {
                StopwatchTime monitorExpire;
                StopwatchTime subjectExpire;
                GetRemoteLeaseExpirationTime(succ_, monitorExpire, subjectExpire);

                if (subjectExpire > succLease_)
                {
                    succLease_ = subjectExpire;
                    succArbitration_ = 0;
                }

                if (site_.Id.SuccDist(succ->Id) < site_.Id.SuccDist(succInstance_.Id) && succLease_ > succStartTime_)
                {
                    succLease_ = succStartTime_;
                }
            }

            succInstance_ = succ->Instance;
            succ_ = (succInstance_ == site_.Instance ? nullptr : succ);

            WriteInfo(TraceImplicitLease, site_.IdString, "Update succ to {0} with lease: {1}",
                succInstance_, succLease_);
        }
        else if (succ_ && succ_ != succ)
        {
            succ_ = succ;
        }

        if (pred->Instance != predInstance_)
        {
            predQueryResult_ = StopwatchTime::Zero;
            predQuery_ = false;
            predStartTime_ = Stopwatch::Now();

            if (!pred_)
            {
                predLease_ = predStartTime_;
                predArbitration_ = 0;
            }
            else
            {
                StopwatchTime monitorExpire;
                StopwatchTime subjectExpire;
                GetRemoteLeaseExpirationTime(pred_, monitorExpire, subjectExpire);

                if (subjectExpire > predLease_)
                {
                    predLease_ = subjectExpire;
                    predArbitration_ = 0;
                }

                if (site_.Id.PredDist(pred->Id) < site_.Id.PredDist(predInstance_.Id) && predLease_ > predStartTime_)
                {
                    predLease_ = predStartTime_;
                }
            }

            predInstance_ = pred->Instance;
            pred_ = (predInstance_ == site_.Instance ? nullptr : pred);

            WriteInfo(TraceImplicitLease, site_.IdString, "Update pred to {0} with lease: {1}",
                predInstance_, predLease_);
        }
        else if (pred_ && pred_ != pred)
        {
            pred_ = pred;
        }
    }

    void RoutingTable::ImplicitLeaseContext::ReportImplicitArbitrationResult(bool result)
    {
        ASSERT_IF(arbitrationStartTime_ == StopwatchTime::Zero, "No pending implicit arbitration");
        if (result)
        {
            if (Stopwatch::Now() > arbitrationEndTime_)
            {
                WriteError(TraceImplicitLease, site_.IdString,
                    "Ignore implicit arbitration result expected at {0}",
                    arbitrationEndTime_);
            }
            else
            {
                cycleStartTime_ = arbitrationEndTime_;
                arbitrationStartTime_ = StopwatchTime::Zero;
                arbitrationEndTime_ = StopwatchTime::Zero;

                WriteInfo(TraceImplicitLease, site_.IdString,
                    "Implicit arbitration succeeded, setting cycle start time to {0}",
                    cycleStartTime_);
            }
        }
        else
        {
            WriteError(TraceImplicitLease, site_.IdString, "Implicit Lease arbitration failed");
            SiteNodeSPtr root = site_.GetSiteNodeSPtr();
            Threadpool::Post([root] { root->OnLeaseFailed(); });
        }
    }

    void RoutingTable::ImplicitLeaseContext::ReportArbitrationQueryResult(wstring const & address, int64 instance, StopwatchTime remoteResult)
    {
        FederationConfig const & config = FederationConfig::GetConfig();
        if (succQuery_)
        {
            if (succ_ && succ_->LeaseAgentAddress == address && succ_->LeaseAgentInstanceId == instance)
            {
                if (remoteResult != StopwatchTime::MaxValue)
                {
                    if (queryStartTime_ - remoteResult > config.MaxImplicitLeaseInterval + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout + config.MaxLeaseDuration)
                    {
                        WriteInfo(TraceImplicitLease, site_.IdString, "Set succ {0} down with implicit query result {1}", succInstance_, remoteResult);
                        site_.Table.InternalSetShutdown(succInstance_, site_.RingName);
                    }

                    WriteInfo(TraceImplicitLease, site_.IdString, "Update succ query result to {0} for {1}, lease {2}/{3}",
                        remoteResult, succInstance_, succLease_, predLease_);
                    succQueryResult_ = remoteResult;
                }
            }

            succQuery_ = false;
        }

        if (predQuery_)
        {
            if (pred_ && pred_->LeaseAgentAddress == address && pred_->LeaseAgentInstanceId == instance)
            {
                if (remoteResult != StopwatchTime::MaxValue)
                {
                    if (queryStartTime_ - remoteResult > config.MaxImplicitLeaseInterval + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout + config.MaxLeaseDuration)
                    {
                        WriteInfo(TraceImplicitLease, site_.IdString, "Set pred {0} down with implicit query result {1}", predInstance_, remoteResult);
                        site_.Table.InternalSetShutdown(predInstance_, site_.RingName);
                    }

                    WriteInfo(TraceImplicitLease, site_.IdString, "Update pred query result to {0} for {1}, lease {2}/{3}",
                        remoteResult, predInstance_, succLease_, predLease_);
                    predQueryResult_ = remoteResult;
                }
            }

            predQuery_ = false;
        }

        queryStartTime_ = StopwatchTime::Zero;
    }

    void RoutingTable::ImplicitLeaseContext::RunStateMachine(StateMachineActionCollection & actions)
    {
        if (cycleStartTime_ == StopwatchTime::Zero)
        {
            return;
        }

        StopwatchTime now = Stopwatch::Now();
        FederationConfig const & config = FederationConfig::GetConfig();
        TimeSpan queryInterval = config.ImplicitLeaseInterval + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout + config.MaxLeaseDuration;

        if (succ_ && arbitrationStartTime_ == StopwatchTime::Zero)
        {
            StopwatchTime monitorExpire;
            StopwatchTime subjectExpire;
            GetRemoteLeaseExpirationTime(succ_, monitorExpire, subjectExpire);

            if (subjectExpire > succLease_)
            {
                succLease_ = subjectExpire;
                succArbitration_ = 0;
            }

            StopwatchTime expireTime = max(succLease_, cycleStartTime_) + config.ImplicitLeaseInterval;
            if (now > expireTime - config.RoutingTableHealthCheckInterval)
            {
                arbitrationStartTime_ = now;
                arbitrationEndTime_ = min(expireTime, arbitrationStartTime_) + config.ArbitrationTimeout;
                ++succArbitration_;
                actions.Add(make_unique<ImplicitArbitrationAction>(ArbitrationType::Implicit, succ_, succArbitration_, now > predLease_ && predArbitration_ == 0 ? 1 : predArbitration_));
                WriteInfo(TraceImplicitLease, site_.IdString, "Start succ implicit arbitration with {0}: expire {1}/{2}, succLease: {3}, cycleStart: {4}, count={5}",
                    succInstance_, monitorExpire, subjectExpire, succLease_, cycleStartTime_, succArbitration_);
            }
            else if (queryStartTime_ == StopwatchTime::Zero &&
                now > monitorExpire + config.MaxArbitrationTimeout &&
                now > max(succLease_, succStartTime_) + queryInterval + config.MaxLeaseDuration &&
                now > succQueryResult_ + queryInterval + config.LeaseSuspendTimeout &&
                succ_->IsExtendedArbitrationEnabled())
            {
                queryStartTime_ = now;
                succQuery_ = true;
                actions.Add(make_unique<ImplicitArbitrationAction>(ArbitrationType::Query, succ_, succArbitration_, predArbitration_));
                WriteInfo(TraceImplicitLease, site_.IdString, "Start succ arbitration query {0}: expire: {1}/{2}, last query result: {3}, succ start: {4}",
                    succInstance_, monitorExpire, subjectExpire, succQueryResult_, succStartTime_);
            }
            else if (now > succLease_)
            {
                WriteInfo(TraceNeighborLease, site_.IdString, "Succ {0}: expire {1}/{2}, succLease: {3}, cycleStart: {4}",
                    succInstance_, monitorExpire, subjectExpire, succLease_, cycleStartTime_);
            }
        }

        if (pred_ && arbitrationStartTime_ == StopwatchTime::Zero)
        {
            StopwatchTime monitorExpire;
            StopwatchTime subjectExpire;
            GetRemoteLeaseExpirationTime(pred_, monitorExpire, subjectExpire);

            if (subjectExpire > predLease_)
            {
                predLease_ = subjectExpire;
                predArbitration_ = 0;
            }

            StopwatchTime expireTime = max(predLease_, cycleStartTime_) + config.ImplicitLeaseInterval;
            if (now > expireTime - config.RoutingTableHealthCheckInterval)
            {
                arbitrationStartTime_ = now;
                arbitrationEndTime_ = min(expireTime, arbitrationStartTime_) + config.ArbitrationTimeout;
                ++predArbitration_;
                actions.Add(make_unique<ImplicitArbitrationAction>(ArbitrationType::Implicit, pred_, predArbitration_, now > succLease_ && succArbitration_ == 0 ? 1 : succArbitration_));
                WriteInfo(TraceImplicitLease, site_.IdString, "Start pred implicit arbitration with {0}: expire {1}/{2}, predLease: {3}, cycleStart: {4}, count={5}",
                    predInstance_, monitorExpire, subjectExpire, predLease_, cycleStartTime_, predArbitration_);
            }
            else if (queryStartTime_ == StopwatchTime::Zero &&
                now > monitorExpire + config.MaxArbitrationTimeout &&
                now > max(predLease_, predStartTime_) + queryInterval + config.MaxLeaseDuration &&
                now > predQueryResult_ + queryInterval + config.LeaseSuspendTimeout &&
                pred_->IsExtendedArbitrationEnabled())
            {
                queryStartTime_ = now;
                predQuery_ = true;
                actions.Add(make_unique<ImplicitArbitrationAction>(ArbitrationType::Query, pred_, predArbitration_, succArbitration_));
                WriteInfo(TraceImplicitLease, site_.IdString, "Start pred arbitration query {0}: expire: {1}/{2}, last query result: {3}, pred start: {4}",
                    predInstance_, monitorExpire, subjectExpire, predQueryResult_, predStartTime_);
            }
            else if (now > predLease_)
            {
                WriteNoise(TraceNeighborLease, site_.IdString, "Pred {0}: expire {1}/{2}, predLease: {3}, cycleStart: {4}, last query result: {5}, pred start: {6}",
                    predInstance_, monitorExpire, subjectExpire, predLease_, cycleStartTime_, predQueryResult_, predStartTime_);
            }
        }
    }
}
