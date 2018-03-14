// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReplicaUp;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestFabricCodeVersionClassifier
{
protected:
    void TestDeactivationInfo(wstring const & versionStr, bool expected);
};

void TestFabricCodeVersionClassifier::TestDeactivationInfo(wstring const & versionStr, bool expected)
{
	FabricCodeVersionClassifier classifier;
	auto version = Reader::ReadHelper<FabricCodeVersion>(versionStr);
	bool actual = classifier.Classify(version).IsDeactivationInfoSupported;
	Verify::AreEqualLogOnError(expected, actual, wformatString("CodeVersion {0}", version));
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFabricCodeVersionClassifierSuite,TestFabricCodeVersionClassifier)

BOOST_AUTO_TEST_CASE(DeactivationInfoTest)
{
    // Anything >= 4.0.1402 is supported
    TestDeactivationInfo(L"4.0.1403", true);
    TestDeactivationInfo(L"4.0.1402", true);
    TestDeactivationInfo(L"4.1.1", true);
    TestDeactivationInfo(L"5.0.1", true);

    // Anything below 4.0.1401 is not supported
    TestDeactivationInfo(L"4.0.1401", false);
    TestDeactivationInfo(L"4.0.116", false);
    TestDeactivationInfo(L"3.2.117", false);
    TestDeactivationInfo(L"3.1.1", false);
    TestDeactivationInfo(L"2.0.1", false);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
