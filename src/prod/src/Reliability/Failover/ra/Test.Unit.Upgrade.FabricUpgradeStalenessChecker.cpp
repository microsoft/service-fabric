// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace FabricUpgradeStalenessCheckResult
            {
                void WriteToTextWriter(TextWriter& w, Enum const & e)
                {
                        switch (e)
                        {
                        case FabricUpgradeStalenessCheckResult::Assert:
                            w << "Assert";
                            break;
                
                        case FabricUpgradeStalenessCheckResult::UpgradeNotRequired:
                            w << "UpgradeNotRequired";
                            break;
                
                        case FabricUpgradeStalenessCheckResult::UpgradeRequired:
                            w << "UpgradeRequired";
                            break;
                
                        default:
                            Assert::CodingError("unknown {0}", static_cast<int>(e));
                        }
                }
            }
        }
    }
}

namespace
{
    
    class Version
    {
    public:
        enum Enum
        {
            VnMinusOne = 0,
            Vn = 1,
            VnPlusOne = 2,
        };

        static FabricVersionInstance GetVersion(Version::Enum e, uint64 instance)
        {
            FabricVersion fabricVersion = GetVersion(e);

            return FabricVersionInstance(fabricVersion, instance);
        }

        static FabricVersionInstance GetVersionWithBaselineUpgradeNotPerformed(Version::Enum e)
        {
            return GetVersion(e, 0);
        }

    private:
        static FabricVersion GetVersion(Version::Enum e)
        {
            uint major = GetMajorVersion(e);
            FabricCodeVersion code(major, 0, 0, 0);

            FabricConfigVersion configVersion;
            auto error = configVersion.FromString(L"A");
            if (!error.IsSuccess()) { Assert::CodingError("Invalid FabricConfigVersion"); }

            return FabricVersion(code, configVersion);
        }

        static uint GetMajorVersion(Version::Enum e)
        {
            switch (e)
            {
            case Version::Vn:
                return 3;
            case Version::VnPlusOne:
                return 4;
            case Version::VnMinusOne:
                return 2;
            default:
                Assert::CodingError("Unknown {0}", static_cast<int>(e));
            }
        }
    };
}

class TestRunner
{
public:
    typedef function<FabricUpgradeStalenessCheckResult::Enum(FabricVersionInstance const &, FabricVersionInstance const&)> CheckFunctionPtr;

    TestRunner(CheckFunctionPtr func)
        : func_(func)
    {
    }

    void ExecuteAndVerify(
        FabricVersionInstance const & node,
        FabricVersionInstance const & incoming,
        FabricUpgradeStalenessCheckResult::Enum expected);

    void ExecuteAndVerifyWithBaselineAndWithoutBaseline(
        FabricVersionInstance const & incoming,
        FabricUpgradeStalenessCheckResult::Enum expected);

    void ExecuteAndVerifyAllCodeVersionsForIncoming(
        wstring const & log,
        uint64 nodeInstance,
        FabricVersionInstance const & incoming,
        FabricUpgradeStalenessCheckResult::Enum expected);

    void ExecuteAndVerifyAllCodeVersionsForIncoming(
        wstring const & log,
        uint64 nodeInstance,
        FabricVersionInstance const & incoming,
        FabricUpgradeStalenessCheckResult::Enum expectedVnMinusOne,
        FabricUpgradeStalenessCheckResult::Enum expectedVn,
        FabricUpgradeStalenessCheckResult::Enum expectedVnPlusOne);

private:
    CheckFunctionPtr func_;
};

void TestRunner::ExecuteAndVerify(
    FabricVersionInstance const & node,
    FabricVersionInstance const & incoming,
    FabricUpgradeStalenessCheckResult::Enum expected)
{    
    auto actual = func_(node, incoming);
    Verify::AreEqual(expected, actual, wformatString("Comparing node {0}. Incoming {1}. Expected = {2}", node, incoming, expected));
}

void TestRunner::ExecuteAndVerifyWithBaselineAndWithoutBaseline(
    FabricVersionInstance const & incoming,
    FabricUpgradeStalenessCheckResult::Enum expected)
{
    ExecuteAndVerify(Version::GetVersion(Version::Vn, 1), incoming, expected);
    ExecuteAndVerify(Version::GetVersionWithBaselineUpgradeNotPerformed(Version::Vn), incoming, expected);
}

void TestRunner::ExecuteAndVerifyAllCodeVersionsForIncoming(
    wstring const & log,
    uint64 nodeInstance,
    FabricVersionInstance const & incoming,
    FabricUpgradeStalenessCheckResult::Enum expected)
{
    ExecuteAndVerifyAllCodeVersionsForIncoming(
        log,
        nodeInstance,
        incoming,
        expected,
        expected,
        expected);
}

void TestRunner::ExecuteAndVerifyAllCodeVersionsForIncoming(
    wstring const & log,
    uint64 nodeInstance,
    FabricVersionInstance const & incoming,
    FabricUpgradeStalenessCheckResult::Enum expectedVnMinusOne,
    FabricUpgradeStalenessCheckResult::Enum expectedVn,
    FabricUpgradeStalenessCheckResult::Enum expectedVnPlusOne)
{
    TestLog::WriteInfo(log);

    ExecuteAndVerify(Version::GetVersion(Version::VnMinusOne, nodeInstance), incoming, expectedVnMinusOne);
    ExecuteAndVerify(Version::GetVersion(Version::Vn, nodeInstance), incoming, expectedVn);
    ExecuteAndVerify(Version::GetVersion(Version::VnPlusOne, nodeInstance), incoming, expectedVnPlusOne);
}

class FabricUpgradeStalenessCheckerTest
{
    // NodeUp tests
protected:
    static TestRunner GetNodeUpTestRunner()
    {
        return TestRunner([](FabricVersionInstance const & node, FabricVersionInstance const & incoming)
        {
            FabricUpgradeStalenessChecker checker;
            return checker.CheckFabricUpgradeAtNodeUp(node, incoming);
        });
    }

    static TestRunner GetUpgradeTestRunner()
    {
        return TestRunner([](FabricVersionInstance const & node, FabricVersionInstance const & incoming)
        {
            FabricUpgradeStalenessChecker checker;
            return checker.CheckFabricUpgradeAtUpgradeMessage(node, incoming);
        });
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(FabricUpgradeStalenessCheckerTestSuite,FabricUpgradeStalenessCheckerTest)

// If incoming is default then no-op
// If incoming has instance then matching is done on instance
// If incoming does not have instance then matching is done on version
BOOST_AUTO_TEST_CASE(NodeUp_UpgradeNotRequired_IncomingIsDefault)
{
    GetNodeUpTestRunner().ExecuteAndVerifyWithBaselineAndWithoutBaseline(
        FabricVersionInstance::Default,
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired);
}

BOOST_AUTO_TEST_CASE(NodeUp_UpgradeV1FMWithNoInstance)
{
    GetNodeUpTestRunner().ExecuteAndVerifyWithBaselineAndWithoutBaseline(
        FabricVersionInstance(),
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired);
}

BOOST_AUTO_TEST_CASE(NodeUp_IncomingHasInstanceTest)
{
    // In all these tests incoming upgrade has an instance and is for vn
    auto incoming = Version::GetVersion(Version::Vn, 2);
    auto runner = GetNodeUpTestRunner();
# pragma region Local Node has instance and incoming has instance -> Look at the instance ids

    // Test1
    // Local node has higher instance is not possible and should be an assert
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeHigherInstance",
        incoming.InstanceId + 1,
        incoming,
        FabricUpgradeStalenessCheckResult::Assert);

    // Test2
    // Local node has equal instance id then it does not matter what the code version is
    // the node should not upgrade
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeEqualInstance",
        incoming.InstanceId,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired);

    // Test3
    // Local node has a lower instnace id then it does not matter what the code version is
    // the node should upgrade
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeLowerInstance",
        incoming.InstanceId - 1,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired);
#pragma endregion

    // Test4
    // Local node has no instance id - node without baseline restarts
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeHasNoInstanceId",
        0,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired);

    // Test5
    // Local node has no instance id and incoming upgrade has no instance id
    // Upgrade should happen if the code versions do not match
    incoming = Version::GetVersionWithBaselineUpgradeNotPerformed(Version::Vn);
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalHasNoInstance+IncomingHasNoInstance",
        0,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired, // Node at vn-1 should upgrade
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired, // node at vn should not upgrade
        FabricUpgradeStalenessCheckResult::UpgradeRequired); // node at vn + 1 should downgrade

    // Test6
    // Local node has instance and incoming node has no instance id
    // Instance can never go backwards
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalHasInstance+IncomingHasNoInstance",
        1,
        incoming,
        FabricUpgradeStalenessCheckResult::Assert);
}

BOOST_AUTO_TEST_CASE(Upgrade_UpgradeNotRequired_IncomingIsDefault)
{
    GetUpgradeTestRunner().ExecuteAndVerifyWithBaselineAndWithoutBaseline(
        FabricVersionInstance::Default,
        FabricUpgradeStalenessCheckResult::Assert);
}

BOOST_AUTO_TEST_CASE(Upgrade_UpgradeV1FMWithNoInstance)
{
    GetUpgradeTestRunner().ExecuteAndVerifyWithBaselineAndWithoutBaseline(
        FabricVersionInstance(),
        FabricUpgradeStalenessCheckResult::Assert);
}

BOOST_AUTO_TEST_CASE(Upgrade_IncomingHasInstanceTest)
{
    auto incoming = Version::GetVersion(Version::Vn, 2);
    auto runner = GetUpgradeTestRunner();

# pragma region Local Node has instance and incoming has instance -> Look at the instance ids

    // Test1
    // Local node has higher instance means message is stale
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeHigherInstance",
        incoming.InstanceId + 1,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired);

    // Test2
    // Local node has equal instance id then it does not matter what the code version is
    // the node should not upgrade
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeEqualInstance",
        incoming.InstanceId,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired);

    // Test3
    // Local node has a lower instnace id then it does not matter what the code version is
    // the node should upgrade
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeLowerInstance",
        incoming.InstanceId - 1,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired);
#pragma endregion

    // Test4
    // Local node has no instance id 
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalNodeHasNoInstanceId",
        0,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired);

    // Test5
    // Local node has no instance id and incoming upgrade has no instance id
    // Upgrade should happen if the code versions do not match
    incoming = Version::GetVersionWithBaselineUpgradeNotPerformed(Version::Vn);
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalHasNoInstance+IncomingHasNoInstance",
        0,
        incoming,
        FabricUpgradeStalenessCheckResult::UpgradeRequired, // Node at vn-1 should upgrade
        FabricUpgradeStalenessCheckResult::UpgradeNotRequired, // node at vn should not upgrade
        FabricUpgradeStalenessCheckResult::UpgradeRequired); // node at vn + 1 should downgrade

    // Test6
    // Local node has instance and incoming node has no instance id
    // Instance can never go backwards
    runner.ExecuteAndVerifyAllCodeVersionsForIncoming(
        L"LocalHasInstance+IncomingHasNoInstance",
        1,
        incoming,
        FabricUpgradeStalenessCheckResult::Assert);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
