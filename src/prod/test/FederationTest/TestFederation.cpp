// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace FederationTest
{
    using namespace Common;
    using namespace Federation;
    using namespace std;
    using namespace TestCommon;

    const StringLiteral TraceSource = "FederationTest.Federation";

    struct CompareNodeId
    {
    public:
        bool operator()(TestNodeSPtr const & node, NodeId id)
        {
            return (node->SiteNodePtr->Id.IdValue < id.IdValue);
        }
    };

    TestNodeSPtr TestFederation::GetNode(NodeId const& nodeId)
    {
        TestNodeSPtr testNodeSPtr;
        TestSession::FailTestIfNot(TryGetNode(nodeId, testNodeSPtr), "Node {0} not found", nodeId);
        return testNodeSPtr;
    }

    void TestFederation::AddNode(TestNodeSPtr const& testNode)
    {
        AcquireExclusiveLock lock(nodeListLock_);

        auto it = lower_bound(nodes_.begin(), nodes_.end(), testNode->SiteNodePtr->Id, CompareNodeId());
        TestSession::FailTestIf(it != nodes_.end() && (*it)->SiteNodePtr->Id == testNode->SiteNodePtr->Id, "{0} already exists", testNode->SiteNodePtr->Id);
        nodes_.insert(it, testNode);
    }

    void TestFederation::RemoveNode(NodeId const & nodeId)
    {
        AcquireExclusiveLock lock(nodeListLock_);
        RemoveNodeInternal(nodeId);
    }

    void TestFederation::RemoveNodeInternal(NodeId const & nodeId)
    {
        auto it = lower_bound(nodes_.begin(), nodes_.end(), nodeId, CompareNodeId());
        if (it != nodes_.end() && (*it)->SiteNodePtr->Id == nodeId)
        {
            nodes_.erase(it);
        }

        TestSession::WriteInfo(TraceSource, "Removing {0} from test federation {1}", nodeId, ringName_);
        if (nodes_.size() == 0)
        {
            lastEpoch_ = 0;
        }
    }

    bool TestFederation::ContainsNode(NodeId const& nodeId)
    {
        AcquireReadLock lock(nodeListLock_);
        TestNodeSPtr testNode;
        return TryGetNodeInternal(nodeId, testNode);
    }

    bool TestFederation::TryGetNodeInternal(NodeId const & nodeId, TestNodeSPtr & testNode)
    {
        auto it = lower_bound(nodes_.begin(), nodes_.end(), nodeId, CompareNodeId());
        if (it != nodes_.end() && (*it)->SiteNodePtr->Id == nodeId)
        {
            testNode = *it;
            return true;
        }

        testNode = nullptr;
        return false;
    }

    void TestFederation::GetNodeList(set<NodeId> & nodes)
    {
        AcquireReadLock lock(nodeListLock_);
        GetNodeListInternal(nodes);
    }

    void TestFederation::GetNodeListInternal(set<NodeId> & nodes)
    {
        for (auto it = nodes_.begin(); it != nodes_.end(); it++)
        {
            nodes.insert((*it)->SiteNodePtr->Id);
        }
    }

    bool TestFederation::TryGetNode(NodeId const & nodeId, TestNodeSPtr & testNode)
    {
        AcquireReadLock lock(nodeListLock_);
        return TryGetNodeInternal(nodeId, testNode);
    }

    TestNodeSPtr TestFederation::GetNodeWithLeaseAgentPort(int port)
    {
        AcquireReadLock lock(nodeListLock_);
        for (TestNodeSPtr const & node : nodes_)
        {
            if (node->GetLeaseAgentPort() == port)
            {
                return node;
            }
        }

        return nullptr;
    }

    bool TestFederation::VerifyAllRoutingTables(TimeSpan timeout)
    {
        TestSession::WriteNoise(TraceSource, "Starting to verify routing tables.");
        bool done = false;
        bool passed = true;

        Stopwatch watch;
        watch.Start();
        while(!done && (timeout.TotalMilliseconds() > watch.ElapsedMilliseconds))
        {
            {
                AcquireExclusiveLock lock(nodeListLock_);
                for (auto iterator = nodes_.begin(); iterator != nodes_.end(); iterator++)
                {
                    TestNodeSPtr testNode = (*iterator);
                    vector<PartnerNodeSPtr> neighbors;
                    NodeIdRange range = testNode->SiteNodePtr->Table.GetHood(neighbors);
                    if (!VerifyRange(range, testNode->SiteNodePtr->Id) || !VerifyNode(range, neighbors))
                    {
                        TestSession::WriteInfo(TraceSource, "Routing table verification failed for node {0}. Pausing for 2 seconds before retrying", testNode->SiteNodePtr->Id);
                        passed = false;
                        break;
                    }
                }
            }

            if(passed)
            {
                done = true;
            }
            else
            {
                Sleep(2000);
                passed = true;
            }
        }

        if (done)
        {
            AcquireExclusiveLock lock(nodeListLock_);
            size_t upNodeTotal = 0;
            size_t upNodeFalsePositive = 0;
            for(auto iterator = nodes_.begin(); iterator != nodes_.end(); iterator++)
            {
                TestNodeSPtr testNode = (*iterator);

                // Calculate stale up-nodes outside neighborhood
                std::vector<NodeInstance> upNodes;
                testNode->SiteNodePtr->Table.Test_GetUpNodesOutsideNeighborhood(upNodes);
                upNodeTotal += upNodes.size();
                for (auto iter = upNodes.cbegin(); iter != upNodes.cend(); ++ iter)
                {
                    TestNodeSPtr tempNode;
                    if (TryGetNodeInternal(iter->Id, tempNode))
                    {
                        ++ upNodeFalsePositive;
                    }
                }
            }

            if (upNodeTotal > 0)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Up-node false-positive outside neighborhood: {0} out of {1}, {2}%",
                    upNodeFalsePositive,
                    upNodeTotal,
                    static_cast<float>(upNodeFalsePositive) * 100.0/upNodeTotal);
            }
        }

        return done;
    }

    bool TestFederation::VerifyAllTokens(TimeSpan timeout)
    {
        TestSession::WriteNoise(TraceSource, "Starting to verify routing tokens.");
        bool done = false;
        Stopwatch watch;
        watch.Start();
        AcquireExclusiveLock lock(nodeListLock_);

        while(!done && (timeout.TotalMilliseconds() > watch.ElapsedMilliseconds))
        {
            string output;
            StringWriterA writer(output);
            int outputCount = 0;

            vector<LargeInteger> nodes;
            for (auto it = nodes_.begin(); it != nodes_.end(); it++)
            {
                nodes.push_back((*it)->SiteNodePtr->Id.IdValue);
            }

            for (size_t i = 0; i < nodes.size(); i++)
            {
                size_t prevIndex = (i == 0 ? nodes.size() - 1 : i - 1);
                size_t nextIndex = (i == nodes.size() - 1 ? 0 : i + 1);
                
                LargeInteger id = nodes[i];
                LargeInteger prevId = nodes[prevIndex];
                LargeInteger nextId = nodes[nextIndex];

                TestNodeSPtr testNode; 
                TestSession::FailTestIfNot(TryGetNodeInternal(NodeId(id), testNode), "Node {0} should exist", id);
                RoutingToken token = testNode->SiteNodePtr->Token;

                if (token.IsEmpty)
                {
                    if (outputCount < 3)
                    {
                        writer.Write("(Node {0} token empty)", id);
                        outputCount++;
                    }
                }
                else if (nodes.size() == 1)
                {
                    if (!token.IsFull)
                    {
                        writer.Write("(Node {0} token {1} is not full)", id, token);
                        outputCount++;
                    }
                }
                else
                {
                    LargeInteger succDist = nextId - id;
                    LargeInteger id1 = id + (succDist >> 1);
                    LargeInteger predDist = id - prevId;
                    predDist--;
                    LargeInteger id2 = id - (predDist >> 1);

                    if (token.Range.Begin != id2 || token.Range.End != id1 && outputCount < 3)
                    {
                        writer.Write("(Node {0} token {1} should be {2}-{3})", id, token, id2, id1);
                        outputCount++;
                    }
                }
            }

            if (outputCount == 0)
            {
                done = true;
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "{0}", output);
                Sleep(2000);
            }
        }

        return done;
    }

    bool TestFederation::VerifyNode(NodeIdRange & range, vector<PartnerNodeSPtr> & neighbors)
    {
        set<NodeId> federationTestNeighbors;

        if(!GetNeighborsInRange(range, federationTestNeighbors))
        {
            return false;
        }

        int deadNodes = 0;
        for (PartnerNodeSPtr const & partnerNodeSPtr : neighbors)
        {
            if(partnerNodeSPtr->IsAvailable)
            {
                set<NodeId>::const_iterator iterator = federationTestNeighbors.find(partnerNodeSPtr->Id);
                if(iterator == federationTestNeighbors.end())
                {
                    TestSession::WriteInfo(TraceSource, "Could not find SiteNode neighbor {0} in FederationTest neighbor list", partnerNodeSPtr->Id);
                    return false;
                }
            }
            else
            {
                deadNodes++;
            }
        }

        size_t federationTestOnlyNeighbors = federationTestNeighbors.size() - 1;
        if(federationTestOnlyNeighbors + deadNodes != neighbors.size())
        {
            TestSession::WriteInfo(TraceSource, "The FederationTest neighborhood size {0} + Deadnodes {1} did not match SiteNode neighborhood size {2}", federationTestOnlyNeighbors, deadNodes, neighbors.size());
            return false;
        }

        return true;
    }

    bool TestFederation::GetNeighborsInRange(NodeIdRange & range, set<NodeId> & neighbors)
    {
        set<NodeId> nodesList;
        GetNodeListInternal(nodesList);
        if(range.IsFull)
        {
            neighbors.insert(nodesList.begin(), nodesList.end());
            return true;
        }
        else
        {
            set<NodeId>::const_iterator begin = nodesList.find(range.Begin);
            set<NodeId>::const_iterator end = nodesList.find(range.End);

            if(begin == nodesList.end() || end == nodesList.end())
            {
                TestSession::WriteInfo(TraceSource, "Could not find nodes edge neighbors in global knowledge. Begin {0}, End {1}", range.Begin, range.End);
                return false;
            }

            if(range.Begin > range.End)
            {
                neighbors.insert(nodesList.begin(), ++end);
                neighbors.insert(begin, nodesList.end());
            }
            else
            {
                neighbors.insert(begin, ++end);
            }

            return true;
        }
    }

    bool TestFederation::VerifyRange(NodeIdRange & range, NodeId nodeId)
    {
        set<NodeId> nodesList;
        GetNodeListInternal(nodesList);

        vector<NodeId> nodeIdVector(nodesList.begin(), nodesList.end());
        int64 index = std::find(nodeIdVector.begin(), nodeIdVector.end(), nodeId) - nodeIdVector.begin();
        size_t size = nodeIdVector.size();
        NodeId end = nodeIdVector[(index + hoodSize_) % size];
        NodeId begin = nodeIdVector[(index - hoodSize_ + size) % size];

        if(range.IsFull)
        {
            // If predesessor and successor has no gap, the range can still be full.
            if(nodesList.size() > static_cast<size_t>(hoodSize_ * 2) && end.SuccDist(begin) != LargeInteger::One)
            {
                TestSession::WriteInfo(TraceSource, "The range verification failed for node {0}.", nodeId);
                TestSession::WriteInfo(TraceSource, "Number of nodes in ring is {0} and NeighborhoodSize is {1}", nodesList.size(), hoodSize_);
                return false;
            }
        }
        else
        {
            // If not enough up nodes in the ring, down nodes can be used as range edge.
            if((range.Begin != begin || range.End != end) && (size > static_cast<size_t>(hoodSize_ * 2)))
            {
                TestSession::WriteInfo(TraceSource, "The range verification failed for node {0}", nodeId);
                TestSession::WriteInfo(TraceSource, "Actual predecessor edge {0}, expected predecessor edge {1}", range.Begin, begin);
                TestSession::WriteInfo(TraceSource, "Actual successor edge {0}, expected successor edge {1}", range.End, end);
                return false;
            }
        }

        return true;
    }

    NodeId TestFederation::CalculateClosestNodeTo(NodeId const & nodeId, bool useTokenRangeForExpectedRouting)
    {
        AcquireExclusiveLock lock(nodeListLock_);

        // Use the current token ranges to find the closest node (assumes ranges are correct), otherwise fallback to using
        // just straight NodeId distances which may return nodes with empty token ranges etc.
        if (useTokenRangeForExpectedRouting)
        {
            for(auto iterator = nodes_.begin(); iterator != nodes_.end(); iterator++)
            {
                TestNodeSPtr testNode = (*iterator);
                RoutingToken token = testNode->SiteNodePtr->Token;
                if (token.Range.Contains(nodeId))
                {
                    return testNode->SiteNodePtr->Id;
                }
            }
        }

        set<NodeId> nodesList;
        GetNodeListInternal(nodesList);

        set<NodeId>::iterator iteratorToLargerNodeId = nodesList.lower_bound(nodeId);

        set<NodeId>::iterator successor;
        set<NodeId>::iterator predecessor;

        if(iteratorToLargerNodeId == nodesList.end() || iteratorToLargerNodeId == nodesList.begin())
        {
            successor = nodesList.begin();
            predecessor = --nodesList.end();
        }
        else
        {
            successor = iteratorToLargerNodeId;
            predecessor = --iteratorToLargerNodeId;
        }

        if (nodeId.MinDist(*predecessor) <= nodeId.MinDist(*successor))
        {
            return *predecessor;
        }
        else
        {
            return *successor;
        }
    }

    size_t TestFederation::Count()
    {
        AcquireExclusiveLock lock(nodeListLock_);
        return nodes_.size();
    }

    NodeId TestFederation::GetNodeIdAt(int index)
    {
        AcquireExclusiveLock lock(nodeListLock_);
        TestSession::FailTestIf(index < 0 || static_cast<size_t>(index) > nodes_.size() - 1, "Invalid index provided");
        int i = 0;
        for(auto iterator = nodes_.begin(); iterator != nodes_.end(); iterator++)
        {
            if(i == index)
            {
                return (*iterator)->SiteNodePtr->Id;
            }

            i++;
        }

        return NodeId();
    }

	bool TestFederation::VerifyVoterStore(TimeSpan timeout)
	{
		bool done = false;
		Stopwatch watch;
		watch.Start();

		set<NodeInstance> seedNodes;
		AcquireExclusiveLock lock(nodeListLock_);
		for (auto it = nodes_.begin(); it != nodes_.end(); it++)
		{
			if (VoteManager::IsSeedNode((*it)->SiteNodePtr->Id, (*it)->SiteNodePtr->RingName))
			{
				seedNodes.insert((*it)->SiteNodePtr->Instance);
			}
		}

        wstring entries;
        NodeId nodeId;
        while (!done && (timeout.TotalMilliseconds() > watch.ElapsedMilliseconds))
		{
			string output;
			StringWriterA writer(output);

            int64 generation = 0;
            int64 epoch = 0;
			for (auto it = nodes_.begin(); it != nodes_.end(); it++)
			{
				SiteNodeSPtr const & node = (*it)->SiteNodePtr;
				if (VoteManager::IsSeedNode(node->Id, node->RingName))
				{
                    VoterStore::ReplicaSetConfiguration config;
                    node->GetVoterStore().GetConfiguration(config);
                    if (generation == 0)
                    {
                        generation = config.Generation;
                        epoch = config.Epoch;
                        nodeId = node->Id;
                        entries = wformatString("{0:e}", node->GetVoterStore());
                    }
                    else if (config.Generation != generation || config.Epoch != epoch)
                    {
                        writer.Write("(Config on {0} and {1} different: {2}:{3}/{4}:{5})",
                            nodeId, node->Id, generation, epoch, config.Generation, config.Epoch);
                    }

                    auto temp = seedNodes;
                    for (NodeInstance const & replica : config.Replicas)
					{
						auto it1 = temp.find(replica);
						if (it1 != temp.end())
						{
							temp.erase(it1);
						}
					}

					if (temp.size() > 0)
					{
                        writer.Write("(Voter store config {0} on {1} does not contain voters {2})", config.Replicas, node->Id, temp);
					}
                    else if (config.Replicas.size() > 0)
                    {
                        if (seedNodes.find(config.Replicas[0]) == seedNodes.end())
                        {
                            writer.Write("(Voter store config {0} on {1} contains down primary)", config.Replicas, node->Id, config.Replicas[0]);
                        }
                        else
                        {
                            wstring expectedRole = (config.Replicas[0].Id == node->Id ? L"P" : L"S");
                            wstring actualRole = wformatString(node->GetVoterStore()).substr(0, 1);
                            if (actualRole != expectedRole)
                            {
                                writer.Write("(Voter store on {0} expected role {1} actual {2})",
                                    node->Id, expectedRole, actualRole);
                            }
                        }
                    }
				}
			}

            if (output.size() == 0)
            {
                for (auto it = nodes_.begin(); it != nodes_.end(); it++)
                {
                    SiteNodeSPtr const & node = (*it)->SiteNodePtr;
                    if (VoteManager::IsSeedNode(node->Id, node->RingName))
                    {
                        wstring data = wformatString("{0:e}", node->GetVoterStore());
                        if (data != entries)
                        {
                            writer.Write("(Voter store on {0} data {1} different from {2} data {3})",
                                nodeId, entries, node->Id, data);
                        }
                    }
                }
            }

			if (output.size() == 0)
			{
				done = true;
			}
			else
			{
				TestSession::WriteInfo(TraceSource, "{0}", output);
				Sleep(2000);
			}
		}

		return done;
	}

	bool TestFederation::VerifyGlobalTime(TimeSpan timeout)
	{
		bool done = false;
		Stopwatch watch;
		watch.Start();

		while (!done && (timeout.TotalMilliseconds() > watch.ElapsedMilliseconds))
		{
			string output;
			StringWriterA writer(output);

			for (auto it = nodes_.begin(); it != nodes_.end(); it++)
			{
				SiteNodeSPtr const & node = (*it)->SiteNodePtr;

                int64 epoch;
                TimeSpan interval;
                bool isAuthority;
                (*it)->GetGlobalTimeState(epoch, interval, isAuthority);

                if (interval > FederationConfig::GetConfig().GlobalTimeUncertaintyIntervalUpperBound)
                {
                    writer.Write("(Node {0} interval {1} exceeds upper bound)", node->Id, interval);
                }

                if (epoch < lastEpoch_)
                {
                    writer.Write("(Node {0} epoch {1} less than the last seen {2})", node->Id, epoch, lastEpoch_);
                }
                else if (epoch > lastEpoch_)
                {
                    lastEpoch_ = epoch;
                }
            }

			for (auto it = nodes_.begin(); it != nodes_.end(); it++)
			{
				SiteNodeSPtr const & node = (*it)->SiteNodePtr;
				
				int64 epoch;
				TimeSpan interval;
				bool isAuthority;
				(*it)->GetGlobalTimeState(epoch, interval, isAuthority);
		
				if (epoch != lastEpoch_)
				{
					writer.Write("(Node {0} epoch {1} is different from the leader {2})", node->Id, epoch, lastEpoch_);
				}

				if (VoteManager::IsSeedNode(node->Id, node->RingName))
				{
					VoterStore & store = node->GetVoterStore();
                    auto entry = store.GetEntry(Federation::Constants::GlobalTimestampEpochName);
                    if (entry)
                    {
                        int64 epoch1 = Int64_Parse(wformatString(*entry));
                        if (epoch1 < lastEpoch_)
                        {
                            writer.Write("(Voter store on {0} sequence is {1}, expected {2})", node->Id, epoch1, lastEpoch_);
                        }
                    }
                }
			}

			if (output.size() == 0)
			{
				done = true;
			}
			else
			{
				TestSession::WriteInfo(TraceSource, "{0}", output);
				Sleep(2000);
			}
		}

		return done;
	}

    void TestFederation::OnStoreWriteCompleted(wstring const & key, int64 value, int64 sequence)
    {
        VoterStoreEntry entry;
        entry.Value = value;
        entry.Sequence = sequence;

        AcquireWriteLock grab(storeLock_);
        auto it = storeEntries_.find(key);
        if (it == storeEntries_.end())
        {
            storeEntries_.insert(make_pair(key, entry));
        }
        else if (entry.Sequence > it->second.Sequence)
        {
            it->second = entry;
        }
    }
    
    int64 TestFederation::GetStoreEntry(wstring const & key, int64 & sequence)
    {
        AcquireReadLock grab(storeLock_);
        auto it = storeEntries_.find(key);
        if (it == storeEntries_.end())
        {
            sequence = 0;
            return 0;
        }
        else
        {
            sequence = it->second.Sequence;
            return it->second.Value;
        }
    }

    bool TestFederation::AddArbitration(NodeId from, NodeId to)
    {
        arbitrationValidator_.AddBlockedEdge(from, to);

        return true;
    }

    bool TestFederation::ReportDownNode(NodeId node)
    {
        return arbitrationValidator_.ReportDownNode(node);
    }

    bool TestFederation::VerifyArbitration()
    {
        if (!arbitrationValidator_.Verify())
        {
            return false;
        }

        set<NodeId> downNodes = arbitrationValidator_.GetDownNodes();
        arbitrationValidator_.Reset();

        vector<NodeId> allNodes;
        for (NodeId id : downNodes)
        {
            allNodes.push_back(id);
        }

        set<NodeId> upNodes;
        GetNodeList(upNodes);
        for (NodeId id : upNodes)
        {
            allNodes.push_back(id);
        }

        size_t length = 0;
        size_t firstUp = allNodes.size();
        size_t lastDown = allNodes.size();
        for (size_t i = 0; i < allNodes.size(); i++)
        {
            if (downNodes.find(allNodes[i]) == downNodes.end())
            {
                if (i < firstUp)
                {
                    firstUp = i;
                }
                if (lastDown < i)
                {
                    length = max(length, i - lastDown);
                }
                lastDown = allNodes.size();
            }
            else if (i < lastDown)
            {
                lastDown = i;
            }
        }

        if (lastDown < allNodes.size())
        {
            length = max(length, firstUp + allNodes.size() - lastDown);
        }

        TestSession::WriteInfo(TraceSource, "Arbitration validation passed, down nodes {0}, length={1}",
            downNodes, length);

        bool result = VerifyAllRoutingTables(length >= 9 ? FederationConfig::GetConfig().GlobalTicketLeaseDuration : FederationTestConfig::GetConfig().VerifyTimeout);
        if (result && length >= 9)
        {
            TestSession::WriteInfo(TraceSource, "Waiting for {0}", FederationConfig::GetConfig().GlobalTicketLeaseDuration);
            Sleep(static_cast<DWORD>(FederationConfig::GetConfig().GlobalTicketLeaseDuration.TotalMilliseconds()));
        }

        return result;
    }

    TestFederation::ArbitrationValidator::ArbitrationValidator()
    {
    }

    void TestFederation::ArbitrationValidator::AddBlockedEdge(NodeId from, NodeId to)
    {
        blockedNodes_.insert(from);
        blockedNodes_.insert(to);

        blockedEdges_.push_back(make_pair(from, to));
    }

    bool TestFederation::ArbitrationValidator::ReportDownNode(NodeId node)
    {
        if (blockedNodes_.find(node) == blockedNodes_.end())
        {
            return false;
        }

        downNodes_.insert(node);

        return true;
    }

    bool TestFederation::ArbitrationValidator::Verify()
    {
        if (blockedNodes_.size() == 0)
        {
            return true;
        }

        set<NodeId> verified;
        for (NodeId id : blockedNodes_)
        {
            if (downNodes_.find(id) == downNodes_.end())
            {
                verified.insert(id);
            }
        }

        bool result;
        do
        {
            result = false;
            for (auto it = blockedEdges_.begin(); it != blockedEdges_.end();)
            {
                bool verified1 = (verified.find(it->first) != verified.end());
                bool verified2 = (verified.find(it->second) != verified.end());
                if (verified1 || verified2)
                {
                    if (!verified1)
                    {
                        verified.insert(it->first);
                    }
                    if (!verified2)
                    {
                        verified.insert(it->second);
                    }

                    it = blockedEdges_.erase(it);
                    result = true;
                }
                else
                {
                    ++it;
                }
            }
        } while (result);

        for (auto it = blockedEdges_.begin(); it != blockedEdges_.end(); ++it)
        {
            TestSession::WriteError(TraceSource, "Arbitration for {0}-{1} is invalid",
                it->first, it->second);
        }

        result = true;
        for (NodeId id : downNodes_)
        {
            if (verified.find(id) == verified.end())
            {
                result = false;
            }
        }

        if (!result)
        {
            TestSession::WriteError(TraceSource, "Down nodes: {0}, verified: {1}",
                downNodes_, verified);
        }

        return result;
    }

    void TestFederation::ArbitrationValidator::Reset()
    {
        blockedNodes_.clear();
        blockedEdges_.clear();
        downNodes_.clear();
    }

    set<NodeId> TestFederation::ArbitrationValidator::GetDownNodes()
    {
        return downNodes_;
    }
}
