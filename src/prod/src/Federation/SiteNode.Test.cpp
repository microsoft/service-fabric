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
    using namespace Federation;
    using namespace Transport;

    const wstring TestSource = L"SiteNode.Test";

    class SiteNodeTests
    {
    public:
        void MisconfigurationTestHelper(wstring host, wstring port)
        {
            FederationConfig::Test_Reset();

            NodeId seedNodeId(LargeInteger(0, 0));

            VoteConfig seedNodes;
            seedNodes.push_back(VoteEntryConfig(seedNodeId, Federation::Constants::SeedNodeVoteType, host + L":" + port));

            FederationConfig::GetConfig().Votes = seedNodes;

            SiteNodeSPtr siteNodeSPtr = SiteNodeHelper::CreateSiteNode(seedNodeId);


            FederationConfig::Test_Reset();
        }
    };

    //This is a basic Single node leak test. Most FederationTest open multiple nodes and verify there are no leaks after Close/Abort
    //which gives us coverage for the multi node leak.
    BOOST_FIXTURE_TEST_SUITE(SiteNodeTestsSuite,SiteNodeTests)

    BOOST_AUTO_TEST_CASE(SiteNodeMemoryLeak)
    {
        FederationConfig::Test_Reset();

        ManualResetEvent stopper(false);

        wstring host = SiteNodeHelper::GetLoopbackAddress();
        wstring port = SiteNodeHelper::GetFederationPort();

        VoteConfig seedNodes;
        seedNodes.push_back(VoteEntryConfig(NodeId::MinNodeId, Federation::Constants::SeedNodeVoteType, host + L":" + port));

        FederationConfig::GetConfig().Votes = seedNodes;

        SiteNodeSPtr node = SiteNodeHelper::CreateSiteNode(NodeId::MinNodeId, host, port);

        node->BeginOpen(TimeSpan::FromSeconds(60), 
            [&, node] (AsyncOperationSPtr const& asyncOperation) -> void
        {
            ErrorCode x = node->EndOpen(asyncOperation);
            ASSERT_IFNOT(x.IsSuccess(), "Open fail {0}", x);
            stopper.Set();
        } );
        stopper.WaitOne();

        stopper.Reset();
        node->BeginClose(TimeSpan::FromSeconds(60), [&, node] (AsyncOperationSPtr const& asyncOperation) -> void
        {
            ErrorCode x = node->EndClose(asyncOperation);
            ASSERT_IFNOT(x.IsSuccess(), "Close fail {0}", x);
            stopper.Set();
        } );
        stopper.WaitOne();

        Sleep(2000);

        ASSERT_IF(
            node.use_count() != 1,
            "SiteNode smart pointer still has {0} references.",
            node.use_count());

        FederationConfig::Test_Reset();
        SiteNodeHelper::DeleteTicketFile(NodeId::MinNodeId);
    }

    BOOST_AUTO_TEST_CASE(SiteNodeOpenFailsWithAddressAlreadyInUse)
    {
        FederationConfig::Test_Reset();

        ManualResetEvent stopper(false);

        wstring host = SiteNodeHelper::GetLoopbackAddress();
        wstring port = SiteNodeHelper::GetFederationPort();

        VoteConfig seedNodes;
        seedNodes.push_back(VoteEntryConfig(NodeId::MinNodeId, Federation::Constants::SeedNodeVoteType, host + L":" + port));

        FederationConfig::GetConfig().Votes = seedNodes;

        SiteNodeSPtr node = SiteNodeHelper::CreateSiteNode(NodeId::MinNodeId, host, port);

        node->BeginOpen(TimeSpan::FromSeconds(60), 
            [&, node] (AsyncOperationSPtr const& asyncOperation) -> void
        {
            ErrorCode x = node->EndOpen(asyncOperation);
            ASSERT_IFNOT(x.IsSuccess(), "Open fail {0}", x);
            stopper.Set();
        } );
        stopper.WaitOne();

        // Now open another siteNode
        stopper.Reset();
        SiteNodeSPtr node2 = SiteNodeHelper::CreateSiteNode(NodeId::MaxNodeId, host, port);

        node2->BeginOpen(TimeSpan::FromSeconds(60),
            [&, node2] (AsyncOperationSPtr const& asyncOperation) -> void
        {
            ErrorCode x = node2->EndOpen(asyncOperation);
            ASSERT_IF(x.ReadValue() != ErrorCodeValue::AddressAlreadyInUse, "Open fail {0}", x);
            stopper.Set();
        } );

        stopper.WaitOne();

        stopper.Reset();
        node->BeginClose(TimeSpan::FromSeconds(60), [&, node] (AsyncOperationSPtr const& asyncOperation) -> void
        {
            ErrorCode x = node->EndClose(asyncOperation);
            ASSERT_IFNOT(x.IsSuccess(), "Close fail {0}", x);

            stopper.Set();
        } );
        stopper.WaitOne();

        Sleep(2000);

        ASSERT_IF(
            node.use_count() != 1,
            "SiteNode smart pointer still has {0} references.",
            node.use_count());

        ASSERT_IF(
            node2.use_count() != 1,
            "SiteNode smart pointer still has {0} references.",
            node.use_count());

        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(MisconfiguredHost) //, microsoft::windows_error::assertion_failure)
    {
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(MisconfigurationTestHelper(L"137.0.0.1", L"12345"), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(MisconfiguredPort) //, microsoft::windows_error::assertion_failure)
    {
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(MisconfigurationTestHelper(L"127.0.0.1", L"22345"), std::system_error);
    }

    BOOST_AUTO_TEST_CASE(MisconfiguredSmallPort) //, microsoft::windows_error::assertion_failure)
    {
        Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
        VERIFY_THROWS(MisconfigurationTestHelper(L"127.0.0.1", L"2345"), std::system_error);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
