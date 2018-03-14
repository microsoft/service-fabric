// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;
    using namespace Transport;

    PartnerNodeSPtr const& NodeRingBase::GetNode(size_t position) const
    {
        return ring_[position];
    }

    size_t NodeRingBase::GetPred(size_t position) const
    {
        ASSERT_IF(position >= Size, "Invalid position to get predecessor");

        if (position == 0)
        {
            position = Size - 1;
        }
        else
        {
            position --;
        }

        return position;
    }

    size_t NodeRingBase::GetSucc(size_t position) const
    {
        ASSERT_IF(position >= Size, "Invalid position");

        if (position == Size - 1)
        {
            position = 0;
        }
        else
        {
            position ++;
        }

        return position;
    }

    size_t NodeRingBase::FindSuccOrSamePosition(NodeId const& value) const
    {
        size_t position = LowerBound(value);

        if (position == Size)
        {
            position = 0;
        }

        return position;
    }

    size_t NodeRingBase::LowerBound(NodeId const& value) const
    {
        auto iter = lower_bound(ring_.begin(), ring_.end(), value, 
            [](PartnerNodeSPtr const& nodePtr, NodeId const& node) -> bool
        {
            return (nodePtr->Instance.Id < node);
        });

        return iter - ring_.begin();
    }

    void NodeRingBase::GetNodesInRange(NodeIdRange const & range, std::vector<NodeInstance> & nodes) const
    {
        size_t index = FindSuccOrSamePosition(range.Begin);
        while (range.Contains(ring_[index]->Id) && nodes.size() <= ring_.size())
        {
            nodes.push_back(ring_[index]->Instance);
            index = GetSucc(index);
        }
    }

    size_t NodeRingBase::GetNodesInRange(NodeIdRange const & range, size_t & start, size_t & end) const
    {
        start = FindSuccOrSamePosition(range.Begin);
        if (!range.Contains(ring_[start]->Id))
        {
            return 0;
        }

        end = FindSuccOrSamePosition(range.End);
        if (!range.Contains(ring_[end]->Id))
        {
            end = GetPred(end);
        }

        return (end >= start ? end - start + 1 : end + ring_.size() - start + 1);
    }

    int NodeRingBase::GetRoutingNodeCount() const
    {
        int result = 0;
        for (size_t i = 0; i < ring_.size(); i++)
        {
            if (ring_[i]->IsRouting)
            {
                result++;
            }
        }

        return result;
    }


    size_t NodeRingBase::AddNode(PartnerNodeSPtr const& node)
    {
        size_t pos = LowerBound(node->Instance.Id);
        return AddNode(node, pos);
    }

    size_t NodeRingBase::AddNode(PartnerNodeSPtr const& node, size_t position)
    {
        // Assume the node id to be inserted is not same to
        // any existing node id. And assume the position to insert 
        // is the right position of the ring
        ASSERT_IF(position < Size && GetNode(position)->Instance.Id == node->Instance.Id, "Duplicate node id");

        // Check if we should insert the node to the end of vector
        if (Size > 0 && position == 0 && node->Instance.Id > GetNode(0)->Instance.Id)
        {
            position = Size;
        }

        ring_.insert(ring_.begin() + position, node);

        OnNodeAdded(node, position);

        return position;
    }

    void NodeRingBase::RemoveNode(size_t position)
    {
        PartnerNodeSPtr node = ring_[position];
        ring_.erase(ring_.begin() + position);

        OnNodeRemoved(node, position);
    }

    void NodeRingBase::ReplaceNode(size_t position, PartnerNodeSPtr const& newNode)
    {
        PartnerNodeSPtr oldNode = ring_[position];
        ring_[position] = newNode;

        OnNodeReplaced(oldNode, newNode, position);
    }

    void NodeRingBase::Clear()
    {
        ring_.clear();
    }

    void NodeRingBase::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write(ring_);
    }

    NodeRing::NodeRing(PartnerNodeSPtr const & node) : thisNode_(0)
    {
        ring_.push_back(node);
    }

    void NodeRing::OnNodeAdded(PartnerNodeSPtr const&, size_t position)
    {
        if (position <= thisNode_)
        {
            thisNode_++;
        }
    }

    void NodeRing::OnNodeRemoved(PartnerNodeSPtr const&, size_t position)
    {
        ASSERT_IF(position == thisNode_, "Invalid position to remove");
        if (position < thisNode_)
        {
            thisNode_--;
        }
    }

    void NodeRing::Clear()
    {
        PartnerNodeSPtr thisNode = ThisNodePtr;

        ring_.clear();
        ring_.push_back(thisNode);

        thisNode_ = 0;
    }

    void NodeRing::WriteTo(TextWriter& w, FormatOptions const& option) const
    {
        if (option.formatString == "l")
        {
            w.Write(ring_);
        }
        else
        {
            size_t maxNodeToDumpEachSide = 6;
            size_t count = maxNodeToDumpEachSide * 2 + 1;
            size_t index = 0;

            if (ring_.size() <= count)
            {
                count = ring_.size();
            }
            else
            {
                index = (thisNode_ >= maxNodeToDumpEachSide ? thisNode_ - maxNodeToDumpEachSide : thisNode_ + ring_.size() - maxNodeToDumpEachSide);
            }

            for (size_t i = 0; i < count - 1; i++, index = GetSucc(index))
            {
                w.Write(ring_[index]->Id);
                w.Write(L",");
            }

            w.Write(ring_[index]->Id);
        }
    }

    void NodeRing::VerifyConsistency(NodeId id) const
    {
        BasicVerify(id);

        for (size_t i = 0; i < ring_.size(); i++)
        {
            ASSERT_IF(i != thisNode_ && !ring_[i]->IsAvailable,
                                "not all nodes available in ring {0}", *this);
        }
    }

    void NodeRing::BasicVerify(NodeId id) const
    {
        ASSERT_IF(id != ring_[thisNode_]->Id, "thisNode index invalid");

        for (size_t i = 0; i < ring_.size() - 1; i++)
        {
            ASSERT_IF(ring_[i]->Id >= ring_[i + 1]->Id, "ring not sorted {0}", *this);
        }
    }

    // NodeRingWithHood members
    NodeIdRange NodeRingWithHood::GetRange() const
    {
        if (completeHoodRange_)
        {
            return NodeIdRange::Full;
        }
        
        return NodeIdRange(PredHoodEdgeId, SuccHoodEdgeId);
    }

    NodeRingWithHood::NodeRingWithHood(PartnerNodeSPtr const & thisNode, int hoodSize)
      : NodeRing(thisNode),
        hoodSize_(hoodSize),
        predHoodEdge_(0), succHoodEdge_(0),
        predHoodCount_(0), succHoodCount_(0),
        availableNodeCount_(0), completeHoodRange_(false),
        lastNeighborHoodHeaderCount_(0),
        neighborhoodExchangeInterval_(FederationConfig::GetConfig().NeighborhoodExchangeTimeout)
    {
        ASSERT_IF(thisNode->IsUnknown, "This node should not be unknown");
        addressToNodeMap_.insert(make_pair(thisNode->Address, thisNode.get()));
    }

    void NodeRingWithHood::Clear()
    {
        NodeRing::Clear();

        predHoodEdge_ = 0;
        succHoodEdge_ = 0;
        predHoodCount_ = 0;
        succHoodCount_ = 0;
        availableNodeCount_ = 0;
        completeHoodRange_ = false;

        addressToNodeMap_.clear();
        addressToNodeMap_.insert(make_pair(ThisNodePtr->Address, ThisNodePtr.get()));
    }

    void NodeRingWithHood::OnNodeAdded(PartnerNodeSPtr const& node, size_t position)
    {
        NodeRing::OnNodeAdded(node, position);

        addressToNodeMap_.insert(make_pair(node->Address, node.get()));

        ASSERT_IF(addressToNodeMap_.size() != ring_.size(), "Collections should be the same size");

        if (!completeHoodRange_)
        {
            // adjust hood edges, here we simply extend the hood range if it is
            // added within the hood range.
            if (position <= predHoodEdge_)
            {
                // increment pred hood edge to make it point to the same node
                predHoodEdge_ ++;
            }
            if (position <= succHoodEdge_)
            {
                // increment succ hood edge to make it point to the same node
                succHoodEdge_ ++;
            }

            // adjust predecessor hood count and successor hood count
            if (node->IsAvailable)
            {
                availableNodeCount_ ++;
                // it's not possible a node is in both pred hood and succ hood 
                // because complete coverage will be treated separatedly
                if (WithinPredHoodRange(position))
                {
                    predHoodCount_++;
                }
                else if (WithinSuccHoodRange(position))
                {
                    succHoodCount_ ++;
                }
            }
        }
        else
        {
            predHoodEdge_ = succHoodEdge_ = thisNode_;
            if (node->IsAvailable)
            {
                availableNodeCount_ ++;
                predHoodCount_ = availableNodeCount_;
                succHoodCount_ = availableNodeCount_;
            }
        }
    }

    void NodeRingWithHood::OnNodeReplaced(PartnerNodeSPtr const& oldNode, PartnerNodeSPtr const& newNode, size_t position)
    {
        // don't adjust predecessor and succcessor hood edge when replacing a node
        // but only adjust the xxxxHoodCount
        if (oldNode->IsAvailable && !newNode->IsAvailable)
        {
            availableNodeCount_--;
            if (WithinPredHoodRange(position))
            {
                ASSERT_IF(predHoodCount_ == 0, "predHoodCount should be larger than 0");
                predHoodCount_ --;
            }
            if (WithinSuccHoodRange(position))
            {
                ASSERT_IF(succHoodCount_ == 0, "succHoodCount should be larger than 0");
                succHoodCount_ --;
            }
        }
        else if (!oldNode->IsAvailable && newNode->IsAvailable)
        {
            availableNodeCount_ ++;
            if (WithinPredHoodRange(position))
            {
                predHoodCount_ ++;
            }
            if (WithinSuccHoodRange(position))
            {
                succHoodCount_ ++;
            }
        }

        auto start = this->addressToNodeMap_.lower_bound(oldNode->Address);
        auto end = this->addressToNodeMap_.upper_bound(oldNode->Address);

        for (start; start != end; ++start)
        {
            if (start->second == oldNode.get())
            {
                this->addressToNodeMap_.erase(start);
                break;
            }
        }

        addressToNodeMap_.insert(make_pair(newNode->Address, newNode.get()));
    }

    void NodeRingWithHood::OnNodeRemoved(PartnerNodeSPtr const& node, size_t position)
    {
        ASSERT_IF(position == predHoodEdge_ || position == succHoodEdge_,
            "{0} removing edge node {1}", ThisNodePtr->Id, node->Id);

        NodeRing::OnNodeRemoved(node, position);

        if (position < predHoodEdge_)
        {
            predHoodEdge_--;
        }

        if (position < succHoodEdge_)
        {
            succHoodEdge_--;
        }

        if (node->IsAvailable)
        {
            availableNodeCount_--;
        }

        auto start = this->addressToNodeMap_.lower_bound(node->Address);
        auto end = this->addressToNodeMap_.upper_bound(node->Address);

        for (start; start != end; ++start)
        {
            if (start->second == node.get())
            {
                this->addressToNodeMap_.erase(start);
                break;
            }
        }

        ASSERT_IF(addressToNodeMap_.size() != ring_.size(), "Collections should be the same size");
    }

    void NodeRingWithHood::BreakCompleteRing(int hoodPerSide)
    {
        ASSERT_IF(!completeHoodRange_, "This should be called when range is complete");

        if (availableNodeCount_ >= 2 * hoodPerSide)
        {
            predHoodCount_ = 0;
            predHoodEdge_ = thisNode_;
            while (predHoodCount_ < hoodPerSide)
            {
                predHoodEdge_ = GetPred(predHoodEdge_);
                if (GetNode(predHoodEdge_)->IsAvailable)
                {
                    predHoodCount_ ++;
                }
            }

            succHoodCount_ = 0;
            succHoodEdge_ = thisNode_;
            while (succHoodCount_ < hoodPerSide)
            {
                succHoodEdge_ = GetSucc(succHoodEdge_);
                if (GetNode(succHoodEdge_)->IsAvailable)
                {
                    succHoodCount_ ++;
                }
            }

            completeHoodRange_ = false;
        }
    }

    int NodeRingWithHood::GetHoodCount() const
    {
        if (completeHoodRange_)
        {
            return static_cast<int>(ring_.size() - 1);
        }

        if (predHoodEdge_ <= succHoodEdge_)
        {
            return static_cast<int>(succHoodEdge_ - predHoodEdge_);
        }
        else
        {
            return static_cast<int>(succHoodEdge_ + ring_.size() - predHoodEdge_);
        }
    }

    NodeIdRange NodeRingWithHood::GetHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        GetPredHood(vecNode);
        if (!completeHoodRange_)
        {
            GetSuccHood(vecNode);
        }

        return GetRange();
    }

    void NodeRingWithHood::GetPingTargets(vector<PartnerNodeSPtr>& vecNode) const
    {
        size_t edge, edgeHood, immediateHood;

        edge = (completeHoodRange_ ? GetSucc(predHoodEdge_) : predHoodEdge_);
        edgeHood = thisNode_;
        immediateHood = thisNode_;

        while (edge != thisNode_)
        {
            PartnerNodeSPtr const& currentNode = GetNode(edge);
            if (currentNode->IsAvailable)
            {
                immediateHood = edge;
                if (edgeHood == thisNode_)
                {
                    edgeHood = edge;
                }
            }
            edge = GetSucc(edge);
        }

        if (edgeHood != thisNode_)
        {
            vecNode.push_back(GetNode(edgeHood));
            if (immediateHood != edgeHood)
            {
                vecNode.push_back(GetNode(immediateHood));
            }
        }

        if (completeHoodRange_)
        {
            return;
        }

        edge = succHoodEdge_;
        edgeHood = thisNode_;
        immediateHood = thisNode_;

        while (edge != thisNode_)
        {
            PartnerNodeSPtr const& currentNode = GetNode(edge);
            if (currentNode->IsAvailable)
            {
                immediateHood = edge;
                if (edgeHood == thisNode_)
                {
                    edgeHood = edge;
                }
            }
            edge = GetPred(edge);
        }

        if (edgeHood != thisNode_)
        {
            vecNode.push_back(GetNode(edgeHood));
            if (immediateHood != edgeHood)
            {
                vecNode.push_back(GetNode(immediateHood));
            }
        }
    }

    void NodeRingWithHood::GetPredHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        size_t edge = (completeHoodRange_ ? GetSucc(predHoodEdge_) : predHoodEdge_);
        while (edge != thisNode_)
        {
            vecNode.push_back(GetNode(edge));
            edge = GetSucc(edge);
        }
    }

    void NodeRingWithHood::GetSuccHood(vector<PartnerNodeSPtr>& vecNode) const
    {
        size_t edge = (completeHoodRange_ ? GetPred(succHoodEdge_) : succHoodEdge_);
        while (edge != thisNode_)
        {
            vecNode.push_back(GetNode(edge));
            edge = GetPred(edge);
        }
    }

    void NodeRingWithHood::AddNeighborHeaders(Message & msg, bool addRangeHeader,  bool addExtendedNeighborhood, bool addFullNeighborhood) const
    {
        FederationConfig const & config = FederationConfig::GetConfig();

        StopwatchTime bound = Stopwatch::Now() - neighborhoodExchangeInterval_;
        int maxQuota = config.MaxNeighborhoodHeaders;
        int directQuota = (maxQuota >> 1);
        int addedCount = 0;
        vector<PartnerNode const *> extraHeaders;
        bool useExtraHeaders = (lastNeighborHoodHeaderCount_ > directQuota);
        if (useExtraHeaders)
        {
            extraHeaders.reserve(lastNeighborHoodHeaderCount_ + directQuota);
        }

        size_t predEnd, succEnd;
        if (addFullNeighborhood)
        {
            predEnd = GetSucc(thisNode_);
            succEnd = thisNode_;
        }
        else if (addExtendedNeighborhood)
        {
            predEnd = succEnd = thisNode_;
        }
        else
        {
            predEnd = predHoodEdge_;
            succEnd = succHoodEdge_;
        }

        int availableCount = 0;
        size_t edge = thisNode_;
        do
        {
            edge = GetPred(edge);
            PartnerNodeSPtr const & neighbor = GetNode(edge);

            if (neighbor->IsAvailable)
            {
                ++addedCount;
                ++availableCount;
                msg.Headers.Add(FederationPartnerNodeHeader(*neighbor));
            }
            else if (neighbor->LastAccessed > bound)
            {
                if (!useExtraHeaders && addedCount > directQuota)
                {
                    useExtraHeaders = true;
                    extraHeaders.reserve(directQuota);
                }

                if (useExtraHeaders)
                {
                    extraHeaders.push_back(neighbor.get());
                }
                else
                {
                    ++addedCount;
                    msg.Headers.Add(FederationPartnerNodeHeader(*neighbor));
                }
            }
        } while (edge != predEnd && (availableCount < hoodSize_ || addFullNeighborhood));

        if (!addFullNeighborhood)
        {
            availableCount = 0;
            edge = thisNode_;
            do
            {
                edge = GetSucc(edge);

                PartnerNodeSPtr const & neighbor = GetNode(edge);
                if (neighbor->IsAvailable)
                {
                    ++addedCount;
                    ++availableCount;
                    msg.Headers.Add(FederationPartnerNodeHeader(*neighbor));
                }
                else if (neighbor->LastAccessed > bound)
                {
                    if (!useExtraHeaders && addedCount > directQuota)
                    {
                        useExtraHeaders = true;
                        extraHeaders.reserve(directQuota);
                    }

                    if (useExtraHeaders)
                    {
                        extraHeaders.push_back(neighbor.get());
                    }
                    else
                    {
                        ++addedCount;
                        msg.Headers.Add(FederationPartnerNodeHeader(*neighbor));
                    }
                }
            } while (edge != succEnd && availableCount < hoodSize_);
        }

        int extraCount = static_cast<int>(extraHeaders.size());

        // This is a hint only so no write lock protection is needed.
        lastNeighborHoodHeaderCount_ = addedCount + extraCount;

        if (useExtraHeaders)
        {
            int addCount = maxQuota - addedCount;
            if (extraCount > addCount)
            {
                sort(extraHeaders.begin(), extraHeaders.end(),
                    [](PartnerNode const * left, PartnerNode const * right) { return left->LastAccessed > right->LastAccessed; });
            }
            else
            {
                addCount = extraCount;
            }

            for (int i = 0; i < addCount; ++i)
            {
                msg.Headers.Add(FederationPartnerNodeHeader(*extraHeaders[i]));
            }
        }

        if (addRangeHeader)
        {
            for (auto i = msg.Headers.Begin(); i != msg.Headers.End(); ++i)
            {
                if (i->Id() == MessageHeaderId::FederationNeighborhoodRange)
                {
                    msg.Headers.Remove(i);
                    break;
                }
            }
            msg.Headers.Add(FederationNeighborhoodRangeHeader(GetRange()));
        }
    }

    bool NodeRingWithHood::ExtendPredHoodEdge()
    {
        if (completeHoodRange_)
        {
            return false;
        }

        predHoodEdge_ = GetPred(predHoodEdge_);
        if (predHoodEdge_ == succHoodEdge_)
        {
            SetCompleteKnowledge();
        }
        else if (GetNode(predHoodEdge_)->IsAvailable)
        {
            predHoodCount_++;
        }

        return true;
    }

    bool NodeRingWithHood::ExtendSuccHoodEdge()
    {
        if (completeHoodRange_)
        {
            return false;
        }

        succHoodEdge_ = GetSucc(succHoodEdge_);
        if (predHoodEdge_ == succHoodEdge_)
        {
            SetCompleteKnowledge();
        }
        else if (GetNode(succHoodEdge_)->IsAvailable)
        {
            succHoodCount_++;
        }

        return true;
    }

    bool NodeRingWithHood::CanShrinkPredHoodRange() const
    {
        return (predHoodCount_ > hoodSize_) ||
               (predHoodCount_ == hoodSize_ && !GetNode(predHoodEdge_)->IsAvailable);
    }

    bool NodeRingWithHood::CanShrinkSuccHoodRange() const
    {
        return (succHoodCount_ > hoodSize_) ||
               (succHoodCount_ == hoodSize_ && !GetNode(succHoodEdge_)->IsAvailable);
    }

    void NodeRingWithHood::ShrinkPredHoodRange()
    {
        if (GetNode(predHoodEdge_)->IsAvailable)
        {
            predHoodCount_--;
        }

        predHoodEdge_ = GetSucc(predHoodEdge_);
    }

    void NodeRingWithHood::ShrinkSuccHoodRange()
    {
        if (GetNode(succHoodEdge_)->IsAvailable)
        {
            succHoodCount_--;
        }

        succHoodEdge_ = GetPred(succHoodEdge_);
    }

    bool NodeRingWithHood::WithinPredHoodRange(size_t position) const
    {
        if (position == thisNode_)
        {
            return false;
        }

        if (predHoodEdge_ < thisNode_)
        {
            return (predHoodEdge_ <= position && position < thisNode_);
        }
        else if (predHoodEdge_ > thisNode_)
        {
            return (predHoodEdge_ <= position || position < thisNode_);
        }
        else
        {
            // pred hood equals to this node, it's either the hood is empty
            // or the hood cover all nodes
            return (completeHoodRange_);
        }
    }

    bool NodeRingWithHood::WithinSuccHoodRange(size_t position) const
    {
        if (position == thisNode_)
        {
            return false;
        }

        if (thisNode_ < succHoodEdge_)
        {
            return (thisNode_ < position && position <= succHoodEdge_);
        }
        else if (thisNode_ > succHoodEdge_)
        {
            return (thisNode_ < position || position <= succHoodEdge_);
        }
        else
        {
            // this node equals to succ hood, it's either the hood is empty
            // or the hood cover all nodes
            return (completeHoodRange_);
        }
    }

    bool NodeRingWithHood::WithinHoodRange(NodeId const & id) const
    {
        return GetRange().Contains(id);
    }

    void NodeRingWithHood::SetCompleteKnowledge()
    {
        if (!completeHoodRange_)
        {
            completeHoodRange_ = true;
            predHoodEdge_ = thisNode_;
            succHoodEdge_ = thisNode_;
            predHoodCount_ = availableNodeCount_;
            succHoodCount_ = availableNodeCount_;
        }
    }

    void NodeRingWithHood::SetUnknown(std::wstring const & address) const
    {
        auto start = this->addressToNodeMap_.lower_bound(address);
        auto end = this->addressToNodeMap_.upper_bound(address);

        for (start; start != end; ++start)
        {
            start->second->SetAsUnknown();
        }
    }

    void NodeRingWithHood::WriteTo(TextWriter& w, FormatOptions const& option) const
    {
        if (option.formatString != "l")
        {
            w.Write(ring_[predHoodEdge_]->Id);

            for (size_t i = GetSucc(predHoodEdge_); i != succHoodEdge_; i = GetSucc(i))
            {
                if (ring_[i]->IsAvailable)
                {
                    w.Write(L",");
                    w.Write(ring_[i]->Id);
                }
            }

            if (predHoodEdge_ != succHoodEdge_)
            {
                w.Write(L",");
                w.Write(ring_[succHoodEdge_]->Id);
            }
        }
        else
        {
            w.Write(ring_);
            w.Write("\r\ntotalNode:{0},totalAvailNode:{1},isComplete:{2},range:{3}-{4},hoodCount:{5}/{6}",
                ring_.size(), availableNodeCount_, completeHoodRange_,
                ring_[predHoodEdge_]->Id, ring_[succHoodEdge_]->Id,
                predHoodCount_, succHoodCount_);
        }
    }

    void NodeRingWithHood::VerifyConsistency(NodeId id) const
    {
        BasicVerify(id);

        size_t availableCount = count_if(ring_.begin(), ring_.end(), [id](PartnerNodeSPtr const & x){ return x->IsAvailable && x->Id != id; });
        ASSERT_IF(availableCount != static_cast<size_t>(availableNodeCount_),
            "Invalid available count: {0}/{1}, ring: {2}",
            availableNodeCount_, availableCount, *this);

        if (completeHoodRange_)
        {
            ASSERT_IF(predHoodEdge_ != thisNode_ || succHoodEdge_ != thisNode_,
                "Complete knowledge with invalid edges: predEdge:{0},this:{1},succEdge:{2}",
                predHoodEdge_, thisNode_, succHoodEdge_);

            ASSERT_IF(predHoodCount_ != availableNodeCount_ ||
                                succHoodCount_ != availableNodeCount_ ||
                                availableNodeCount_ >= hoodSize_ * 2,
                                "Complete knowledge with invalid counts: totalAvailNode:{0},predHood:{1},succHood:{2}",
                                availableNodeCount_, predHoodCount_, succHoodCount_);
        }
        else
        {
            int predCount = 0;
            for (size_t i = predHoodEdge_; i != thisNode_; i = GetSucc(i))
            {
                if (ring_[i]->IsAvailable)
                {
                    predCount++;
                }
            }

            int succCount = 0;
            for (size_t i = succHoodEdge_; i != thisNode_; i = GetPred(i))
            {
                if (ring_[i]->IsAvailable)
                {
                    succCount++;
                }
            }

            ASSERT_IF(Count(predHoodEdge_, thisNode_) + Count(thisNode_, succHoodEdge_) + 1 > ring_.size(),
                "Invalid hood edges: predEdge:{0},this:{1},succEdge:{2}",
                predHoodEdge_, thisNode_, succHoodEdge_);

            ASSERT_IF(predHoodCount_ > hoodSize_ || succHoodCount_ > hoodSize_,
                "Invalid hood count: predHood:{0},succHood:{1}",
                predHoodCount_, succHoodCount_);
        }
    }

    ExternalRing::ExternalRing(SiteNode & site, wstring const & ringName)
        : site_(site), ringName_(ringName), nextPingIndex_(0), lastCheckTime_(DateTime::Zero), lastCompactTime_(DateTime::Zero)
    {
    }

    ExternalRing::ExternalRing(ExternalRing && other)
        : site_(other.site_), ringName_(move(other.ringName_)), votes_(move(other.votes_)), nextPingIndex_(other.nextPingIndex_), lastCheckTime_(other.lastCheckTime_), lastCompactTime_(other.lastCompactTime_)
    {
    }

    void ExternalRing::LoadVotes()
    {
        votes_.clear();

        VoteConfig const & config = FederationConfig::GetConfig().Votes;
        for (auto it = config.begin(); it != config.end(); it++)
        {
            if (it->RingName == ringName_)
            {
                ASSERT_IF(it->Type == Constants::SqlServerVoteType,
                    "SQL voter not supported in multi-ring environment");
                votes_.push_back(*it);
            }
        }

        sort(votes_.begin(), votes_.end(), 
            [this] (VoteEntryConfig const & vote1, VoteEntryConfig const & vote2) -> bool
            {
                return this->site_.Id.MinDist(vote1.Id) < this->site_.Id.MinDist(vote2.Id);
            });

        if (nextPingIndex_ >= votes_.size())
        {
            nextPingIndex_ = 0;
        }
    }

    bool ComparePartnerNode(PartnerNodeSPtr const & node1, PartnerNodeSPtr const & node2)
    {
        if (node1->IsRouting != node2->IsRouting)
        {
            return node1->IsRouting;
        }

        if (node1->IsShutdown != node2->IsShutdown)
        {
            return node2->IsShutdown;
        }

        if (node1->IsUnknown != node2->IsUnknown)
        {
            return node2->IsUnknown;
        }

        return (node1->LastAccessed > node2->LastAccessed);
    }

    PartnerNodeSPtr const & ExternalRing::GetRoutingSeedNode() const
    {
        if (Size > 0)
        {
            for (auto it = votes_.begin(); it != votes_.end(); ++it)
            {
                size_t position = FindSuccOrSamePosition(it->Id);
                PartnerNodeSPtr const & node = GetNode(position);
                if (node->IsRouting && !node->IsUnknown)
                {
                    return node;
                }
            }
        }

        return RoutingTable::NullNode;
    }

    void ExternalRing::CheckHealth(StateMachineActionCollection & actions)
    {
        FederationConfig & config = FederationConfig::GetConfig();
        if (Size > static_cast<size_t>(config.RoutingTableCapacity) && DateTime::Now() > lastCompactTime_ + config.RoutingTableCompactInterval)
        {
            nth_element(ring_.begin(), ring_.begin() + config.RoutingTableCapacity, ring_.end(), ComparePartnerNode);
            ring_.erase(ring_.begin() + config.RoutingTableCapacity, ring_.end());
            lastCompactTime_ = DateTime::Now();
        }

        PartnerNodeSPtr const & result = GetRoutingSeedNode();
        if (result)
        {
            return;
        }

        if (DateTime::Now() < lastCheckTime_ + config.ExternalRingPingInterval)
        {
            return;
        }

        lastCheckTime_ = DateTime::Now();

        auto it = votes_.begin() + nextPingIndex_;
        ++nextPingIndex_;
        if (nextPingIndex_ >= votes_.size())
        {
            nextPingIndex_ = 0;
        }

        PartnerNodeSPtr node;
        if (it->Type == Constants::SeedNodeVoteType)
        {
            node = make_shared<PartnerNode>(NodeConfig(it->Id, it->ConnectionString, L"", L"", ringName_), site_);
        }
        else if (it->Type == Constants::WindowsAzureVoteType)
        {
            node = make_shared<PartnerNode>(WindowsAzureProxy::ResolveAzureEndPoint(it->Id, it->ConnectionString, it->RingName, site_.Id), site_);
        }
        else
        {
            return;
        }

        MessageUPtr request = FederationMessage::GetExternalRingPing().CreateMessage();
        request->Idempotent = true;
        actions.Add(make_unique<SendMessageAction>(move(request), node));
    }
}
