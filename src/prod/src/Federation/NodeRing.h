// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class NodeRingBase
    {
        DENY_COPY(NodeRingBase);

    public:
        NodeRingBase()
        {
        }

        NodeRingBase(NodeRingBase && other)
            : ring_(std::move(other.ring_))
        {
        }

        /// <summary>
        /// Return the size of the ring
        /// </summary>
        __declspec (property(get=getSize)) size_t Size;
        size_t getSize() const { return ring_.size(); }

        /// <summary>
        /// Get a node at specified position
        /// </summary>
        /// <param name="position">Position of the node</param>
        /// <returns>The node pointer</returns>
        PartnerNodeSPtr const& GetNode(size_t position) const;

        /// <summary>
        /// Get the predecessor of a specified position
        /// </summary>
        size_t GetPred(size_t position) const;

        /// <summary>
        /// Get the successor of a specified position
        /// </summary>
        size_t GetSucc(size_t position) const;

        /// <summary>
        /// Find the same node or the successor node for a given value using binary search
        /// The returned value should actually point to an existing element
        /// </summary>
        /// <param name="value">The value to be search for</param>
        /// <returns>The index of the successor node</returns>
        size_t FindSuccOrSamePosition(NodeId const& value) const;

        /// <summary>
        /// Internal function to find the lowerbound of a given value using binary search
        /// This could return a position point to one beyond the last element
        /// </summary>
        /// <param name="value">The value to be search for</param>
        /// <returns>The index of the lower bound</returns>
        size_t LowerBound(NodeId const& value) const;

        void GetNodesInRange(NodeIdRange const & range, std::vector<NodeInstance> & nodes) const;

        size_t GetNodesInRange(NodeIdRange const & range, size_t & start, size_t & end) const;

        int GetRoutingNodeCount() const;

        /// <summary>
        /// Add a node to the ring
        /// </summary>
        /// <param name="node">The node to be added</param>
        /// <returns>The position it is added</returns>
        size_t AddNode(PartnerNodeSPtr const& node);

        /// <summary>
        /// Add a node to the specified position of the ring
        /// </summary>
        /// <param name="node">The node to be added</param>
        /// <param name="position">The position of the node that is 
        /// supposed to be the successor of the node to be added</param>
        /// <returns>The position it is added</returns>
        size_t AddNode(PartnerNodeSPtr const& node, size_t position);

        /// <summary>
        /// Remove a node at specified position
        /// </summary>
        /// <param name="position">The position to be removed</param>
        void RemoveNode(size_t position);

        /// <summary>
        /// Replace a node at the specified position
        /// </summary>
        /// <param name="position">The position to be replaced</param>
        /// <param name="newNode">The new node</param>
        void ReplaceNode(size_t position, PartnerNodeSPtr const& newNode);

        /// <summary>
        /// Clear the ring.
        /// </summary>
        virtual void Clear();

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    protected:
        size_t Count(size_t start, size_t end) const
        {
            return (end >= start ? end - start : end + ring_.size() - start);
        }

        /// <summary>
        /// Function to call after a node is added
        /// </summary>
        /// <param name="position">The position that the node is added</param>
        virtual void OnNodeAdded(PartnerNodeSPtr const&, size_t) { }

        /// <summary>
        /// Function to call after a node is replaced
        /// </summary>
        /// <param name="position">The position that the node is replaced</param>
        virtual void OnNodeReplaced(PartnerNodeSPtr const&, PartnerNodeSPtr const&, size_t) { }

        /// <summary>
        /// Function to call after a node is removed
        /// </summary>
        /// <param name="position">The position that the node is replaced</param>
        virtual void OnNodeRemoved(PartnerNodeSPtr const&, size_t) { }

        /// <summary>
        /// The ring data structure
        /// </summary>
        std::vector<PartnerNodeSPtr> ring_;
    };

    /// <summary>
    /// The class to maintain a ring of nodes.
    /// </summary>
    /// <remarks>
    /// The nodes are stored in a vector and sorted by the node id. 
    /// The end of the vector is connected to the start of the vector,
    /// forming a ring. 
    /// </remarks>
    class NodeRing : public NodeRingBase
    {
        DENY_COPY(NodeRing);

    public:

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="node">The first node to insert</param>
        NodeRing(PartnerNodeSPtr const & node);

        /// <summary>
        /// Get the index of this node
        /// </summary>
        __declspec (property(get=getThisNode)) size_t ThisNode;

        /// <summary>
        /// Get the shared pointer of this node
        /// </summary>
        __declspec (property(get=getThisNodePtr)) PartnerNodeSPtr const & ThisNodePtr;

        /// <summary>
        /// Return the successor to this node in the ring.
        /// </summary>
        __declspec (property(get=getNext)) PartnerNodeSPtr const & Next;

        /// <summary>
        /// Get the predecessor to this node in the ring.
        /// </summary>
        __declspec (property(get=getPrev)) PartnerNodeSPtr const & Prev;

        /// <summary>
        /// Clear the ring except "this" node
        /// </summary>
        virtual void Clear();

        size_t getThisNode() const { return thisNode_; }
        PartnerNodeSPtr const & getThisNodePtr() const { return GetNode(thisNode_); }

        PartnerNodeSPtr const & getPrev() const { return ring_[GetPred(thisNode_)]; } 
        PartnerNodeSPtr const & getNext() const { return ring_[GetSucc(thisNode_)]; }

        virtual void VerifyConsistency(NodeId id) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    protected:
        /// <summary>
        /// The index of this node
        /// </summary>
        size_t thisNode_;

        virtual void OnNodeAdded(PartnerNodeSPtr const&, size_t);

        virtual void OnNodeRemoved(PartnerNodeSPtr const&, size_t);

        void BasicVerify(NodeId id) const;
    };

    /// <summary>
    /// The class to maintain a ring of nodes as well as a neighborhood range
    /// </summary>
    /// <remarks>
    /// A hood range is the range that we have the tight knowledge about. That
    /// means that all nodes in the range are in the data structure. It is defined 
    /// by predecessor hood edge and the successor hood edge. If the two edge meet,
    /// that means we have the complete knowledge of the whole ring
    /// </remarks>
    class NodeRingWithHood : public NodeRing
    {
        DENY_COPY(NodeRingWithHood);

    public:
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="thisNode">The pointer to this node</param>
        NodeRingWithHood(PartnerNodeSPtr const & thisNode, int hoodSize);

        /// <summary>
        /// Clear the ring except "this" node
        /// </summary>
        virtual void Clear();

        /// <summary>
        /// Get the neighborhood of the current node
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the neighborhood</param>
        NodeIdRange GetHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Get the immediate neighborhood and neighborhood edges
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the neighborhood</param>
        void GetPingTargets(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Get the predecessor neighborhood of the current node
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the predecessor neighborhood</param>
        void GetPredHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Get the successor neighborhood of the current node
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the successor neighborhood</param>
        void GetSuccHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Add neigborhood headers to the message.
        /// </summary>
        /// <param name="msg">The message object.</param>
        /// <param name="addRangeHeader">Whether to add the range header.</param>
        /// <param name="addExtendedNeighborhood">Whether to use extended neighborhood.</param>
        /// <param name="addFullNeighborhood">Whether to add every node.</param>
        void AddNeighborHeaders(Transport::Message & msg, bool addRangeHeader, bool addExtendedNeighborhood, bool addFullNeighborhood) const;

        /// <summary>
        /// Get the number of predecessor hood
        /// </summary>
        __declspec (property(get=getPredHoodCount)) int PredHoodCount;

        /// <summary>
        /// Get the number of successor hood
        /// </summary>
        __declspec (property(get=getSuccHoodCount)) int SuccHoodCount;

        /// <summary>
        /// The index of the predecessor neighborhood edge
        /// </summary>
        __declspec (property(get=getPredHoodEdge)) size_t PredHoodEdge;

        /// <summary>
        /// The index of the successor neighborhood edge
        /// </summary>
        __declspec (property(get=getSuccHoodEdge)) size_t SuccHoodEdge;

        /// <summary>
        /// The id of the predecessor neighborhood edge
        /// </summary>
        __declspec (property(get=getPredHoodEdgeId)) NodeId PredHoodEdgeId;

        /// <summary>
        /// The id of the successor neighborhood edge
        /// </summary>
        __declspec (property(get=getSuccHoodEdgeId)) NodeId SuccHoodEdgeId;

        /// <summary>
        /// Whether hood range covers all the node
        /// </summary>
        __declspec (property(get=getCompleteHoodRange)) bool CompleteHoodRange;

        __declspec (property(get = getNeighborhoodExchangeInterval, put = setNeighborhoodExchangeInterval)) Common::TimeSpan NeighborhoodExchangeInterval;

        /// <summary>
        /// Extend the predecessor hood edge by one
        /// </summary>
        /// <returns>Whether the predecessor hood edge is extended</returns>
        bool ExtendPredHoodEdge();

        /// <summary>
        /// Extend the successor hood edge by one
        /// </summary>
        /// <returns>Whether the successor hood edge is extended</returns>
        bool ExtendSuccHoodEdge();

        bool CanShrinkPredHoodRange() const;

        /// <summary>
        /// Shrink the predecessor hood range until the hood count less 
        /// or equal to the specified number
        /// </summary>
        void ShrinkPredHoodRange();

        bool CanShrinkSuccHoodRange() const;

        /// <summary>
        /// Shrink the successor hood range until the hood count less 
        /// or equal to the specified number
        /// </summary>
        void ShrinkSuccHoodRange();

        /// <summary>
        /// Whether a position is within the predecessor hood range
        /// </summary>
        bool WithinPredHoodRange(size_t position) const;

        /// <summary>
        /// Whether a position is within the predecessor hood range
        /// </summary>
        bool WithinSuccHoodRange(size_t position) const;

        /// <summary>
        /// Whether a position is within the predecessor or successor hood range
        /// </summary>
        bool WithinHoodRange(NodeId const & id) const;

        /// <summary>
        /// Whether the node has complete neighborhood knowledge on its predecessor side.
        /// </summary>
        bool IsPredHoodComplete() const
        {
            return (predHoodCount_ >= hoodSize_ || completeHoodRange_);
        }

        /// <summary>
        /// Whether the node has complete neighborhood knowledge on its successor side.
        /// </summary>
        bool IsSuccHoodComplete() const
        {
            return (succHoodCount_ >= hoodSize_ || completeHoodRange_);
        }

        /// <summary>
        /// Set the hood range to complete knownledge
        /// </summary>
        void SetCompleteKnowledge();

        /// <summary>
        /// Break the complete hood range when the available node on each side
        /// exceed the specified number
        /// </summary>
        void BreakCompleteRing(int hoodPerSide);

        NodeIdRange GetRange() const;

        int GetHoodCount() const;
        int getPredHoodCount() const { return predHoodCount_; }
        int getSuccHoodCount() const { return succHoodCount_; }
        size_t getPredHoodEdge() const { return predHoodEdge_; }
        size_t getSuccHoodEdge() const { return succHoodEdge_; }
        NodeId getPredHoodEdgeId() const { return GetNode(predHoodEdge_)->Id; }
        NodeId getSuccHoodEdgeId() const { return GetNode(succHoodEdge_)->Id; }
        bool getCompleteHoodRange() const { return completeHoodRange_; }
        Common::TimeSpan getNeighborhoodExchangeInterval() const { return neighborhoodExchangeInterval_; }
        void setNeighborhoodExchangeInterval(Common::TimeSpan interval)
        {
            neighborhoodExchangeInterval_ = interval;
        }

        void SetUnknown(std::wstring const & address) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        virtual void VerifyConsistency(NodeId id) const;

    private:

        int hoodSize_;

        /// <summary>
        /// The index of the predecessor neighborhood edge
        /// </summary>
        size_t predHoodEdge_;

        /// <summary>
        /// The index of the successor neighborhood edge
        /// </summary>
        size_t succHoodEdge_;

        /// <summary>
        /// The number of neighborhood nodes in predecessor side, 
        /// only include those Available nodes
        /// </summary>
        int predHoodCount_;

        /// <summary>
        /// The number of neighborhood nodes in successor side, 
        /// only include those Available nodes
        /// </summary>
        int succHoodCount_;

        /// <summary>
        /// The number of available nodes in the ring, not including "this" node
        /// </summary>
        int availableNodeCount_;

        /// <summary>
        /// Whether hood range covers all the node
        /// </summary>
        bool completeHoodRange_;

        /// <summary>
        /// A hint for the number of neighborhood headers enumerated last time
        /// </summary>
        mutable int lastNeighborHoodHeaderCount_;

        // This map shadows the ring_ to be able to quickly convert a string address to collection
        // of PartnerNodes.  Since the node is alive in the vector, these can be pointer to avoid
        // interlocks but requires that the items must be removed from this map when removed from the vector.
        std::multimap<std::wstring, PartnerNode const *> addressToNodeMap_;

        Common::TimeSpan neighborhoodExchangeInterval_;

        /// <summary>
        /// Function to call after a node is added
        /// </summary>
        /// <param name="position">The position that the node is added</param>
        void OnNodeAdded(PartnerNodeSPtr const& node, size_t position);

        /// <summary>
        /// Function to call after a node is replaced
        /// </summary>
        /// <param name="position">The position that the node is replaced</param>
        void OnNodeReplaced(PartnerNodeSPtr const& oldNode, PartnerNodeSPtr const& newNode, size_t position);

        /// <summary>
        /// Function to call after a node is removed
        /// </summary>
        /// <param name="position">The position that the node is removed</param>
        void OnNodeRemoved(PartnerNodeSPtr const& node, size_t position);
   };

   class ExternalRing : public NodeRingBase
   {
        DENY_COPY(ExternalRing);

    public:
        ExternalRing(SiteNode & site, std::wstring const & ringName);
        ExternalRing(ExternalRing && other);
        void LoadVotes();
        void CheckHealth(StateMachineActionCollection & actions);

        PartnerNodeSPtr const & GetRoutingSeedNode() const;

    private:
        SiteNode & site_;
        std::wstring ringName_;
        VoteConfig votes_;
        size_t nextPingIndex_;
        Common::DateTime lastCheckTime_;
        Common::DateTime lastCompactTime_;
    };
}
