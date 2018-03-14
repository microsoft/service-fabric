// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct FederationTraceProbeHeader;

    /// <summary>
    /// The class to maintain the list of partner nodes and neighborhood nodes and
    /// enable fast lookup of the closest node id to a value
    /// </summary>
    class RoutingTable : public Common::TextTraceComponent<Common::TraceTaskCodes::RoutingTable>
    {
        DENY_COPY(RoutingTable)

        friend class GlobalTimeManager;
    public:

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="site">The site node that owns the routing table</param>
        /// <param name="hoodSize">The desired number of hood each side</param>
        RoutingTable(SiteNodeSPtr const& site, int hoodSize);

        /// <summary>
        /// Destructor
        /// </summary>
        ~RoutingTable();

		void Test_SetTestMode()
		{
			isTestMode_ = true;
		}

        /// <summary>
        /// Get the size of the routing table (all nodes including "this" node and shutdown ones)
        /// </summary>
        __declspec (property(get=getSize)) size_t Size;
        size_t getSize() const { return knownTable_.Size; }

        __declspec (property(get=getUpNodes)) size_t UpNodes;
        size_t getUpNodes() const { return ring_.Size - 1; }

        int GetRoutingNodeCount() const;

        /// <summary>
        /// Find the PartnerNode that have the closest id with the input
        /// </summary>
        /// <param name="value">The value to search</param>
        /// <returns>The pointer to the partner node</returns>
        PartnerNodeSPtr FindClosest(NodeId const& value, std::wstring const & toRing) const;

        PartnerNodeSPtr GetRoutingHop(NodeId const& value, std::wstring const & ringName, bool safeMode, bool& ownsToken) const;

        /// <summary>
        /// Get the PartnerNode with the id in the input.
        /// </summary>
        /// <param name="value">The instance of the node.</param>
        /// <returns>The pointer to the partner node if it exists, else returns null.</returns>
        PartnerNodeSPtr Get(NodeInstance const & value) const;
        PartnerNodeSPtr Get(NodeInstance const & value, std::wstring const & ringName) const;

        /// <summary>
        /// Get the Predecessor of the current node
        /// </summary>
        /// <returns>The pointer to the partner node if it exists, else returns null.</returns>
        PartnerNodeSPtr GetPredecessor() const;

        /// <summary>
        /// Get the Successor of the current node
        /// </summary>
        /// <returns>The pointer to the partner node if it exists, else returns null.</returns>
        PartnerNodeSPtr GetSuccessor() const;

        /// <summary>
        /// Get the PartnerNode with the id in the input.
        /// </summary>
        /// <param name="value">The id of the node.</param>
        /// <returns>The pointer to the partner node if it exists, else returns null.</returns>
        PartnerNodeSPtr Get(NodeId value) const;
        PartnerNodeSPtr Get(NodeId value, std::wstring const & ringName) const;

        /// <summary>
        /// Consider update or insert the partner node
        /// </summary>
        /// <param name="node">The information of the partner node</param>
        /// <param name="isInserting">Whether or not this is the initial insertion of the PartnerNode</param>
        /// <param name="ringName">Name of the ring of the node.</param>
        void Consider(FederationPartnerNodeHeader const & nodeInfo, bool isInserting = false);

        PartnerNodeSPtr ConsiderAndReturn(FederationPartnerNodeHeader const & nodeInfo, bool isInserting = false);

        /// <summary>
        /// Get the neighborhood of the current node
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the neighborhood</param>
        NodeIdRange GetHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Get the neighborhood range of the current node
        /// </summary>
        NodeIdRange GetHoodRange() const;

        /// <summary>
        /// Get the neighborhood range combined with the token range of the current node
        /// </summary>
        NodeIdRange GetCombinedNeighborHoodTokenRange() const;

        /// <summary>
        /// Get the immediate neighborhood and neighborhood edges
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the neighborhood</param>
        void GetPingTargets(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Get the full neighborhood (hoodSize_ nodes on each side) of the current node
        /// regardless of the neighborhood range.
        /// </summary>
        /// <param name="vecNode">The vector of partner node that which are the neighborhood</param>
        void GetExtendedHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        /// <summary>
        /// Set the specified node's state to shutdown
        /// </summary>
        /// <param name="shutdownNode">The node we know that is shutdown</param>
        void SetShutdown(PartnerNodeSPtr const& shutdownNode);

        /// <summary>
        /// Set the specified node's state to shutdown
        /// </summary>
        /// <param name="shutdownNode">The node instance we know that is shutdown</param>
        void SetShutdown(NodeInstance const& shutdownNode, std::wstring const & ringName);

        /// <summary>
        /// Set the specified node's state to unknown
        /// </summary>
        /// <param name="unknownNode">The node instance we know that is unknown</param>
        void SetUnknown(NodeInstance const& unknownNode, std::wstring const & ringName);

        /// <summary>
        /// Set the nodes with specified address to unknown
        /// </summary>
        /// <param name="unknownNode">The node instance we know that is unknown</param>
        void SetUnknown(std::wstring const& unreachableAddress);

        bool IsDown(NodeInstance const & nodeInstance) const;

        /// <summary>
        /// This is used for test only now to remove nodes outside of neighborhood range
        /// </summary>
        void TestCompact();

        // Use by FederationTest to check for stale information in routing table
        void Test_GetUpNodesOutsideNeighborhood(std::vector<NodeInstance> & nodes) const;

        /// <summary>
        /// Process the routing headers in the incoming message.
        /// </summary>
        PartnerNodeSPtr ProcessNeighborHeaders(Transport::Message & message, NodeInstance const & from, std::wstring const & fromRing, bool instanceMatched);

        PartnerNodeSPtr ProcessNodeHeaders(Transport::Message & message, NodeInstance const & from, std::wstring const & fromRing);

        /// <summary>
        /// Add the routing headers to the outgoing message.
        /// </summary>
        void AddNeighborHeaders(Transport::Message & message, bool isRingToRing);

        void UpdateNeighborhoodExchangeInterval(Common::TimeSpan interval);

        /// <summary>
        /// Set routing table as the first node in the ring.
        /// </summary>
        void Bootstrap();

        void SetInitialLeasePartners(std::vector<PartnerNodeSPtr> const & partners);

        /// <summary>
        /// Start routing stage state machine.
        /// </summary>
        void StartRouting();

        /// <summary>
        /// Change site node phase.
        /// </summary>
        /// <param name="phase">The new phase.</param>
        bool ChangePhase(NodePhase::Enum phase);

        void RestartInstance();

        /// <summary>
        /// Close the site node shared_ptr object
        /// </summary>
        void CloseSiteNode();

        #pragma region Token Related methods

        void TrySplitToken(PartnerNodeSPtr const & neighbor, std::vector<FederationRoutingTokenHeader> & tokens);

        void CreateDepartMessages(std::vector<Transport::MessageUPtr> & messages, std::vector<PartnerNodeSPtr> & targets);

        //---------------------------------------------------------------------
        // Description:
        //     Split the current token to pass to the neighbor if possible.
        //
        //  Arguments:
        //      neighbor: The neighbor to which to transfer the token
        //      ownerId: The id of the owner of this token.
        //      tokens: The vector to which the split token is to be added.
        void Split(std::shared_ptr<PartnerNode const> const & neighbor, NodeId const & ownerId, std::vector<FederationRoutingTokenHeader> & tokens, uint64 targetVersion);

        Transport::MessageUPtr GetProbeMessage(bool successorDirection, PartnerNodeSPtr & to);
        void EdgeProbeOneWayMessageHandler(__in Transport::Message & message, Federation::OneWayReceiverContext & oneWayReceiverContext);
        void RingAdjustOneWayMessageHandler(__in Transport::Message & message, Federation::OneWayReceiverContext & oneWayReceiverContext);

        void TokenTransferRequestMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void TokenTransferMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void TokenAcceptRejectMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void TokenProbeMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void TokenEchoMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void LivenessQueryRequestHandler(Transport::Message & message, RequestReceiverContext & requestReceiverContext);
        void LivenessUpdateMessageHandler(Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        bool CheckLiveness(FederationPartnerNodeHeader const & target);
        void CheckGapRecovery(NodeIdRange const & range);

        RoutingToken GetRoutingToken() const;
        RoutingToken GetRoutingTokenAndShutdownNodes(std::vector<NodeInstance>& shutdownNodes) const;

        bool LockRange(NodeInstance const & owner, PartnerNodeSPtr const & node, Common::StopwatchTime expiryTime, NodeIdRange & range, FederationPartnerNodeHeader & ownerHeader);
        void ReleaseRangeLock();

        void SetTokenAcquireDeadline(Common::StopwatchTime value) { tokenAcquireDeadline_ = value; }

        NodeIdRange PartitionRanges(NodeIdRange const & range, std::vector<PartnerNodeSPtr> & targets, std::vector<NodeIdRange> & subRanges, bool excludeNeighborhood) const;

        std::vector<NodeInstance> RemoveDownNodes(std::vector<NodeInstance> & nodes);

        void UpdateExternalRingVotes();
        void GetExternalRings(std::vector<std::wstring> & externalRings);

        void BecomeLeader();
        void AddGlobalTimeExchangeHeader(Transport::Message & message, PartnerNodeSPtr const & target);
        bool ProcessGlobalTimeExchangeHeader(Transport::Message & message, NodeInstance const & fromInstance);
        bool ProcessGlobalTimeExchangeHeader(Transport::Message & message, PartnerNodeSPtr const & from);
        int64 GetGlobalTimeInfo(int64 & lowerLimit, int64 & upperLimit, bool & isAuthority);
        bool HasGlobalTime() const;

        void ReportImplicitArbitrationResult(bool result);
        void ReportArbitrationQueryResult(std::wstring const & address, int64 instance, Common::StopwatchTime remoteResult);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const & options) const;

        static PartnerNodeSPtr const NullNode;

        #pragma region Token Related methods

    private:
        class WriteLock;

        // This list maintains all the effective echoed promise by
        // the node so that before token recovery we can check against
        // this list to see whether the recovery is going to break
        // any promise.
        class EchoedProbeList
        {
            // Every echoed token probe represent a promise by the echoing
            // node for performing token recovery with a node that is
            // further from the other party other such recovery is not
            // safe if the echo results in a recovery by the other party.
            class EchoedProbe
            {
            public:

                EchoedProbe(uint64 version, Common::LargeInteger distance) : version_(version), distance_(distance)
                {
                }

                uint64 GetVersion()
                {
                    return version_;
                }

                bool Closer(Common::LargeInteger distance)
                {
                    // Zero means the entire ring and hence larger than any other value.
                    if (distance_ == Common::LargeInteger::Zero)
                    {
                        return (false);
                    }

                    if (distance == Common::LargeInteger::Zero)
                    {
                        return (true);
                    }

                    // Now compare using normal logic.
                    return (distance_ < distance);
                }

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
                {
                    w.Write("{0},{1}", version_, distance_);
                }

            private:
                uint64 version_;
                Common::LargeInteger distance_;
            };

        public:
            EchoedProbeList() : echos_()
            {
            }

            void Add(uint64 version, Common::LargeInteger distance);
            bool CanRecover(uint64 version, Common::LargeInteger dist);
            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        private:
            std::vector<EchoedProbe> echos_;
        };

        class ImplicitLeaseContext
        {
            DENY_COPY(ImplicitLeaseContext);

        public:
            ImplicitLeaseContext(SiteNode & site);
            void Start();
            void Update(PartnerNodeSPtr const & succ, PartnerNodeSPtr const & pred);

            void ReportImplicitArbitrationResult(bool result);
            void ReportArbitrationQueryResult(std::wstring const & address, int64 instance, Common::StopwatchTime remoteResult);

            void RunStateMachine(StateMachineActionCollection & actions);

        private:
            void GetRemoteLeaseExpirationTime(PartnerNodeSPtr const & remote, Common::StopwatchTime & monitorExpire, Common::StopwatchTime & subjectExpire);

            SiteNode & site_;
            PartnerNodeSPtr succ_;
            NodeInstance succInstance_;
            Common::StopwatchTime succStartTime_;
            Common::StopwatchTime succLease_;
            Common::StopwatchTime succQueryResult_;
            int succArbitration_;
            bool succQuery_;
            PartnerNodeSPtr pred_;
            NodeInstance predInstance_;
            Common::StopwatchTime predStartTime_;
            Common::StopwatchTime predLease_;
            Common::StopwatchTime predQueryResult_;
            int predArbitration_;
            bool predQuery_;
            Common::StopwatchTime arbitrationStartTime_;
            Common::StopwatchTime arbitrationEndTime_;
            Common::StopwatchTime queryStartTime_;
            Common::StopwatchTime cycleStartTime_;
        };

        /// <summary>
        /// Keep the site node pointer
        /// </summary>
        SiteNode & site_;
        int hoodSize_;
        int safeHoodSize_;

        /// <summary>
        /// Keep the routing partners of this node, including this node. The nodes are sorted
        /// by their node id
        /// </summary>
        NodeRing ring_;

        /// <summary>
        /// Keep all known nodes of this node, including the routing and non-routing ones.
        /// The nodes are sorted by the node ids
        /// </summary>
        NodeRingWithHood knownTable_;

        std::map<std::wstring, ExternalRing> externalRings_;

        /// <summary>
        /// The lock object
        /// </summary>
        MUTABLE_RWLOCK(Federation.RoutingTable, lock_);

        Common::TimerSPtr timer_;

        Common::StopwatchTime lastSuccEdgeProbe_;
        Common::StopwatchTime lastPredEdgeProbe_;
        Common::StopwatchTime lastSuccTokenProbe_;
        Common::StopwatchTime lastPredTokenProbe_;

        ThrottleManager tokenProbeThrottle_;

        NodeId succProbeTarget_;
        NodeId predProbeTarget_;
        NodeId ackedProbe_;
        bool probeResult_;
        Common::StopwatchTime succRecoveryTime_;
        Common::StopwatchTime predRecoveryTime_;
        Common::StopwatchTime gapReportTime_;

        Common::StopwatchTime lastCompactTime_;
        Common::StopwatchTime lastTraceTime_;
        Common::StopwatchTime tokenAcquireDeadline_;

        uint neighborhoodVersion_;
        bool isRangeConsistent_;
        bool isNeighborLostReported_;
        Common::StopwatchTime rangeLockExpiryTime_;

        std::unique_ptr<FederationRoutingTokenHeader> succPendingTransfer_;
        std::unique_ptr<FederationRoutingTokenHeader> predPendingTransfer_;

        EchoedProbeList succEchoList_;
        EchoedProbeList predEchoList_;

        std::vector<NodeInstance> initialLeasePartners_;

        bool isHealthy_;
		bool isTestMode_;

        GlobalTimeManager globalTimeManager_;
        Common::StopwatchTime lastGlobalTimeUncertaintyIncreaseTime_;

        ImplicitLeaseContext implicitLeaseContext_;

        PartnerNodeSPtr const& InternalFindClosest(NodeId const& value, std::wstring const & toRing, bool safeMode) const;
        PartnerNodeSPtr const& InternalFindClosest(NodeId const& value, NodeRingBase const & ring) const;

        PartnerNodeSPtr const& InternalConsider(FederationPartnerNodeHeader const & nodeInfo, bool isInserting = false, int64 now = 0);
        PartnerNodeSPtr const& InternalConsiderExternalNode(FederationPartnerNodeHeader const & nodeInfo);

        /// <summary>
        /// Try to extend the neighborhood of current node when receiving
        /// a liveness header from the other node
        /// </summary>
        /// <param name="range">The range that the other node knows</param>
        /// <param name="shutdownNodes">The shutdown nodes that the other node knows</param>
        /// <param name="availableNodes">The available nodes that the other node knows</param>
        /// <param name="versionMatched">Skip the check for shutdown nodes</param>
        void InternalExtendHood(NodeIdRange const& range, std::set<NodeInstance> const& shutdownNodes, std::set<NodeInstance> const* availableNodes, bool versionMatched);

        /// <summary>
        /// Try to extend the predecessor hood by one
        /// </summary>
        /// <param name="range">The range that the other node knows</param>
        /// <param name="shutdownNodes">The shutdown nodes that the other node knows</param>
        /// <param name="availableNodes">The available nodes that the other node knows</param>
        /// <param name="versionMatched">Skip the check for shutdown nodes</param>
        /// <returns>Whether the predecessor hood is extended</returns>
        bool TryExtendPredHood(NodeIdRange const& range, std::set<NodeInstance> const& shutdownNodes, std::set<NodeInstance> const* availableNodes, bool versionMatched);

        /// <summary>
        /// Try to extend the successor hood by one
        /// </summary>
        /// <param name="range">The range that the other node knows</param>
        /// <param name="shutdownNodes">The shutdown nodes that the other node knows</param>
        /// <param name="availableNodes">The available nodes that the other node knows</param>
        /// <param name="versionMatched">Skip the check for shutdown nodes</param>
        /// <returns>Whether the successor hood is extended</returns>
        bool TryExtendSuccHood(NodeIdRange const& range, std::set<NodeInstance> const& shutdownNodes, std::set<NodeInstance> const* availableNodes, bool versionMatched);

        /// <summary>
        /// Invoke change notification event and neighborhood maintenance state machine.
        /// </summary>
        void OnNeighborhoodChanged(bool healthCheckNeeded);

        /// <summary>
        /// Invoke change notification event and neighborhood maintenance state machine.
        /// </summary>
        void OnRoutingTokenChanged(bool tokenVersionChanged);

        void InternalSetShutdown(NodeInstance const& shutdownNode, std::wstring const & ringName);

        void OnNeighborhoodLost();

        void Compact();

        void InternalGetExtendedHood(std::vector<PartnerNodeSPtr>& vecNode) const;

        void OnTimer();
        void CheckHealth();
        void CheckInitialLeasePartners();

        Common::StopwatchTime CheckSuccEdge(StateMachineActionCollection & actions);
        Common::StopwatchTime CheckPredEdge(StateMachineActionCollection & actions);
        void AddEdgeProbeAction(StateMachineActionCollection & actions, PartnerNodeSPtr const & target, bool isSuccDirection);
        Common::StopwatchTime CheckLivenessUpdateAction(StateMachineActionCollection & actions);

        Common::StopwatchTime CheckToken(StateMachineActionCollection & actions);
        void CheckEmptyNeighborhood();
        Common::StopwatchTime RequestFirstToken(StateMachineActionCollection & actions);
        Common::StopwatchTime CheckSuccToken(StateMachineActionCollection & actions);
        Common::StopwatchTime CheckPredToken(StateMachineActionCollection & actions);
        Transport::MessageUPtr CreateTokenTransferMessage(PartnerNodeSPtr const & from);
        void UpdateProbeHeader(FederationTraceProbeHeader & probeHeader);

        Common::StopwatchTime CheckInsertingNode(StateMachineActionCollection & actions);

        void TryReleaseToken(Transport::Message & message, PartnerNodeSPtr const & partner);

        void VerifyConsistency() const;
        PartnerNodeSPtr const & GetInternal(NodeId value, std::wstring const & ringName) const;
        PartnerNodeSPtr const & GetInternal(NodeId value, NodeRingBase const* ring) const;
        PartnerNodeSPtr const & GetInternal(NodeInstance const& value) const;
        Transport::MessageUPtr PrepareProbeMessage(FederationTraceProbeHeader & header, bool successorDirection, PartnerNodeSPtr & to);
        Transport::MessageUPtr CreateTokenTransferMessage(PartnerNodeSPtr const & to, std::vector<FederationRoutingTokenHeader> const & tokens);
        void VerifyProbePath(FederationTraceProbeHeader & probeHeader, PartnerNodeSPtr & to, bool ignoreStaleInfo);
        Transport::MessageUPtr PrepareEchoMessage(FederationTraceProbeHeader probeHeader);
        bool CanRecover(uint64 version, NodeId const& origin, bool succDirection);
        void InternalPartitionRanges(NodeIdRange const & range, size_t start, size_t end, size_t count, std::vector<PartnerNodeSPtr> & targets, std::vector<NodeIdRange> & subRanges) const;
        bool InternalIsDown(NodeInstance const & nodeInstance) const;

        void ReportNeighborhoodLost(std::wstring const & extraDescription);
        void ReportNeighborhoodRecovered();

        void IncreaseGlobalTimeUpperLimitForNodes(Common::TimeSpan delta);
        bool ProcessGlobalTimeExchangeHeaderInternal(Transport::Message & message, PartnerNodeSPtr const & from);
    };
}
