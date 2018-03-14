// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "SiteNodeHelper.h"

namespace FederationUnitTests
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;

    class RoutingTableTests
    {
    };

    const wstring prefix = L"addr_";
    int basePort_ = 10500;

    wstring AddressOfNode(size_t node)
    {
        CODING_ERROR_ASSERT(basePort_ + node < 65536);
        return wformatString("127.0.0.1:{0}", basePort_ + node);
    }

    SiteNodeSPtr CreateSiteNode(size_t low, bool initializeAsSeedNode = true)
    {
        wstring host = SiteNodeHelper::GetLoopbackAddress();
        wstring port = SiteNodeHelper::GetFederationPort();
        NodeId nodeId(LargeInteger(0, low));

        // override seednode config
        VoteConfig seedNodes;
        seedNodes.push_back(VoteEntryConfig(nodeId, Federation::Constants::SeedNodeVoteType, host + L":" + port));

        if (initializeAsSeedNode)
        {
            FederationConfig::GetConfig().Votes = seedNodes;
        }

        SiteNodeSPtr sitePtr = SiteNodeHelper::CreateSiteNode(nodeId, host, port);

        return sitePtr;
    }

    FederationPartnerNodeHeader CreateNodeHeader(size_t low, NodePhase::Enum phase, int nodeInstance, int tokenVersion)
    {
        NodeId id(LargeInteger(0, low));
        return FederationPartnerNodeHeader(NodeInstance(id, nodeInstance), phase, AddressOfNode(low), L"", 0, RoutingToken(NodeIdRange(), tokenVersion), Common::Uri(), false, L"", 1);
    }

    FederationPartnerNodeHeader CreateNodeHeader(size_t low, NodePhase::Enum phase)
    {
        return CreateNodeHeader(low, phase, 1, 0);
    }

    PartnerNodeSPtr CreatePtr(size_t low, NodePhase::Enum phase, SiteNodeSPtr const & siteNode)
    {
        return make_shared<PartnerNode>(CreateNodeHeader(low, phase), *(siteNode.get()));
    }

    void OpenSiteNode(SiteNodeSPtr & sitePtr)
    {
		sitePtr->Initialize(sitePtr);
		sitePtr->Table.Test_SetTestMode();
		sitePtr->Table.Bootstrap();
    }

    void CloseSiteNode(SiteNodeSPtr & sitePtr)
    {
		sitePtr->DeInitialize();
    }

    void FillTable(RoutingTable & table, size_t tableNodes[], int size)
    {
        for (int i = 0; i < size; i++)
        {
            size_t tableNode = tableNodes[i];

            table.Consider(CreateNodeHeader(tableNode, NodePhase::Inserting), true);
            table.Consider(CreateNodeHeader(tableNode, NodePhase::Routing));
        }
    }

    bool ContainsExactlyOne(vector<PartnerNodeSPtr> const& partnerNodes, NodeId nodeId)
    {
        size_t count = count_if(
            partnerNodes.begin(),
            partnerNodes.end(),
            [nodeId](PartnerNodeSPtr p) -> bool { return p->Id == nodeId; });

        return count == 1;
    }


    vector<RoutingToken> FindRoutingTokens(MessageHeaders & headers)
    {
        vector<RoutingToken> tokens;

		for (auto iter = headers.begin(); iter != headers.end(); ++iter)
		{
			if (iter->Id() == MessageHeaderId::FederationRoutingToken)
			{
				FederationRoutingTokenHeader header = iter->Deserialize<FederationRoutingTokenHeader>();
				tokens.push_back(header.Token);
			}
		}

        return tokens;
    }

    BOOST_FIXTURE_TEST_SUITE(RoutingTableTestsSuite,RoutingTableTests)

    BOOST_AUTO_TEST_CASE(NodeRingTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(0);
        OpenSiteNode(sitePtr);

        NodeRing ring(CreatePtr(0, NodePhase::Routing, sitePtr));

        for (int i = 200; i > 0; i -= 10)
        {
            ring.AddNode(CreatePtr(i, NodePhase::Routing, sitePtr));
        }

        VERIFY_IS_TRUE(ring.Size == 21);

        for (size_t i = 0; i < 21; i++)
        {
            LargeInteger value(0, i * 10);
            NodeId id = ring.GetNode(i)->Instance.Id;
            VERIFY_IS_TRUE(id == value);
            VERIFY_IS_TRUE(ring.FindSuccOrSamePosition(value) == i);
            VERIFY_IS_TRUE(ring.GetPred(i) == (i == 0? 20 : i - 1));
            VERIFY_IS_TRUE(ring.GetSucc(i) == (i == 20? 0 : i + 1));
        }

        for (int i = 20; i > 0; i--)
        {
            LargeInteger value(0, i * 10);
            ring.RemoveNode(i);
            VERIFY_IS_TRUE(ring.FindSuccOrSamePosition(value) == 0);
        }

        CloseSiteNode(sitePtr);

        //SiteNodeHelper::DeleteTicketFile(sitePtr->Id);
        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(NodeRingWithHoodSampleTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(180);
        OpenSiteNode(sitePtr);

        NodeRingWithHood ring(sitePtr, 2);

        ring.AddNode(CreatePtr(10, NodePhase::Routing, sitePtr));
        ring.AddNode(CreatePtr(90, NodePhase::Routing, sitePtr));
        ring.AddNode(CreatePtr(150, NodePhase::Routing, sitePtr));
        ring.AddNode(CreatePtr(210, NodePhase::Routing, sitePtr));

        ring.ExtendPredHoodEdge();
        ring.ExtendPredHoodEdge();
        ring.ExtendSuccHoodEdge();
        ring.ExtendSuccHoodEdge();

        VERIFY_IS_TRUE(ring.PredHoodEdge == 1 && ring.SuccHoodEdge == 0);
        VERIFY_IS_TRUE(ring.PredHoodCount == 2 && ring.SuccHoodCount == 2);

        ring.AddNode(CreatePtr(9, NodePhase::Routing, sitePtr));
        VERIFY_IS_TRUE(ring.PredHoodCount == 2 && ring.SuccHoodCount == 3);
        ring.AddNode(CreatePtr(92, NodePhase::Routing, sitePtr));
        VERIFY_IS_TRUE(ring.PredHoodCount == 3 && ring.SuccHoodCount == 3);
        ring.AddNode(CreatePtr(50, NodePhase::Routing, sitePtr));
        VERIFY_IS_TRUE(ring.PredHoodCount == 3 && ring.SuccHoodCount == 3);

        ring.ExtendPredHoodEdge();
        ring.ExtendSuccHoodEdge();
        VERIFY_IS_TRUE(ring.CompleteHoodRange);
        VERIFY_IS_TRUE(ring.PredHoodCount == 7 && ring.SuccHoodCount == 7);

        ring.AddNode(CreatePtr(49, NodePhase::Routing, sitePtr));
        VERIFY_IS_TRUE(ring.PredHoodCount == 8 && ring.SuccHoodCount == 8);

        ring.BreakCompleteRing(2);
        VERIFY_IS_TRUE(ring.PredHoodCount == 2 && ring.SuccHoodCount == 2);

        // Release the sitePtr
        PartnerNodeSPtr const& ptr = ring.ThisNodePtr;
        PartnerNodeSPtr newPtr = make_shared<PartnerNode>(*ptr);
        ring.ReplaceNode(ring.ThisNode, newPtr);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableGetTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

		RoutingTable table(sitePtr, FederationConfig::GetConfig().NeighborhoodSize);

        size_t tableNodes[] = {90, 110, 120};
        FillTable(table, tableNodes, 3);

        PartnerNodeSPtr partnerNode;

        // Verify Get
        NodeId correctNodeId(LargeInteger(0, 90));
        NodeInstance correctInstance(correctNodeId, 1);

        partnerNode = table.Get(correctInstance);
        VERIFY_IS_TRUE(partnerNode);
        VERIFY_IS_TRUE(partnerNode->Id == correctNodeId);

        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode);
        VERIFY_IS_TRUE(partnerNode->Id == correctNodeId);

        // Get of incorrect Instance value should return null
        NodeInstance wrongInstance(correctNodeId, 2);
        partnerNode = table.Get(wrongInstance);
        VERIFY_IS_TRUE(!partnerNode);

        // Get of unknown entities should return null
        NodeId unknownNodeId(LargeInteger(0, 0));
        NodeInstance unknownInstance(unknownNodeId, 0);

        partnerNode = table.Get(unknownInstance);
        VERIFY_IS_TRUE(!partnerNode);

        partnerNode = table.Get(unknownNodeId);
        VERIFY_IS_TRUE(!partnerNode);

		table.CloseSiteNode();
        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableConsiderTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        PartnerNodeSPtr partnerNode;
        NodeId node90(LargeInteger(0, 90));
        NodeId node100(LargeInteger(0, 100));
        NodeId node110(LargeInteger(0, 110));

        // Consider on the SiteNode itself should be no-op
        table.Consider(CreateNodeHeader(100, NodePhase::Inserting), true);
        partnerNode = table.Get(node100);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(100, NodePhase::Inserting), false);
        partnerNode = table.Get(node100);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(100, NodePhase::Shutdown), true);
        partnerNode = table.Get(node100);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(100, NodePhase::Shutdown), false);
        partnerNode = table.Get(node100);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        size_t tableNodes[] = {90, 110, 120, 150};
        FillTable(table, tableNodes, 4);

        // Considers with previous/same/future Phase information
        table.Consider(CreateNodeHeader(90, NodePhase::Inserting), true);
        partnerNode = table.Get(node90);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(90, NodePhase::Inserting), false);
        partnerNode = table.Get(node90);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(90, NodePhase::Routing), true);
        partnerNode = table.Get(node90);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(90, NodePhase::Routing), false);
        partnerNode = table.Get(node90);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.Consider(CreateNodeHeader(90, NodePhase::Shutdown), true);
        partnerNode = table.Get(node90);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        table.Consider(CreateNodeHeader(110, NodePhase::Shutdown), false);
        partnerNode = table.Get(node110);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        // Considers with future token versions + later NodePhases
        NodeId node95(LargeInteger(0, 95));
        table.Consider(CreateNodeHeader(95, NodePhase::Inserting), true);

        FederationPartnerNodeHeader header = CreateNodeHeader(95, NodePhase::Inserting, 1, 100);
        table.Consider(header, false);
        partnerNode = table.Get(node95);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Inserting);
        VERIFY_IS_TRUE(partnerNode->Token.Version == 100);

        header = CreateNodeHeader(95, NodePhase::Routing, 1, 200);
        table.Consider(header, false);
        partnerNode = table.Get(node95);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);
        VERIFY_IS_TRUE(partnerNode->Token.Version == 200);

        // Considers for Routing nodes etc. that are not yet in the Routing table
        NodeId node105(LargeInteger(0, 105));

        header = CreateNodeHeader(105, NodePhase::Routing);
        table.Consider(header, false);
        partnerNode = table.Get(node105);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableSetShutdownAndUnknownTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        size_t tableNodes[] = {90, 110, 120};
        FillTable(table, tableNodes, 3);

        PartnerNodeSPtr partnerNode;

        NodeId correctNodeId(LargeInteger(0, 90));
        NodeInstance correctInstance(correctNodeId, 1);
        NodeInstance wrongInstancePast(correctNodeId, 0);
        NodeInstance wrongInstanceFuture(correctNodeId, 2);

        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        // Try to shutdown with a 'past' instance (no-op)
        table.SetShutdown(wrongInstancePast, L"");
        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        // Try to shutdown with a 'future' instance (success)
        table.SetShutdown(wrongInstanceFuture, L"");
        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        // Try to shutdown the exact instance (success)
        correctNodeId = NodeId(LargeInteger(0, 110));
        correctInstance = NodeInstance(correctNodeId, 1);

        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        table.SetShutdown(correctInstance, L"");
        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        // Double shutdown (no-op)
        table.SetShutdown(correctInstance, L"");
        partnerNode = table.Get(correctNodeId);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Shutdown);

        // Try to make a partner unknown (success)
        VERIFY_IS_TRUE(!partnerNode->IsUnknown);
        table.SetUnknown(correctInstance, L"");
        VERIFY_IS_TRUE(partnerNode->IsUnknown);

        // Try to make a previous or future instance unknown (no-op)
        correctNodeId = NodeId(LargeInteger(0, 120));
        correctInstance = NodeInstance(correctNodeId, 1);
        wrongInstancePast = NodeInstance(correctNodeId, 0);
        wrongInstanceFuture = NodeInstance(correctNodeId, 2);

        partnerNode = table.Get(correctInstance);
        VERIFY_IS_TRUE(partnerNode->Phase == NodePhase::Routing);

        VERIFY_IS_TRUE(!partnerNode->IsUnknown);
        table.SetUnknown(wrongInstancePast, L"");
        VERIFY_IS_TRUE(!partnerNode->IsUnknown);

        table.SetUnknown(wrongInstanceFuture, L"");
        VERIFY_IS_TRUE(!partnerNode->IsUnknown);

        table.SetUnknown(correctInstance, L"");
        VERIFY_IS_TRUE(partnerNode->IsUnknown);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableFindClosestTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        size_t tableNodes[] = {60, 70, 80, 90, 110, 120, 130, 140, 150, 160};
        FillTable(table, tableNodes, 10);

        PartnerNodeSPtr node;
        NodeId node130(LargeInteger(0, 130));
        NodeId node140(LargeInteger(0, 140));
        NodeId node150(LargeInteger(0, 150));
        NodeId node160(LargeInteger(0, 160));

        // Test all interesting points for Node 140
        // Range is defined as (low midpoint, high midpoint]
        node = table.FindClosest(NodeId(LargeInteger(0, 140)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        node = table.FindClosest(NodeId(LargeInteger(0, 139)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        node = table.FindClosest(NodeId(LargeInteger(0, 141)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        node = table.FindClosest(NodeId(LargeInteger(0, 136)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        node = table.FindClosest(NodeId(LargeInteger(0, 145)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        node = table.FindClosest(NodeId(LargeInteger(0, 135)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node130);

        node = table.FindClosest(NodeId(LargeInteger(0, 146)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node150);

        // Unknown\Shutdown nodes should not be found...
        table.SetShutdown(NodeInstance(node150, 1), L"");
        node = table.FindClosest(NodeId(LargeInteger(0, 152)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node160);

        table.SetUnknown(NodeInstance(node160, 1), L"");
        node = table.FindClosest(NodeId(LargeInteger(0, 152)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node140);

        // Unknown nodes SHOULD be found if there are no other Routing nodes available
        table.SetShutdown(NodeInstance(NodeId(LargeInteger(0, 110)), 1), L"");
        table.SetShutdown(NodeInstance(NodeId(LargeInteger(0, 120)), 1), L"");
        table.SetShutdown(NodeInstance(NodeId(LargeInteger(0, 130)), 1), L"");
        table.SetShutdown(NodeInstance(NodeId(LargeInteger(0, 140)), 1), L"");

        node = table.FindClosest(NodeId(LargeInteger(0, 152)), L"");
        VERIFY_IS_TRUE(node->Instance.Id == node160);

        // Null should be returned if FindClosest fails
        SiteNodeSPtr sitePtrEmpty = CreateSiteNode(0);
        RoutingTable emptyTable(sitePtrEmpty, 2);

        node = emptyTable.FindClosest(NodeId(LargeInteger(0, 0)), L"");
        VERIFY_IS_TRUE(!node);

        OpenSiteNode(sitePtrEmpty);
        emptyTable.CloseSiteNode();
        CloseSiteNode(sitePtrEmpty);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableGetRoutingHopTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        size_t tableNodes[] = {90, 110, 120, 130, 140, 150};
        FillTable(table, tableNodes, 6);

        PartnerNodeSPtr node;
        bool ownsToken = false;
        NodeId node100(LargeInteger(0, 100));
        NodeId node130(LargeInteger(0, 130));
        NodeId node140(LargeInteger(0, 140));
        NodeId node150(LargeInteger(0, 150));

		sitePtr->Test_SetToken(RoutingToken(NodeIdRange(LargeInteger(0, 96), LargeInteger(0, 105)), 1));

        // Check a routing hop arriving at the current node
        node = table.GetRoutingHop(NodeId(LargeInteger(0, 100)), L"", 0, ownsToken);
        VERIFY_IS_TRUE(node->Instance.Id == node100);
        VERIFY_IS_TRUE(ownsToken);

        node = table.GetRoutingHop(NodeId(LargeInteger(0, 102)), L"", 0, ownsToken);
        VERIFY_IS_TRUE(node->Instance.Id == node100);
        VERIFY_IS_TRUE(ownsToken);

        // Check a routing hop arriving at some other node
        node = table.GetRoutingHop(NodeId(LargeInteger(0, 142)), L"", 0, ownsToken);
        VERIFY_IS_TRUE(node->Instance.Id == node140);
        VERIFY_IS_TRUE(!ownsToken);

        // Unknown nodes should again not be found for a hop...
        table.SetUnknown(NodeInstance(node140, 1), L"");
        node = table.GetRoutingHop(NodeId(LargeInteger(0, 142)), L"", 0, ownsToken);
        VERIFY_IS_TRUE(node->Instance.Id == node150);
        VERIFY_IS_TRUE(!ownsToken);

        table.SetShutdown(NodeInstance(node150, 1), L"");
        node = table.GetRoutingHop(NodeId(LargeInteger(0, 142)), L"", 0, ownsToken);
        VERIFY_IS_TRUE(node->Instance.Id == node130);
        VERIFY_IS_TRUE(!ownsToken);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableGetHoodTest)
    {
        FederationConfig::Test_Reset();

        FederationConfig::GetConfig().NeighborhoodSize = 2;

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        PartnerNodeSPtr node;
        NodeIdRange nodeIdRange;
        NodeId node90(LargeInteger(0, 90));
        NodeId node110(LargeInteger(0, 110));
        NodeId node120(LargeInteger(0, 120));
        NodeId node140(LargeInteger(0, 140));
        NodeId node150(LargeInteger(0, 150));
        vector<PartnerNodeSPtr> hoodNodes;
        vector<PartnerNodeSPtr> extHoodNodes;

        // Try an empty table (0 table nodes)
        table.GetExtendedHood(extHoodNodes);
        nodeIdRange = table.GetHood(hoodNodes);

        VERIFY_IS_TRUE(nodeIdRange.IsFull);
        VERIFY_IS_TRUE(hoodNodes.size() == 0);
        VERIFY_IS_TRUE(extHoodNodes.size() == 0);

        // Test wrap around + size == 1 (1 table node)
        size_t tableNodes[] = {90};
        FillTable(table, tableNodes, 1);

        hoodNodes.clear();
        extHoodNodes.clear();
        table.GetExtendedHood(extHoodNodes);
        nodeIdRange = table.GetHood(hoodNodes);

        VERIFY_IS_TRUE(nodeIdRange.IsFull);
        VERIFY_IS_TRUE(hoodNodes.size() == 1);
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node90));

        VERIFY_IS_TRUE(extHoodNodes.size() == 1);
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node90));

        // Test wrap around + size < full neighborhood (3 table nodes)
        size_t tableNodes1[] = {110, 120};
        FillTable(table, tableNodes1, 2);

        hoodNodes.clear();
        extHoodNodes.clear();
        table.GetExtendedHood(extHoodNodes);
        nodeIdRange = table.GetHood(hoodNodes);

        VERIFY_IS_TRUE(nodeIdRange.IsFull);
        VERIFY_IS_TRUE(hoodNodes.size() == 3);
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node120));

        VERIFY_IS_TRUE(extHoodNodes.size() == 3);
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node120));

        // Test wrap around + size > full neighborhood (6 table nodes)
        size_t tableNodes2[] = {130, 140, 150 };
        FillTable(table, tableNodes2, 3);

        hoodNodes.clear();
        extHoodNodes.clear();
        table.GetExtendedHood(extHoodNodes);
        nodeIdRange = table.GetHood(hoodNodes);

        VERIFY_IS_TRUE(nodeIdRange.Begin == node150);
        VERIFY_IS_TRUE(nodeIdRange.End == node120);
        VERIFY_IS_TRUE(hoodNodes.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node150));

        VERIFY_IS_TRUE(extHoodNodes.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node150));

        // All nodes should be included for GetHood
        // Only unknown nodes should be included for GetExtendedHood
        table.SetShutdown(NodeInstance(node90, 1), L"");
        table.SetUnknown(NodeInstance(node150, 1), L"");

        hoodNodes.clear();
        extHoodNodes.clear();
        table.GetExtendedHood(extHoodNodes);
        nodeIdRange = table.GetHood(hoodNodes);

        VERIFY_IS_TRUE(nodeIdRange.Begin == node150);
        VERIFY_IS_TRUE(nodeIdRange.End == node120);
        VERIFY_IS_TRUE(hoodNodes.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(hoodNodes, node150));

        VERIFY_IS_TRUE(extHoodNodes.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node140));
        VERIFY_IS_TRUE(ContainsExactlyOne(extHoodNodes, node150));

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableCompactTest)
    {
        FederationConfig::Test_Reset();

        FederationConfig::GetConfig().NeighborhoodSize = 2;

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        NodeIdRange nodeIdRange;
        NodeId node80(LargeInteger(0, 80));
        NodeId node120(LargeInteger(0, 120));
        vector<PartnerNodeSPtr> hoodNodes;

        size_t tableNodes[] = {70, 80, 90, 110, 120, 130};
        FillTable(table, tableNodes, 6);
        table.SetShutdown(NodeInstance(node120, 1), L"");

        VERIFY_IS_TRUE(table.Size == 7);
        nodeIdRange = table.GetHood(hoodNodes);
        VERIFY_IS_TRUE(nodeIdRange.Begin == node80 && nodeIdRange.End == node120);

        table.TestCompact();
        VERIFY_IS_TRUE(table.Size == 5);
        nodeIdRange = table.GetHood(hoodNodes);
        VERIFY_IS_TRUE(nodeIdRange.Begin == node80 && nodeIdRange.End == node120);

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableGetPingTargetsTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);
        OpenSiteNode(sitePtr );

        RoutingTable & table = sitePtr->Table;

        PartnerNodeSPtr node;
        NodeIdRange nodeIdRange;
        NodeId node90(LargeInteger(0, 90));
        NodeId node110(LargeInteger(0, 110));
        NodeId node120(LargeInteger(0, 120));
        NodeId node150(LargeInteger(0, 150));
        vector<PartnerNodeSPtr> pingTargets;

        // Try an empty table (0 table nodes)
        table.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 0);

        // 1 ping target
        size_t tableNodes[] = {90};
        FillTable(table, tableNodes, 1);

        pingTargets.clear();
        table.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 1);
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node90));

        // Returns the immediate pred+succ but not the edge node 120?
        size_t tableNodes1[] = {110, 120};
        FillTable(table, tableNodes1, 2);

        pingTargets.clear();
        table.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 2);
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node110));

        // Return the immediate pred+succ and both edges
        size_t tableNodes2[] = {130, 140, 150};
        FillTable(table, tableNodes2, 3);

        pingTargets.clear();
        table.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node90));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node150));

        // Shutdown nodes should not be included for GetPingTargets
        table.SetShutdown(NodeInstance(node90, 1), L"");
        table.SetUnknown(NodeInstance(node150, 1), L"");

        pingTargets.clear();
        table.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 3);
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node110));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node120));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node150));

        CloseSiteNode(sitePtr);

        // Verify that only the pred+succ and edges are returned by using a larger neighborhood
        FederationConfig::GetConfig().NeighborhoodSize = 4;

        sitePtr = CreateSiteNode(300);
        OpenSiteNode(sitePtr);

        RoutingTable & newTable = sitePtr->Table;

        size_t tableNodes3[] = {290, 280, 270, 260, 250, 310, 320, 330, 340, 350};
        FillTable(newTable, tableNodes3, 10);

        NodeId node260(LargeInteger(0, 260));
        NodeId node290(LargeInteger(0, 290));
        NodeId node310(LargeInteger(0, 310));
        NodeId node340(LargeInteger(0, 340));

        pingTargets.clear();
        newTable.GetPingTargets(pingTargets);

        VERIFY_IS_TRUE(pingTargets.size() == 4);
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node260));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node290));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node310));
        VERIFY_IS_TRUE(ContainsExactlyOne(pingTargets, node340));

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableSplitTokenTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(256);
        OpenSiteNode(sitePtr);

        RoutingTable & table = sitePtr->Table;

        PartnerNodeSPtr partnerNodePred = CreatePtr(128, NodePhase::Routing, sitePtr);
        PartnerNodeSPtr partnerNodeSucc = CreatePtr(384, NodePhase::Routing, sitePtr);
        vector<FederationRoutingTokenHeader> tokens;

        // Token should start as Full and with a version of 1
        VERIFY_IS_TRUE(sitePtr->Token.Version == 1);
        VERIFY_IS_TRUE(sitePtr->Token.IsFull);

        table.TrySplitToken(partnerNodePred, tokens);
        table.TrySplitToken(partnerNodeSucc, tokens);

        // Range is defined as (low midpoint, high midpoint]
        VERIFY_IS_TRUE(tokens.size() == 2);
        VERIFY_IS_TRUE(sitePtr->Token.Version == 3);
        VERIFY_IS_TRUE(sitePtr->Token.Range.Begin == NodeId(LargeInteger(0, 193)));
        VERIFY_IS_TRUE(sitePtr->Token.Range.End == NodeId(LargeInteger(0, 320)));

        CloseSiteNode(sitePtr);

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(RoutingTableProcessNeighborHeadersFailedTest)
    {
        FederationConfig::Test_Reset();

        SiteNodeSPtr sitePtr = CreateSiteNode(100);

        OpenSiteNode(sitePtr);
        RoutingTable & table = sitePtr->Table;

        auto checkException = [](RoutingTable & routingTable, MessageUPtr & message)
        {
            bool exceptionThrown = false;
            Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;

            try
            {
                NodeInstance from(NodeId::MinNodeId, 0);
                routingTable.ProcessNeighborHeaders(*message, from, L"", true);
            }
            catch (system_error error)
            {
                exceptionThrown = true;
            }

            VERIFY_IS_TRUE(exceptionThrown);
        };

        // More than 1 NeighborhoodRange header is invalid
        FederationNeighborhoodRangeHeader neighborhoodRangeHeader;
        auto badMessageTooManyRanges = FederationMessage::GetTokenTransfer().CreateMessage();
        badMessageTooManyRanges->Headers.Add<FederationNeighborhoodRangeHeader>(neighborhoodRangeHeader);
        badMessageTooManyRanges->Headers.Add<FederationNeighborhoodRangeHeader>(neighborhoodRangeHeader);
        checkException(table, badMessageTooManyRanges);

        // More than 2 RoutingToken headers is invalid
        FederationRoutingTokenHeader tokenHeader;
        auto badMessageTooManyTokens = FederationMessage::GetTokenTransfer().CreateMessage();
        badMessageTooManyTokens->Headers.Add<FederationRoutingTokenHeader>(tokenHeader);
        badMessageTooManyTokens->Headers.Add<FederationRoutingTokenHeader>(tokenHeader);
        badMessageTooManyTokens->Headers.Add<FederationRoutingTokenHeader>(tokenHeader);
        checkException(table, badMessageTooManyTokens);

        CloseSiteNode(sitePtr);
        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
