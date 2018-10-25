// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace Client;
    using namespace Federation;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;
    using namespace std;


    class RebuildTest
    {
    protected:

        RebuildTest() { BOOST_REQUIRE(ClassSetup()); }
        TEST_CLASS_SETUP(ClassSetup);

        ~RebuildTest() { BOOST_REQUIRE(ClassCleanup()); }
        TEST_CLASS_CLEANUP(ClassCleanup);

        void TestRebuild(
			wstring const & testName,
            wstring expectedFailoverUnitStr,
            vector<wstring> reports);

        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;
    };



    void RebuildTest::TestRebuild(
		wstring const & testName,
        wstring expectedFailoverUnitStr,
        vector<wstring> reports)
    {
		Trace.WriteInfo("RebuildTestSource", "{0}", testName);

        // Remove extra spaces.
        StringUtility::Replace<wstring>(expectedFailoverUnitStr, L"   ", L" ");
        StringUtility::Replace<wstring>(expectedFailoverUnitStr, L"  ", L" ");
        for (size_t i = 0; i < reports.size(); ++i)
        {
            StringUtility::Replace<wstring>(reports[i], L"   ", L" ");
            StringUtility::Replace<wstring>(reports[i], L"  ", L" ");
        }

        std::vector<NodeInfoSPtr> nodes;
        NodeCache nodeCache(*fm_, nodes);

        // Generate the first report.
        unique_ptr<FailoverUnitInfo> failoverUnitInfo;
        NodeInstance orign = TestHelper::FailoverUnitInfoFromString(reports[0], failoverUnitInfo);

        FabricVersionInstance versionInstance;
        nodeCache.NodeUp(TestHelper::CreateNodeInfo(orign), false /* IsVersionGatekeepingNeeded */, versionInstance);

        InBuildFailoverUnitUPtr inBuildFailoverUnit;
        if (failoverUnitInfo)
        {
            // Create the InBuildFailoverUnit.
            inBuildFailoverUnit = make_unique<InBuildFailoverUnit>(
                failoverUnitInfo->FailoverUnitDescription.FailoverUnitId,
                failoverUnitInfo->FailoverUnitDescription.ConsistencyUnitDescription,
                failoverUnitInfo->ServiceDescription);

            // Add the first report.
            inBuildFailoverUnit->Add(*failoverUnitInfo, orign, *fm_);
        }

        // Add the rest of the reports.
        for (size_t i = 1; i < reports.size(); ++i)
        {
            failoverUnitInfo = nullptr;
            orign = TestHelper::FailoverUnitInfoFromString(reports[i], failoverUnitInfo);
            nodeCache.NodeUp(TestHelper::CreateNodeInfo(orign), false /* IsVersionGatekeepingNeeded */, versionInstance);

            if (failoverUnitInfo)
            {
                inBuildFailoverUnit->Add(*failoverUnitInfo, orign, *fm_);
            }
        }

        FailoverUnitUPtr actualFailoverUnit = inBuildFailoverUnit->Generate(nodeCache, false);

        if (actualFailoverUnit)
        {
            vector<ApplicationInfoSPtr> applications;
            vector<ServiceTypeSPtr> serviceTypes;
            vector<ServiceInfoSPtr> services;

            applications.push_back(make_shared<ApplicationInfo>(ServiceModel::ApplicationIdentifier(L"TestApp", 0), NamingUri(L"fabric:/TestApp"), 0));
            serviceTypes.push_back(make_shared<ServiceType>(inBuildFailoverUnit->Description.Type, nullptr));
            services.push_back(make_shared<ServiceInfo>(inBuildFailoverUnit->Description, nullptr, FABRIC_INVALID_SEQUENCE_NUMBER, false));

            ServiceCache serviceCache(*fm_, fm_->Store, fm_->PLB, applications, serviceTypes, services);

            for (auto replica = actualFailoverUnit->BeginIterator; replica != actualFailoverUnit->EndIterator; ++replica)
            {
                if (!nodeCache.GetNode(replica->FederationNodeId))
                {
                    nodeCache.NodeDown(replica->FederationNodeInstance);
                }
            }

            actualFailoverUnit->UpdatePointers(*fm_, nodeCache, serviceCache);
        }

        if (actualFailoverUnit)
        {
            TestHelper::AssertEqual(expectedFailoverUnitStr, TestHelper::FailoverUnitToString(actualFailoverUnit), testName);
        }
        else if (expectedFailoverUnitStr != L"")
        {
            wstring result;
            StringWriter w(result);

            w.Write("\n{0}\nExpected:\n{1}\nActual: NotGenerated.  InBuildFT:\n{2}\n", testName, expectedFailoverUnitStr, inBuildFailoverUnit);
            VERIFY_FAIL(result.c_str());
        }
    }

    BOOST_FIXTURE_TEST_SUITE(RebuildTestSuite,RebuildTest)

    BOOST_AUTO_TEST_CASE(TestRebuildStateless)
    {
        vector<wstring> reports;
        reports.push_back(L"0|3 0 - 000/111 F [0 N/N RD - Up]                                ");
        reports.push_back(L"1|3 0 - 000/111 F                 [1 N/N RD - Up]                ");
        reports.push_back(L"2|3 0 - 000/111 F                                 [2 N/N RD - Up]");
        TestRebuild(
            L"Stateless service",
            L"3 0 - 000/111 [0 N/N RD - Up] [1 N/N RD - Up] [2 N/N RD - Up]",
            reports);
    }

    BOOST_AUTO_TEST_CASE(TestRebuildPrimaryUpAndReady)
    {
        vector<wstring> reports;
        reports.push_back(L"0|3 2 S 000/122 T [0 N/P RD - Up]");
        TestRebuild(
            L"One primary",
            L"3 2 SE 000/122 [0 N/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 S 122/133 T [0 N/P IB - Up]");
        TestRebuild(
            L"Primary in-build",
            L"3 2 SE 122/133 [0 N/P IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/I IB - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F                 [1 N/I IB - Up]");
        TestRebuild(
            L"One primary and one IB idle replica - I",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1 N/I IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/I IB - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F                 [1 N/I RD - Up]");
        TestRebuild(
            L"One primary and one IB idle replica - II",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1 N/I RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [1 N/I RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 T [1 N/I IB - Up] [2 N/P RD - Up]");
        TestRebuild(
            L"One primary and one IB idle replica - III",
            L"3 2 SPE 000/122 [1 N/I RD - Up] [2 N/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [1 N/I IB - Up]");
        reports.push_back(L"2|3 2 SP 000/122 T [1 N/I RD - Up] [2 N/P RD - Up] ");
        TestRebuild(
            L"One primary and one IB idle replica - IV",
            L"3 2 SPE 000/122 [1 N/I RD - Up] [2 N/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 2 SP 000/122 T [1 N/I RD - Up] [2 N/P RD - Up] ");
        reports.push_back(L"1|3 2 SP 000/122 F [1 N/I IB - Up]");
        TestRebuild(
            L"One primary and one IB idle replica - IIV",
            L"3 2 SPE 000/122 [1 N/I RD - Up] [2 N/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/I RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F                 [1 N/I RD - Up]");
        TestRebuild(
            L"One primary and one idle replica",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1 N/I RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/I RD - Down]");
        TestRebuild(
            L"One primary and one idle Down replica",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1 N/I RD N Down]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up]");
        TestRebuild(
            L"One primary and one secondary replica",
            L"3 2 SP 000/122 [0 N/P RD - Up] [1 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/S RD - Down]");
        TestRebuild(
            L"One primary and one secondary Down replica",
            L"3 2 SP 000/122 [0 N/P RD - Up] [1 N/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 S/I RD - Down]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up  ]");
        TestRebuild(
            L"Reconfiguration to move Down secondary out of configuration",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Up] [2 S/I RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"One primary and two Down secondary replicas",
            L"3 2 SP 000/122 [0 N/P RD - Up] [1 N/S RD - Down] [2 N/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up]                ");
        reports.push_back(L"2|3 2 SP 000/122 F                                 [2 N/I RD - Up]");
        TestRebuild(
            L"Reconfiguration started to promote idle replica to secondary",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up]                ");
        reports.push_back(L"2|3 2 SP 000/123 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration in progress to promote idle replica to secondary",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up]                ");
        reports.push_back(L"2|3 2 SP 000/123 F                                 [2 I/N RD - Up]");
        TestRebuild(
            L"Reconfiguration in progress, idle reports after GetLSN",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1|3 2 SP 122/123 F [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 F                                 [2 N/I RD - Up]");
        TestRebuild(
            L"Reconfiguration in progress, replicas report after deactivation",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/123 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Secondary Down during reconfiguration",
            L"3 2 SP 122/123 [0 P/P RD - Up] [1 S/S RD - Down] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0  |3 2 SP 000/122 T [0 N/P RD - Up] [1     N/I RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F                 [1:2:2 N/I SB - Up]");
        TestRebuild(
            L"One primary and one idle SB replica",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1:2:2 N/I SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0  |3 2 SP 000/122 T [0 N/P RD - Up] [1     N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F [0 N/P RD - Up] [1:2:2 N/S SB - Up]");
        TestRebuild(
            L"One primary and one secondary SB replica",
            L"3 2 SP 000/122 [0 N/P RD - Up] [1:2:2 N/S SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 000/122 T [0 N/P RD - Up] [1 N/I IB - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F                 [1 N/I SB - Up]");
        TestRebuild(
            L"One primary and one idle replica chaning state from SB to IB",
            L"3 2 SPE 000/122 [0 N/P RD - Up] [1 N/I IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0  |3 2 SP 000/123 T [0 N/P RD - Up] [1     N/I RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F                 [1:2:2 N/I SB - Up]");
        TestRebuild(
            L"One primary and one idle SB replica reporting an older epoch",
            L"3 2 SPE 000/123 [0 N/P RD - Up] [1:2:2 N/I SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/124 T [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1|3 2 SP 122/123 F [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 F                                 [2 N/I RD - Up]");
        TestRebuild(
            L"Reconfiguration in progress with replicas reporting different epochs",
            L"3 2 SP 122/124 [0 P/P RD - Up] [1 S/S RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0  |3 2 SP 123/124 T [0 P/P RD - Up] [1     S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/120 F [0 N/P RD - Up] [1:2:2 N/S SB - Up]                ");
        reports.push_back(L"2  |3 2 SP 000/123 F                                     [2 N/I RD - Up]");
        TestRebuild(
            L"StandBy replica reporting older epoch during reconfiguration",
            L"3 2 SP 123/124 [0 P/P RD - Up] [1:2:2 S/S SB - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/133 T [0 P/S RD - Up] [1 S/P RD - Up] [2 S/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/133 F [0 N/S RD - Up] [1 N/P RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration for swap primary, one RD primary replica",
            L"3 2 SPW 122/133 [0 P/S RD - Up] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0|3 2 SP 122/123 T [0 P/S RD - Up] [1 S/P RD - Up] [2 S/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/123 F [0 N/S RD - Up] [1 N/P RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/123 F [0 N/S RD - Up] [1 N/P RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration for swap primary, two RD primary replicas",
            L"3 2 SPW 122/123 [0 P/S RD - Up] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [0 N/P RD - Up] [1:2:2 N/S SB - Up  ] [2 N/S RD - Up]                                ");
        reports.push_back(L"3  |3 2 SP 000/145 T                 [1     N/I RD - Down]                 [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"4  |3 2 SP 000/145 F                                                       [3 N/P RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"StandBy replica reporting a very old epoch",
            L"3 2 SP 000/145 [0 N/I RD - Down] [1:2:2 N/I SB - Up] [2 N/I RD - Down] [3 N/P RD - Up] [4 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 122/123 F [0 P/P RD - Up] [1:2:2 S/S SB - Up  ] [2 I/S RD - Up]                                ");
        reports.push_back(L"3  |3 2 SP 000/145 T                 [1     N/I RD - Down]                 [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"4  |3 2 SP 000/145 F                                                       [3 N/P RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"StandBy replica reporting a very old reconfiguration",
            L"3 2 SP 000/145 [0 N/I RD - Down] [1:2:2 N/I SB - Up] [2 N/I RD - Down] [3 N/P RD - Up] [4 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [0 N/P RD - Up] [1:2:2 N/S SB - Up  ] [2 N/S RD - Up]                                ");
        reports.push_back(L"3  |3 2 SP 145/146 T                 [1     N/I RD - Down]                 [3 P/P RD - Up] [4 I/S RD - Up]");
        reports.push_back(L"4  |3 2 SP 000/146 F                                                       [3 N/P RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"StandBy replica reporting a very old epoch while RD replicas going through a reconfiguration",
            L"3 2 SP 145/146 [0 I/I RD - Down] [1:2:2 I/I SB - Up] [2 I/I RD - Down] [3 P/P RD - Up] [4 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|5 1 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up]                ");
        reports.push_back(L"2|5 1 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up]                ");
        reports.push_back(L"3|5 1 SP 000/122 T [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up] [4 N/I RD - Up]");
        TestRebuild(
            L"Idle replica that is only reported by the Primary",
            L"5 1 SP 000/122 [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up] [4 N/I RD N Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/123 T [1 P/P RD - Up] [2 S/N RD - Up] [3 S/S RD - Up] [4 I/S RD - Up]");
        reports.push_back(L"4|3 1 SP 000/123 F [1 N/P RD - Up]                 [3 N/S RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - Secondary reporting after Activate",
            L"3 1 SP 122/123 [1 P/P RD - Up] [2 S/N RD - Down] [3 S/S RD - Down] [4 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/000/123 T [1 P/N/P RD - Up] [2 S/N/N RD - Up] [3 S/N/S RD - Up] [4 I/N/S RD - Up]");
        reports.push_back(L"3|3 1 SP 122/000/123 F [1 P/N/P RD - Up] [2 S/N/S RD - Up] [3 S/N/S RD - Up]                  ");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - Secondary reporting after GetLSN",
            L"3 1 SP 122/123 [1 P/P RD - Up] [2 S/N RD - Down] [3 S/S RD - Up] [4 I/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/000/124 T [1 P/N/P RD - Up] [2 S/N/N RD - Up] [3 S/N/S RD - Up] [4 I/N/S RD - Up]");
        reports.push_back(L"3|3 1 SP 122/123/123 F [1 P/P/P RD - Up] [2 S/N/N RD - Up] [3 S/S/S RD - Up] [4 I/S/S RD - Up]");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - Secondary reporting after GetLSN->Deactivate",
            L"3 1 SP 122/124 [1 P/P RD - Up] [2 S/N RD - Down] [3 S/S RD - Up] [4 I/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/000/124 T [1 P/N/P RD - Up] [2 S/N/N RD - Up] [3 S/N/S RD - Up] [4 I/N/S RD - Up]");
        reports.push_back(L"3|3 1 SP 122/123/124 F [1 P/P/P RD - Up] [2 S/N/N RD - Up] [3 S/S/S RD - Up] [4 I/S/S RD - Up]");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - Secondary reporting after GetLSN->Deactivate->GetLSN",
            L"3 1 SP 122/124 [1 P/P RD - Up] [2 S/N RD - Down] [3 S/S RD - Up] [4 I/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/123 T [1 P/P RD - Up] [2 S/N RD - Up] [3 S/S RD - Up] [4 I/S RD - Up]");
        reports.push_back(L"2|3 1 SP 000/122 F [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]                ");
        reports.push_back(L"4|3 1 SP 000/123 F [1 N/P RD - Up]                 [3 N/S RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - I",
            L"3 1 SP 122/123 [1 P/P RD - Up] [2 S/N RD - Up] [3 S/S RD - Down] [4 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 1 SP 000/122 F [1 N/P RD - Up] [2 N/S RD - Up] [3 N/S RD - Up]                ");
        reports.push_back(L"1|3 1 SP 122/123 T [1 P/P RD - Up] [2 S/N RD - Up] [3 S/S RD - Up] [4 I/S RD - Up]");
        reports.push_back(L"4|3 1 SP 000/123 F [1 N/P RD - Up]                 [3 N/S RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"Reconfiguration going on to drop a secondary - II",
            L"3 1 SP 122/123 [1 P/P RD - Up] [2 S/N RD - Up] [3 S/S RD - Down] [4 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|4 1 S 000/122 F [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up]");
        reports.push_back(L"2|4 1 S 000/122 T [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up] [4 N/I DD - Down]");
        TestRebuild(
            L"A dropped replica that needs to be removed on the primary",
            L"4 1 S 000/122 [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S DD - Down] [4 N/I DD N Down]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 1 SP 122/133 F [1:2:3 P/P SB - Up] [2:1:1 S/S IB - Up] [3:1:2 S/S IB - Up]");
        reports.push_back(L"2:1|3 1 SP 111/133 F [1:1:1 P/P RD - Up] [2:1:2 S/S SB - Up] [3:1:1 S/S RD - Up]");
        reports.push_back(L"3:2|3 1 SP 111/144 T [1:2:2 P/S IB - Up] [2:1:1 S/S IB - Up] [3:2:2 S/P IB - Up]");
        TestRebuild(
            L"An InBuild replica changing role from Secondary to Primary during reconfiguration",
            L"3 1 SP 122/144 [1:2:3 P/S SB - Up] [2:1:2 S/S SB - Up] [3:2:2 S/P IB - Up]",
            reports);
    }

    BOOST_AUTO_TEST_CASE(TestRebuildPrimaryDownOrStandBy)
    {
        vector<wstring> reports;
        reports.push_back(L"1|2 1 S 000/123 F [1 N/I RD - Up]");
        TestRebuild(
            L"Only Idle replica is reporting",
            L"2 1 S 100/264 [1 N/I RD - Up]",
            reports);
    
        reports.clear();
        reports.push_back(L"1|3 1 S 000/122 F [1 N/S RD - Up] [2 N/P RD - Up]                ");
        reports.push_back(L"3|3 1 S 000/122 F                                 [3 N/I RD - Up]");
        TestRebuild(
            L"Non-persisted stateful primary is down",
            L"3 1 S 122/163 [1 S/P RD - Up] [2 P/N DD - Down] [3 I/I RD D Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Primary is Down",
            L"3 2 SP 122/163 [0 P/I RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0:2|3 2 SP 000/122 F [0:2:2 N/P SB - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"1  |3 2 SP 000/122 F [0     N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"2  |3 2 SP 000/122 F [0     N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Primary is StandBy",
            L"3 2 SP 122/163 [0:2:2 P/S SB - Up] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 122/133 T [0 P/I RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 F [0 N/P RD - Up  ] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Primary is down, reconfiguration to promote new primary",
            L"3 2 SP 122/133 [0 P/I RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0:2|3 2 SP 000/122 F [0:2:2 N/P SB - Up  ] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"1  |3 2 SP 122/133 T [0     P/I RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]");
        reports.push_back(L"2  |3 2 SP 000/122 F [0     N/P RD - Up  ] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Old primary is up as StandBy, reconfiguration going on to promote new primary",
            L"3 2 SP 122/133 [0:2:2 P/I SB - Up] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0:2|3 2 SP 122/123 F [0:2:2 P/P SB - Up  ] [1 S/S RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"1  |3 2 SP 122/144 T [0     P/S RD - Down] [1 S/P RD - Up] [2 I/S RD - Up]");
        reports.push_back(L"2  |3 2 SP 000/122 F                                       [2 N/I RD - Up]");
        TestRebuild(
            L"Old primary in reconfiguration is up as StandBy, reconfiguration going on to promote new primary",
            L"3 2 SP 122/144 [0:2:2 P/S SB - Up] [1 S/P RD - Up] [2 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"Primary and secondary is Down, one RD secondary is Up",
            L"3 2 SP 000/163 [0 N/P RD - Down] [1 N/S RD - Down] [2 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"0:2|3 2 SP 000/122 F [0:2:2 N/P SB - Up  ] [1     N/S RD - Up] [2     N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 122/133 F [0     P/I RD - Down] [1:2:2 S/P SB - Up] [2     S/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 F [0     N/P RD - Up  ] [1     N/S RD - Up] [2:2:2 N/S SB - Up]");
        TestRebuild(
            L"All replicas are SB reporting different epochs",
            L"3 2 SP 122/174 [0:2:2 P/I SB - Up] [1:2:2 S/P IB - Up] [2:2:2 S/S SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|5 1 SP 000/123 F [1:2:2 N/S SB - Up] [2     N/P RD - Up] [3 N/S RD - Up] [4     N/S RD - Up]");
        reports.push_back(L"2:2|5 1 SP 000/123 F [1     N/S RD - Up] [2:2:2 N/P SB - Up] [3 N/S RD - Up] [4     N/S RD - Up]");
        reports.push_back(L"4:2|5 1 SP 000/123 F [1     N/S RD - Up] [2     N/P RD - Up] [3 N/S RD - Up] [4:2:2 N/S SB - Up]");
        TestRebuild(
            L"Reports from only SB replicas with some replicas not reporting",
            L"5 1 SP 123/164 [1:2:2 S/S SB - Up] [2:2:2 P/P IB - Up] [3 S/I RD - Down] [4:2:2 S/S SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 000/122 F [1 N/I RD - Up]                                                  ");
        reports.push_back(L"2|3 1 SP 000/122 F                   [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"4|3 1 SP 000/122 F                   [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up]");
        TestRebuild(
            L"IB Idle replicas with Primary down",
            L"3 1 SP 122/163 [1 I/I IB - Up] [2 S/P RD - Up] [3 P/I RD - Down] [4 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1  |3 2 SP 122/000/134 F [1 S/N/S SB - Up] [2 P/N/P RD - Up  ] [3     S/N/S RD - Up]");
        reports.push_back(L"3:2|3 2 SP 122/000/134 T [1 S/N/I SB - Up] [2 P/N/S RD - Down] [3:2:2 S/N/P IB - Up]");
        TestRebuild(
            L"Old primary down during reconfiguration - Secondary reporting after GetLSN",
            L"3 2 SP 122/134 [1 S/I SB - Up] [2 P/S RD - Down] [3:2:2 S/P IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/000/202 F [1 N/N/P SB - Up]");
        TestRebuild(
            L"Initial primary is SB",
            L"3 2 SPE 202/243 [1 P/P IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/000/202 F [1 N/N/I SB - Up]                  ");
        reports.push_back(L"2|3 2 SP 000/000/202 F                   [2 N/N/P SB - Up]");
        TestRebuild(
            L"SB primary and an SB idle replica",
            L"3 2 SPE 202/243 [1 I/I SB - Up] [2 P/P IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:1|5 1 SP 000/111 F [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up] [4 N/S RD - Up] [5 N/S RD - Up]");
        reports.push_back(L"2:2|5 1 SP 111/112 F [1 S/S RD - Up] [2:2:2 P/P SB - Down] [3 S/S RD - Up] [4 S/N RD - Up] [5 S/S RD - Up]");
        reports.push_back(L"3:1|5 1 SP 000/111 F [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up] [4 N/S RD - Up] [5 N/S RD - Up]");
        reports.push_back(L"4:1|5 1 SP 000/111 F [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up] [4 N/S RD - Up] [5 N/S RD - Up]");
        reports.push_back(L"5:1|5 1 SP 000/111 F [1 N/S RD - Up] [2 N/P RD - Up] [3 N/S RD - Up] [4 N/S RD - Up] [5 N/S RD - Up]");
        TestRebuild(
            L"A secondary is going through S/N reconfiguration while primary is down",
            L"5 1 SP 111/153 [1 S/P RD - Up] [2:2:2 P/S SB - Down] [3 S/S RD - Up] [4 S/S RD - Up] [5 S/S RD - Up]",
            reports);
    }

    BOOST_AUTO_TEST_CASE(TestRebuildConfiguration)
    {
        vector<wstring> reports;
        reports.push_back(L"4:1|3 2 SP 000/111 F [0     N/P RD - Up  ] [4     N/S SB - Up] [5     N/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 121/122 F [0:2:2 P/P RD - Up  ] [1     S/S RD - Up] [2:2:2 I/S SB - Up]");
        reports.push_back(L"1:2|3 2 SP 121/133 F [0:2:2 P/S RD - Down] [1:2:2 S/P SB - Up] [2     I/S RD - Up]");
        TestRebuild(
            L"PC1 = PC2 && CC1 < CC2",
            L"",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        reports.push_back(L"2|3 2 SP 121/133 F [0 P/P RD - Up] [1 I/S RD - Up] [2 S/S RD - Up]");
        TestRebuild(
            L"PC2 < CC1 < CC2",
            L"3 2 SP 122/174 [0 P/S RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 2 SP 121/133 F [0 P/P RD - Up] [1 I/S RD - Up] [2 S/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/122 F [0 N/P RD - Up] [1 N/S RD - Up] [2 N/S RD - Up]");
        TestRebuild(
            L"PC1 < CC2 < CC1",
            L"3 2 SP 122/174 [0 P/S RD - Down] [1 S/P RD - Up] [2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 122/123 F [0 P/P RD - Up] [1 S/S RD - Up] [2 S/I RD - Up] [3 I/S RD - Up]");
        reports.push_back(L"3|3 2 SP 000/122 F                                                 [3 N/I RD - Up]");
        TestRebuild(
            L"CC has quorum but PC lost quorum, a RD secondary is Up",
            L"3 2 SP 122/164 [0 P/P RD - Down] [1 S/S RD - Up] [2 S/I RD - Down] [3 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 122/123 F [0 P/P RD - Up] [1 S/S RD - Up] [2 S/I RD - Up] [3 I/S RD - Up]");
        reports.push_back(L"3|3 2 SP 000/122 F [3 N/I RD - Up]");
        reports.push_back(L"2|3 2 SP 000/123 F [2 N/I RD - Up]");
        TestRebuild(
            L"Both quorum available",
            L"3 2 SP 122/164 [0 P/S RD - Down] [1 S/P RD - Up] [2 S/I RD - Up] [3 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/132 F [0 N/P RD - Up] [1:2:2 N/S SB - Up] [2     N/S RD - Up]                                                ");
        reports.push_back(L"2:2|3 2 SP 000/132 F [0 N/P RD - Up] [1     N/S RD - Up] [2:2:2 N/S SB - Up]                                                ");
        reports.push_back(L"4  |3 2 SP 000/112 F                                                         [4 N/P SB - Up] [5 N/S RD - Up] [6 N/S RD - Up]");
        TestRebuild(
            L"Stale CC2",
            L"3 2 SP 132/173 [0 P/I RD - Down] [1:2:2 S/P IB - Up] [2:2:2 S/S SB - Up] [4 I/I SB - Up] [5 I/I IB - Down] [6 I/I IB - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/132 F [0 N/P RD - Up] [1:2:2 N/S SB - Up] [2     N/S RD - Up]                ");
        reports.push_back(L"2:2|3 2 SP 000/132 F [0 N/P RD - Up] [1     N/S RD - Up] [2:2:2 N/S SB - Up]                ");
        reports.push_back(L"4  |3 2 SP 000/143 F                                                         [4 N/I RD - Up]");
        TestRebuild(
            L"Idle replica has higher epoch",
            L"3 2 SP 132/184 [0 P/I RD - Down] [1:2:2 S/P IB - Up] [2:2:2 S/S SB - Up] [4 I/I IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 122/000/133 F [1 S/N/S RD - Up] [2 S/N/S RD - Up] [3 P/N/P RD - Up]");
        reports.push_back(L"2|3 1 SP 122/000/133 F [1 S/N/S RD - Up] [2 S/N/S RD - Up] [3 P/N/P RD - Up]");
        TestRebuild(
            L"Secondary replicas reporting after GetLSN",
            L"3 1 SP 122/174 [1 S/P RD - Up] [2 S/S RD - Up] [3 P/I RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"4|5 1 SP 122/133/133 F [1 P/S/S RD - Down] [2 S/P/P RD - Up] [3 I/S/S RD - Up] [4 S/S/S RD - Up] [5 S/S/S RD - Up]");
        reports.push_back(L"5|5 1 SP 122/000/133 F [1 P/N/P RD - Down] [2 S/N/S RD - Up]                   [4 S/N/S RD - Up] [5 S/N/S RD - Up]");
        reports.push_back(L"3|5 1 SP 000/000/133 F                                       [3 I/N/N RD - Up]                                    ");
        TestRebuild(
            L"Secondary replicas reporting after Deactivate and GetLSN respectively, and Idle reporting after GetLSN",
            L"5 1 SP 122/174 [1 P/S RD - Down] [2 S/S RD - Down] [3 I/S RD - Up] [4 S/P RD - Up] [5 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"4|5 1 SP 122/000/134 F [1 P/N/P RD - Down] [2 S/N/S RD - Up]                   [4 S/N/S RD - Up] [5 S/N/S RD - Up]");
        reports.push_back(L"5|5 1 SP 122/123/134 F [1 P/S/S RD - Down] [2 S/P/P RD - Up] [3 I/S/S RD - Up] [4 S/S/S RD - Up] [5 S/S/S RD - Up]");
        reports.push_back(L"3|5 1 SP 000/000/122                                         [3 N/N/I RD - Up]                                    ");
        TestRebuild(
            L"4 reporting after GetLSN, 5 reporting after Deactivate and GetLSN, and 3 reporting before GetLSN",
            L"5 1 SP 122/175 [1 P/S RD - Down] [2 S/S RD - Down] [4 S/P RD - Up] [5 S/S RD - Up] [3 I/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 000/000/123 F [1 N/N/S SB - Up] [2 N/N/P SB - Up]");
        reports.push_back(L"2|3 1 SP 122/000/123 T [1 S/N/S SB - Up] [2 S/N/P IB - Up] [3 P/N/I RD - Down]");
        TestRebuild(
            L"IB primary and SB secondary replicas",
            L"3 1 SP 122/123 [1 S/S SB - Up] [2 S/P IB - Up] [3 P/I RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|5 1 SP 111/000/122 F [1 S/N/S RD - Up] [2 S/N/S RD - Up] [3 P/N/P RD - Up  ] [4 S/N/S RD - Up  ]");
        reports.push_back(L"2|5 1 SP 111/000/122 T [1 S/N/S RD - Up] [2 S/N/P RD - Up] [3 P/N/S DD - Down] [4 S/N/I DD - Down]");
        TestRebuild(
            L"Replica DD during reconfiguration",
            L"5 1 SP 111/122 [1 S/S RD - Up] [2 S/P RD - Up] [3 P/S DD - Down] [4 S/I DD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up]");
        reports.push_back(L"2|3 1 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up] [3 N/P RD - Up]");
        reports.push_back(L"3|3 1 SP 111/122 T [1 P/S RD - Up] [2 S/S RD - Up] [3 S/P RD - Up]");
        TestRebuild(
            L"Old primary restarts when reconfiguration to elect new primary is complete on secondary replicas",
            L"3 1 SP 111/122 [1 P/S RD - Up] [2 S/S RD - Up] [3 S/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:1|3 3 SP 111/112/123 F [1 P/P/S IB - Up] [2:1:1 S/S/S RD - Down] [3:1:1 S/S/S RD - Down]");
        reports.push_back(L"2:2|3 3 SP 000/000/123 T [1 N/N/S RD - Up] [2:2:2 N/N/P RD - Up  ] [3:2:2 N/N/S RD - Up  ]");
        TestRebuild(
            L"A restarted replica reporting after processing CreateReplica message",
            L"3 3 SP 000/123 [1 N/S RD - Up] [2:2:2 N/P RD - Up] [3:2:2 N/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/333 F [1 N/I IB - Up]");
        reports.push_back(L"2|3 2 SP 000/333 F                 [2 N/S RD - Up] [3 N/P RD - Up]");
        TestRebuild(
            L"Reports from idle and secondary replicas - I",
            L"3 2 SP 333/374 [1 I/I IB - Up] [2 S/P RD - Up] [3 P/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 2 SP 000/333 F [2 N/S RD - Up] [3 N/P RD - Up]");
        reports.push_back(L"1|3 2 SP 000/333 F                                 [1 N/I IB - Up]");
        TestRebuild(
            L"Reports from idle and secondary replicas - II",
            L"3 2 SP 333/374 [2 S/P RD - Up] [3 P/S RD - Down] [1 I/I IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/333 F [1 N/I IB - Up]");
        reports.push_back(L"2|3 2 SP 000/333 T                 [2 N/P RD - Up] [3 N/S RD - Up]");
        TestRebuild(
            L"Reports from idle and primary replicas, primary does not know about the idle replica - I",
            L"3 2 SP 000/333 [1 N/I IB - Up] [2 N/P RD - Up] [3 N/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"2|3 2 SP 000/333 T [2 N/P RD - Up] [3 N/S RD - Up]");
        reports.push_back(L"1|3 2 SP 000/333 F                                 [1 N/I IB - Up]");
        TestRebuild(
            L"Reports from idle and primary replicas, primary does not know about the idle replica - II",
            L"3 2 SP 000/333 [2 N/P RD - Up] [3 N/S RD - Down] [1 N/I IB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 1 SP 111/112/112 T [1 P/S/S RD - Up] [2 S/P/P RD - Up] [3 S/S/S RD - Up]");
        reports.push_back(L"2|3 1 SP 111/112/112 T [1 P/S/S RD - Up] [2 S/P/P RD - Up] [3 S/S/S RD - Up]");
        TestRebuild(
            L"Swap primary - I",
            L"3 1 SPW 111/112 [1 P/S RD - Up] [2 S/P RD - Up] [3 S/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|7 1 SP 111/123/123 T [1 S/P/P RD - Up] [2:2:2 P/S/S RD - Up] [3 S/S/S RD - Up] [4 I/S/S RD - Up]");
        reports.push_back(L"3|7 1 SP 000/000/123 F [1 N/N/P RD - Up] [2:2:2 N/N/S RD - Up] [3 N/N/S RD - Up] [4 N/N/S RD - Up]");
        reports.push_back(L"4|7 1 SP 112/000/123 F [1 S/N/S RD - Up] [2:1:1 P/N/P RD - Up] [3 S/N/S RD - Up] [4 S/N/S IB - Up]");
        TestRebuild(
            L"Swap primary - II",
            L"7 1 SP 111/123 [1 S/P RD - Up] [2:2:2 P/S RD - Down] [3 S/S RD - Up] [4 I/S RD - Up]",
            reports);
    }

    BOOST_AUTO_TEST_CASE(TestRebuildReplicaDownOrDropped)
    {
        vector<wstring> reports;
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2   N/P RD - Down] [2     N/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 F [1     N/P RD - Up  ] [2:2:2 N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 T [1:2:2 N/P RD - Up  ] [2     N/S RD - Up]");
        TestRebuild(
            L"Down replica - Scenario A",
            L"3 2 SP 000/122 [1:2:2 N/P RD - Up] [2:2:2 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2   N/P RD - Down] [2     N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 T [1:2:2 N/P RD - Up  ] [2     N/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 F [1     N/P RD - Up  ] [2:2:2 N/S RD - Up]");
        TestRebuild(
            L"Down replica - Scenario B",
            L"3 2 SP 000/122 [1:2:2 N/P RD - Up] [2:2:2 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"2:2|3 2 SP 000/122 F [1     N/P RD - Up  ] [2:2:2 N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2   N/P RD - Down] [2     N/S RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 T [1:2:2 N/P RD - Up  ] [2     N/S RD - Up]");
        TestRebuild(
            L"Down replica - Scenario C",
            L"3 2 SP 000/122 [1:2:2 N/P RD - Up] [2:2:2 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2   N/S RD - Down] [2     N/P RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 T [1     N/S RD - Down] [2:2:2 N/P RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2:2 N/S RD - Up  ] [2     N/P RD - Up]");
        TestRebuild(
            L"Down replica - Scenario D",
            L"3 2 SP 000/122 [1:2:2 N/S RD - Up] [2:2:2 N/P RD - Up]",
            reports);
        
        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2   N/S RD - Down] [2     N/P RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 T [1:2   N/S RD - Down] [2:2:2 N/P RD - Up]");
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2:2 N/S RD - Up  ] [2     N/P RD - Up]");
        TestRebuild(
            L"Down replica - Scenario E",
            L"3 2 SP 000/122 [1:2:2 N/S RD - Up] [2:2:2 N/P RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1  |3 2 SP 000/122 T [1 N/P RD - Up] [2 N/S RD - Up] [3   N/I RD - Down]");
        reports.push_back(L"2  |3 2 SP 000/122 F [1 N/P RD - Up] [2 N/S RD - Up]                    ");
        reports.push_back(L"3:2|3 2 SP 000/122 F [1 N/P RD - Up] [2 N/S RD - Up] [3:2 N/S RD - Down]");
        TestRebuild(
            L"Down replica - Scenario F",
            L"3 2 SP 000/122 [1 N/P RD - Up] [2 N/S RD - Up] [3:2 N/I RD N Down]",
            reports);
        
        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 T [1:2:2 N/P RD - Up] [2   N/S RD - Up  ] [3     N/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 F [1     N/P RD - Up] [2:2 N/S DD - Down] [3     N/S RD - Up]");
        reports.push_back(L"3:2|3 2 SP 000/122 F [1     N/P RD - Up] [2   N/S RD - Up  ] [3:2:2 N/S RD - Up]");
        TestRebuild(
            L"Secondary replica is Dropped",
            L"3 2 SP 000/122 [1:2:2 N/P RD - Up] [2:2 N/S DD - Down] [3:2:2 N/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1:2|3 2 SP 000/122 F [1:2 N/P DD - Down] [2     N/S RD - Up] [3     N/S RD - Up]");
        reports.push_back(L"2:2|3 2 SP 000/122 F [1   N/P RD - Up  ] [2:2:2 N/S RD - Up] [3     N/S RD - Up]");
        reports.push_back(L"3:2|3 2 SP 000/122 F [1   N/P RD - Up  ] [2     N/S RD - Up] [3:2:2 N/S RD - Up]");
        TestRebuild(
            L"Primary replica is Dropped",
            L"3 2 SP 122/163 [1:2 P/N DD - Down] [2:2:2 S/P RD - Up] [3:2:2 S/S RD - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [1 N/P RD - Down] [2 N/S RD - Down]");
        reports.push_back(L"2|3 2 SP 000/122 F [1 N/P RD - Down] [2 N/S RD - Down]");
        TestRebuild(
            L"All replicas are down",
            L"3 2 SP 000/163 [1 N/P RD - Down] [2 N/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 122/134 F [1 P/S RD - Down] [2 S/P RD - Up] [3 S/S DD - Down]");
        reports.push_back(L"2|3 2 SP 000/122 F [1 N/P RD - Up  ] [2 N/S RD - Up] [3 N/S RD - Up  ]");
        TestRebuild(
            L"Old primary is down and a secondary replica is Dropped",
            L"3 2 SP 122/175 [1 P/S RD - Down] [2 S/P RD - Up] [3 S/S DD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 122/134 T [1 P/S RD - Up] [2 S/P RD - Up] [3 S/S RD - Up]");
        reports.push_back(L"2");
        reports.push_back(L"3");
        TestRebuild(
            L"Majority of the replicas are Dropped",
            L"3 2 SPW 122/134 [1 P/S RD - Up] [2 S/P DD Z Down] [3 S/S DD Z Down]",
            reports);

        reports.clear();
        reports.push_back(L"1|3 2 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up]                   [4 N/P RD - Up]");
        reports.push_back(L"2|3 2 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up]                   [4 N/P RD - Up]");
        reports.push_back(L"3|3 2 SP 000/122 F                                 [3 N/I SB - Down]                ");
        reports.push_back(L"4|3 2 SP 000/122 T [1 N/S RD - Up] [2 N/S RD - Up] [3 N/I SB - Down] [4 N/P RD - Up]");
        TestRebuild(
            L"A down Idle replica reports that needs to be removed from the view of the Primary replica",
            L"3 2 SP 000/122 [1 N/S RD - Up] [2 N/S RD - Up] [4 N/P RD - Up] [3 N/I SB N Down]",
            reports);

        reports.clear();
        reports.push_back(L"1:3|4 3 SP 000/124 F [1:3:2 N/I SB - Down]");
        reports.push_back(L"2 |4 3 SP 000/124 F [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"3 |4 3 SP 000/124 T [1:2:2 N/I IB - Up ] [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"4 |4 3 SP 000/124 F [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up]");
        reports.push_back(L"5 |4 3 SP 000/122 F [1:1:1 N/S RD - Up ] [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up] [5 N/S SB - Up]");
        TestRebuild(
            L"Down idle replica should be makred as PendingRemove",
            L"4 3 SP 000/124 [1:3:2 N/I IB N Down] [2 N/S RD - Up] [3 N/P RD - Up] [4 N/S RD - Up] [5 N/I SB - Up]",
            reports);

        reports.clear();
        reports.push_back(L"1  |4 2 SP 122/133 F [1 S/S RD - Up] [2 S/P RD - Up] [3     P/I RD - Down] [4 S/S RD - Up]");
        reports.push_back(L"3:2|4 2 SP 000/122 F [1 N/S RD - Up] [2 N/S RD - Up] [3:2:2 N/P SB - Up  ] [4 N/S RD - Up]");
        TestRebuild(
            L"Old primary is up while going through P/I reconfiguration",
            L"4 2 SP 122/174 [1 S/S RD - Up] [2 S/P RD - Down] [3:2:2 P/I SB - Up] [4 S/S RD - Down]",
            reports);

        reports.clear();
        reports.push_back(L"1  |5 5 SP 144/145 F [1 S/S RD - Up] [2 P/P RD - Up] [3     S/I RD - Down] [4 S/S RD - Down] [5 S/S RD - Down] [6 I/S RD - Up]");
        reports.push_back(L"3:2|5 5 SP 000/133 F [1 N/S RD - Up] [2 N/S RD - Up] [3:2:2 N/P SB - Up  ] [4 N/S RD - Up  ] [5 N/S RD - Up]");
        TestRebuild(
            L"Old primary is up while going through S/I reconfiguration",
            L"5 5 SP 144/186 [1 S/S RD - Up] [2 P/P RD - Down] [3:2:2 S/I SB - Up] [4 S/S RD - Down] [5 S/S RD - Down] [6 I/S RD - Down]",
            reports);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool RebuildTest::ClassSetup()
    {
        FailoverConfig::Test_Reset();
        FailoverConfig & config = FailoverConfig::GetConfig();
        config.IsTestMode = true;
        config.DummyPLBEnabled = true;
        config.PeriodicStateScanInterval = TimeSpan::MaxValue;

        auto & config_plb = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
        config_plb.IsTestMode = true;
        config_plb.DummyPLBEnabled=true;

        config_plb.PLBRefreshInterval = (TimeSpan::MaxValue);
        config_plb.ProcessPendingUpdatesInterval = (TimeSpan::MaxValue);

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        return true;
    }

    bool RebuildTest::ClassCleanup()
    {
        fm_->Close(true /* isStoreCloseNeeded */);
        FailoverConfig::Test_Reset();
        return true;
    }

}
