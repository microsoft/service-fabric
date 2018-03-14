// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class TestFederation
    {
        DENY_COPY(TestFederation)
    public:     

        TestFederation(std::wstring const & ringName) : 
            hoodSize_(Federation::FederationConfig::GetConfig().NeighborhoodSize),
            ringName_(ringName),
			lastEpoch_(0)
        {
        }

        // properties
        __declspec (property(get=getNeighborhoodSize, put=setNeighborhoodSize)) int NeighborhoodSize;
        int getNeighborhoodSize() const { return hoodSize_; }

        __declspec (property(get=getRingName)) std::wstring const & RingName;
        std::wstring const & getRingName() const { return ringName_; }

        // Setter functions for properties
        void setNeighborhoodSize(int value) { hoodSize_ = value; }

        bool ContainsNode(Federation::NodeId const& nodeId);
        Federation::NodeId GetNodeIdAt(int index);
        void AddNode(TestNodeSPtr const& testNode);
        TestNodeSPtr GetNode(Federation::NodeId const& nodeId);
        bool TryGetNode(Federation::NodeId const& nodeId, TestNodeSPtr & testNode);
        void RemoveNode(Federation::NodeId const& nodeId);

        bool AddArbitration(Federation::NodeId from, Federation::NodeId to);
        bool ReportDownNode(Federation::NodeId node);
        bool VerifyArbitration();

        bool VerifyAllRoutingTables(Common::TimeSpan timeout);
        bool VerifyAllTokens(Common::TimeSpan timeout);
		bool VerifyVoterStore(Common::TimeSpan timeout);
		bool VerifyGlobalTime(Common::TimeSpan timeout);

        Federation::NodeId CalculateClosestNodeTo(Federation::NodeId const & nodeId, bool useTokenRangeForExpectedRouting);

        void OnStoreWriteCompleted(std::wstring const & key, int64 value, int64 sequence);
        int64 GetStoreEntry(std::wstring const & key, int64 & sequence);

        void ForEachTestNode(std::function<void(TestNodeSPtr const&)> processor) const
        {
            Common::AcquireExclusiveLock lock(nodeListLock_);
            for_each(nodes_.begin(), nodes_.end(), processor);
        }

        void GetNodeList(std::set<Federation::NodeId> &);

        TestNodeSPtr GetNodeWithLeaseAgentPort(int port);

        std::size_t Count();

    private:
        struct VoterStoreEntry
        {
            int64 Value;
            int64 Sequence;
        };

        class ArbitrationValidator
        {
        public:
            ArbitrationValidator();
            void AddBlockedEdge(Federation::NodeId from, Federation::NodeId to);
            bool ReportDownNode(Federation::NodeId node);
            bool Verify();
            void Reset();
            std::set<Federation::NodeId> GetDownNodes();

        private:
            typedef std::pair<Federation::NodeId, Federation::NodeId> BlockedEdge;

            std::set<Federation::NodeId> blockedNodes_;
            std::vector<BlockedEdge> blockedEdges_;
            std::set<Federation::NodeId> downNodes_;
        };

        bool VerifyNode(Federation::NodeIdRange & range, std::vector<Federation::PartnerNodeSPtr> & neighbors);
        bool VerifyRange(Federation::NodeIdRange & range, Federation::NodeId nodeId);
        bool GetNeighborsInRange(Federation::NodeIdRange & range, std::set<Federation::NodeId> & neighbors);
        void GetNodeListInternal(std::set<Federation::NodeId> &);
        void RemoveNodeInternal(Federation::NodeId const& nodeId);
        bool TryGetNodeInternal(Federation::NodeId const& nodeId, TestNodeSPtr & testNode);

        std::vector<TestNodeSPtr> nodes_;
        mutable Common::ExclusiveLock nodeListLock_;
        int hoodSize_;
        std::wstring ringName_;
		
        mutable Common::ExclusiveLock storeLock_;
        int64 lastEpoch_;
        std::map<std::wstring, VoterStoreEntry> storeEntries_;
        ArbitrationValidator arbitrationValidator_;
    };
};
