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

    const StringLiteral TraceType("FederationSubsystemTest");

    class FederationSubsystemTests
    {
    };

    class Root;
    typedef shared_ptr<Root> RootSPtr;

    class Root : public ComponentRoot
    {
    public:
        static RootSPtr Create(NodeId nodeId)
        {
            RootSPtr root = make_shared<Root>();

            wstring host = SiteNodeHelper::GetLoopbackAddress();
            wstring port = SiteNodeHelper::GetFederationPort();

            VoteConfig seedNodes;
            seedNodes.push_back(VoteEntryConfig(NodeId::MinNodeId, Federation::Constants::SeedNodeVoteType, host + L":" + port));
            FederationConfig & federationConfig = FederationConfig::GetConfig();
            federationConfig.Votes = seedNodes;

            wstring addr = SiteNodeHelper::BuildAddress(host, port);
            wstring laAddr = SiteNodeHelper::BuildAddress(host, SiteNodeHelper::GetLeaseAgentPort());
            NodeConfig config(nodeId, addr, laAddr, SiteNodeHelper::GetWorkingDir());

            Transport::SecuritySettings defaultSettings;
            root->FS = make_shared<FederationSubsystem>(config, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, *root);

            return root;
        }

        FederationSubsystemSPtr FS;
    };

    BOOST_FIXTURE_TEST_SUITE(FederationSubsystemTestsSuite,FederationSubsystemTests)

    BOOST_AUTO_TEST_CASE(OpenCloseTest)
    {
        NodeId nodeId = NodeId::MinNodeId;
        SiteNodeHelper::DeleteTicketFile(nodeId);
        FederationConfig::Test_Reset();
        AutoResetEvent closeCompleted(false);

        {
            RootSPtr root = Root::Create(nodeId);

            AutoResetEvent openCompleted(false);
            root->FS->BeginOpen(
                TimeSpan::MaxValue,
                [&root, &openCompleted] (AsyncOperationSPtr const& operation)
                { 
                    VERIFY_IS_TRUE(root->FS->EndOpen(operation).IsSuccess());
                    openCompleted.Set();
                },
                root->FS->Root.CreateAsyncOperationRoot());
            Trace.WriteInfo(TraceType, "Waiting for open to complete ...");
            VERIFY_IS_TRUE(openCompleted.WaitOne(TimeSpan::FromMinutes(5)));

            root->FS->BeginClose(
                TimeSpan::FromSeconds(60),
                [&root, &closeCompleted] (AsyncOperationSPtr const& operation)
                { 
                    ErrorCode error = root->FS->EndClose(operation);
                    VERIFY_IS_TRUE(error.IsSuccess());
                    Sleep(1000); // Add delay to prove "root" is kept alive by close AsyncOperation
                    closeCompleted.Set();
                },
                root->FS->Root.CreateAsyncOperationRoot());
        }

        Trace.WriteInfo(TraceType, "Waiting for close to complete ...");
        VERIFY_IS_TRUE(closeCompleted.WaitOne(TimeSpan::FromMinutes(5)));

        FederationConfig::Test_Reset();
        SiteNodeHelper::DeleteTicketFile(NodeId::MinNodeId);
    }

    BOOST_AUTO_TEST_SUITE_END()

}
