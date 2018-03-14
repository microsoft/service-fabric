// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

class TestConfigurationUtility;

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    const int LocalReplicaId = 1;
}

class TestConfigurationUtility
{
public:
    void RunTest(
        std::wstring const & incoming,
        std::wstring const & initialStore,
        std::wstring const & expectedStore);

protected:
    TestConfigurationUtility()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestConfigurationUtility()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void RunStalenessTest(
        std::wstring const & incoming,
        std::wstring const & store);

    void RunNotModifiedTest(
        std::wstring const & incoming,
        std::wstring const & store);

    void RunModifiedTest(
        std::wstring const & incoming,
        std::wstring const & initialStore,
        std::wstring const & expectedStore);

    void VerifyStore(
        std::wstring const & msg,
        std::wstring const & expectedStr,
        FailoverUnitProxy::ConfigurationReplicaStore const & actual);
    
    std::wstring ToString(
        FailoverUnitProxy::ConfigurationReplicaStore const & actual);

    std::wstring ToString(
        vector<FABRIC_REPLICA_INFORMATION> const & actual,
        bool verifyLSN);

    void VerifyCount(
        std::wstring const & incoming,
        int ccCountExpected,
        int ccNonDroppedCountExpected,
        int pcCountExpected,
        int pcNonDroppedCountExpected);

    void VerifyReplicatorConfiguration(
        bool isPrimaryChangeBetweenPCAndCC,
        bool runCatchup,
        bool verifyLSN,
        std::wstring const & store,
        std::wstring const & ccExpected,
        std::wstring const & pcExpected);

    void VerifyConfiguration(
        bool verifyLSN,
        wstring const & tag,
        wstring const & expected,
        vector<FABRIC_REPLICA_INFORMATION> const & actual);

    void VerifyConfiguration(
        wstring const & tag,
        wstring const & expected,
        vector<FABRIC_REPLICA_INFORMATION> const & actual);

    FailoverUnitProxy::ConfigurationUtility utility_;

private:
    vector<ReplicaDescription> ReadReplicas(std::wstring const & str) const;

    FailoverUnitProxy::ConfigurationReplicaStore ReadStore(std::wstring const & str) const;
};

bool TestConfigurationUtility::TestSetup()
{
    return true;
}

bool TestConfigurationUtility::TestCleanup()
{
    return true;
}

vector<ReplicaDescription> TestConfigurationUtility::ReadReplicas(wstring const& str) const
{
    return Reader::ReadVector<ReplicaDescription>(L'[', L']', str);
}

FailoverUnitProxy::ConfigurationReplicaStore TestConfigurationUtility::ReadStore(std::wstring const & str) const
{
    return Reader::ReadHelper<FailoverUnitProxy::ConfigurationReplicaStore>(str);
}    

void TestConfigurationUtility::RunStalenessTest(
    std::wstring const & incoming,
    std::wstring const & store)
{
    RunTest(incoming, store, wstring());
}

void TestConfigurationUtility::RunNotModifiedTest(
    std::wstring const & incoming,
    std::wstring const & store)
{
    RunTest(incoming, store, store);
}

void TestConfigurationUtility::RunModifiedTest(
    std::wstring const & incoming,
    std::wstring const & initialStore,
    std::wstring const & expectedStore)
{
    Verify::IsTrueLogOnError(expectedStore != initialStore, wformatString("Running modified test but initial == final\r\n{0}\r\n{1}", initialStore, expectedStore));
    RunTest(incoming, initialStore, expectedStore);
}

void TestConfigurationUtility::VerifyStore(
    std::wstring const & msg,
    std::wstring const & expectedStr,
    FailoverUnitProxy::ConfigurationReplicaStore const & actualStore)
{
    auto expected = ToString(ReadStore(expectedStr));
    auto actual = ToString(actualStore);

    Verify::AreEqualLogOnError(expected, actual, msg);
}

std::wstring TestConfigurationUtility::ToString(
    FailoverUnitProxy::ConfigurationReplicaStore const & store)
{
    if (store.empty())
    {
        return L"";
    }

    wstring rv;
    for (auto const & it : store)
    {
        rv += wformatString(it.second);
        rv += L"\r\n";
    }

    rv.resize(rv.length() - 2);

    return rv;
}

void TestConfigurationUtility::RunTest(
    std::wstring const & incoming,
    std::wstring const & initialStore,
    std::wstring const & expectedStore)
{
    auto isStaleExpected = expectedStore.empty();

    auto replicas = ReadReplicas(incoming);
    auto store = ReadStore(initialStore);
    auto isStale = utility_.IsConfigurationMessageBodyStale(LocalReplicaId, replicas, store);

    auto suffix = wformatString("{0}\r\nIncoming: {1}\r\nInitial: {2}\r\nExpected: {3}", L"", incoming, initialStore, expectedStore);
    Verify::AreEqualLogOnError(isStaleExpected, isStale, L"IsStale " + suffix);
    if (isStale)
    {
        return;
    }

    auto modified = utility_.CheckConfigurationMessageBodyForUpdates(LocalReplicaId, replicas, false, store);
    bool isModifiedExpected = expectedStore != initialStore;
    Verify::AreEqualLogOnError(isModifiedExpected, modified, L"IsModified (shouldApply=false) " + suffix);
    VerifyStore(L"Store (shouldApply=false) " + suffix, initialStore, store);

    modified = utility_.CheckConfigurationMessageBodyForUpdates(LocalReplicaId, replicas, true, store);
    Verify::AreEqualLogOnError(isModifiedExpected, modified, L"IsModified (shouldApply=true) " + suffix);
    VerifyStore(L"Store (shouldApply=true) " + suffix, expectedStore, store);
}

void TestConfigurationUtility::VerifyCount(
    std::wstring const & incoming,
    int ccCountExpected,
    int ccNonDroppedCountExpected,
    int pcCountExpected,
    int pcNonDroppedCountExpected)
{
    int ccCount = 1923;
    int ccNonDroppedCount = 19299;
    int pcCount = 39919;
    int pcNonDroppedCount = 1992999;

    auto replicas = ReadReplicas(incoming);

    utility_.GetReplicaCountForPCAndCC(replicas, ccCount, ccNonDroppedCount, pcCount, pcNonDroppedCount);

    function<wstring (wstring)> getMsgFunc = [&](wstring arg)
    {
        return wformatString(
            "{0}. Incoming: {1}. Expected: {2}, {3}, {4}, {5}. Actual: {6} {7} {8} {9}",
            arg, incoming,
            ccCountExpected, ccNonDroppedCountExpected, pcCountExpected, pcNonDroppedCountExpected,
            ccCount, ccNonDroppedCount, pcCount, pcNonDroppedCount);
    };

    Verify::AreEqualLogOnError(ccCountExpected, ccCount, getMsgFunc(L"ccCount"));
    Verify::AreEqualLogOnError(ccNonDroppedCountExpected, ccNonDroppedCount, getMsgFunc(L"ccNonDroppedCount"));
    Verify::AreEqualLogOnError(pcCountExpected, pcCount, getMsgFunc(L"pcCount"));
    Verify::AreEqualLogOnError(pcNonDroppedCountExpected, pcNonDroppedCount, getMsgFunc(L"pcNonDroppedCount"));
}

void TestConfigurationUtility::VerifyReplicatorConfiguration(
    bool isPrimaryChangeBetweenPCAndCC,
    bool runCatchup,
    bool verifyLSN,
    std::wstring const & store,
    std::wstring const & ccExpectedStr,
    std::wstring const & pcExpectedStr)
{
    ScopedHeap heap;

    auto configurationReplicas = ReadStore(store);

    vector<FABRIC_REPLICA_INFORMATION> pc;
    vector<FABRIC_REPLICA_INFORMATION> cc;
    vector<ReplicaDescription const *> tombstoneReplicas;
    
    utility_.CreateReplicatorConfigurationList(configurationReplicas, isPrimaryChangeBetweenPCAndCC, runCatchup, heap, cc, pc, tombstoneReplicas);

    VerifyConfiguration(verifyLSN, L"PC", pcExpectedStr, pc);
    VerifyConfiguration(verifyLSN, L"CC", ccExpectedStr, cc);
}

wstring TestConfigurationUtility::ToString(vector<FABRIC_REPLICA_INFORMATION> const & actual, bool includeLSN)
{
    if (actual.empty())
    {
        return L"";
    }

    wstring rv;
    for (auto const & it : actual)
    {
        ASSERT_IF(it.Status == FABRIC_REPLICA_STATUS_INVALID, "Status cannot be invalid");

        if (!includeLSN)
        {
            ASSERT_IF(it.CatchUpCapability != -1, "CatchupCapability not set to default");
            ASSERT_IF(it.CurrentProgress != -1, "CurrentProgress not set to default");
        }

        wstring role;
        switch (it.Role)
        {
        case ::FABRIC_REPLICA_ROLE_UNKNOWN:
            role = L"U";
            break;
        case ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            role = L"S";
            break;
        case ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            role = L"I";
            break;
        case ::FABRIC_REPLICA_ROLE_NONE:
            role = L"N";
            break;
        case ::FABRIC_REPLICA_ROLE_PRIMARY:
            role = L"P";
            break;
        default:
            Assert::CodingError("Unknown role");
        }

        ASSERT_IF(it.Reserved == NULL, "Must create the reserved object");
        auto casted = reinterpret_cast<FABRIC_REPLICA_INFORMATION_EX1*>(it.Reserved);
        ASSERT_IF(casted->Reserved != NULL, "Reserved must be null");

        if (!includeLSN)
        {
            rv += wformatString("[{0} {1}{2}] ", role, it.Id, casted->MustCatchup ? L" true" : L"");
        }
        else
        {
            rv += wformatString("[{0} {1}{2} {3} {4}] ", role, it.Id, casted->MustCatchup ? L" true" : L"", it.CatchUpCapability, it.CurrentProgress);
        }
    }

    rv.resize(rv.length() - 1);
    return rv;
}

void TestConfigurationUtility::VerifyConfiguration(
    bool verifyLSN,
    wstring const & tag,
    wstring const & expected,
    vector<FABRIC_REPLICA_INFORMATION> const & actual)
{
    auto actualStr = ToString(actual, verifyLSN);
    Verify::AreEqualLogOnError(expected, actualStr, tag);
}

void TestConfigurationUtility::VerifyConfiguration(
    wstring const & tag,
    wstring const & expected,
    vector<FABRIC_REPLICA_INFORMATION> const & actual)
{
    VerifyConfiguration(false, tag, expected, actual);
}

namespace Result
{
    enum Enum
    {
        Stale,
        Modified,
        NotModified,
        Assert
    };
}

class TestDescription
{
public:
    static TestDescription Create(wstring const & incomingRemote, wstring const & store, Result::Enum result)
    {
        switch (result)
        {
        case Result::Stale:
            return CreateStale(incomingRemote, store);
        case Result::NotModified:
            return CreateNotModified(incomingRemote, store);
        case Result::Modified:
            return CreateModified(incomingRemote, store);
        case Result::Assert:
            return TestDescription();
        default:
            Assert::CodingError("unknown");
        }
    }

    static TestDescription CreateStale(wstring const & incomingRemote, wstring const & store)
    {
        return TestDescription(incomingRemote, store, wstring());
    }

    static TestDescription CreateNotModified(wstring const & incomingRemote, wstring const & store)
    {
        return TestDescription(incomingRemote, store, store);
    }

    static TestDescription CreateModified(wstring const & incomingRemote, wstring const & store)
    {
        return TestDescription(incomingRemote, store, incomingRemote);
    }

    static TestDescription CreateModified(wstring const & incomingRemote, wstring const & store, wstring const & storeFinal)
    {
        return TestDescription(incomingRemote, store, storeFinal);
    }

    void ExecuteWithSecondaryToPrimary(TestConfigurationUtility & utility)
    {
        Execute(L"[S/P RD U 1:1] ", L"", utility);
    }

    void ExecuteWithIdleToPrimary(TestConfigurationUtility & utility)
    {
        Execute(L"[S/P RD U 1:1] ", L"", utility);
    }

    void ExecuteWithPrimaryToPrimary(TestConfigurationUtility & utility)
    {
        Execute(L"[P/P RD U 1:1] ", L"", utility);
    }

    void ExecuteWithDemoteToSecondary(TestConfigurationUtility & utility)
    {
        Execute(L"[P/S RD U 1:1] [S/P RD U 3:1] ", L"[S/P RD U 3:1] ", utility);
    }


private:
    TestDescription(
        wstring const & incomingRemote,
        wstring const & initial,
        wstring const & storeFinal) :
        incomingRemoteReplicas_(incomingRemote),
        initialConfiguration_(initial),
        finalRemoteReplicas_(storeFinal),
        isValid_(true)
    {
    }

    TestDescription() : isValid_(false)
    {
    }

    void Execute(wstring const & incomingPrefix, wstring const & storePrefix, TestConfigurationUtility & utility)
    {
        if (!isValid_)
        {
            TestLog::WriteInfo(wformatString("Skipping {0} {1} as invalid", incomingRemoteReplicas_, initialConfiguration_));
            return;
        }

        wstring incoming = incomingPrefix + incomingRemoteReplicas_;
        wstring store = storePrefix + initialConfiguration_;
        wstring final2 = finalRemoteReplicas_.empty() ? finalRemoteReplicas_ : storePrefix + finalRemoteReplicas_;
        utility.RunTest(incoming, store, final2);
    }

    bool isValid_;
    std::wstring incomingRemoteReplicas_;
    std::wstring initialConfiguration_;
    std::wstring finalRemoteReplicas_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestConfigurationUtilitySuite, TestConfigurationUtility)

BOOST_AUTO_TEST_CASE(SingleReplicaConfigurationUpdate)
{
    /*
Add all tests that are valid for all I/P, P/P, S/P, P/S cases
*/
    vector<TestDescription> allCases;

#pragma region Local Replica is i1

# pragma region Local is SB
    // Local is Up
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S SB U 2:1]", Result::NotModified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S SB U 2:1]", Result::Modified));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S SB U 2:1]", Result::Modified));

    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S SB U 2:1]", Result::Modified));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S SB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S SB U 2:1]", Result::Modified));

    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S SB D 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S SB D 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S SB D 2:1]", Result::Assert));
                                                                            
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S SB D 2:1]", Result::NotModified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S SB D 2:1]", Result::Modified));
                                                                            
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S SB D 2:1]", Result::Modified));
                                                                            
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S SB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S SB D 2:1]", Result::Modified));
# pragma endregion

#pragma region Local is IB
    // Local is U
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S IB U 2:1]", Result::Modified)); 
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S IB U 2:1]", Result::NotModified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S IB U 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S IB U 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S IB U 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S IB U 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S IB U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S IB U 2:1]", Result::Modified));
                                                                         
    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S IB D 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S IB D 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S IB D 2:1]", Result::Assert));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S IB D 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S IB D 2:1]", Result::NotModified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S IB D 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S IB D 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S IB D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S IB D 2:1]", Result::Modified));

#pragma endregion

# pragma region Local is RD
    // Local is U
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S RD U 2:1]", Result::Stale)); 
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S RD U 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S RD U 2:1]", Result::NotModified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S RD U 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S RD U 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S RD U 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S RD U 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S RD U 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S RD U 2:1]", Result::Modified));
                                                                         
    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S RD D 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S RD D 2:1]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S RD D 2:1]", Result::Stale));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S RD D 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S RD D 2:1]", Result::Assert));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S RD D 2:1]", Result::NotModified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S RD D 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:2]", L"[S/S RD D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:2]", L"[S/S RD D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:2]", L"[S/S RD D 2:1]", Result::Modified));
                                                                         
    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:2]", L"[S/S RD D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:2]", L"[S/S RD D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:2]", L"[S/S RD D 2:1]", Result::Modified));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:2]", L"[S/S RD D 2:1]", Result::Modified));

#pragma endregion

#pragma endregion   

#pragma region Local Replica is i2

# pragma region Local is SB
    // Local is Up
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S SB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S SB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S SB U 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S SB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S SB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S SB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S SB U 2:2]", Result::Stale));

    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S SB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S SB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S SB D 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S SB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S SB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S SB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S SB D 2:2]", Result::Stale));

# pragma endregion

#pragma region Local is IB
    // Local is U
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S IB U 2:2]", Result::Stale)); 
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S IB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S IB U 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S IB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S IB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S IB U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S IB U 2:2]", Result::Stale));

    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S IB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S IB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S IB D 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S IB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S IB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S IB D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S IB D 2:2]", Result::Stale));

#pragma endregion

# pragma region Local is RD
    // Local is U
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S RD U 2:2]", Result::Stale)); 
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S RD U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S RD U 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S RD U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S RD U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S RD U 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S RD U 2:2]", Result::Stale));

    // Local is D
    allCases.push_back(TestDescription::Create(L"[S/S SB U 2:1]", L"[S/S RD D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB U 2:1]", L"[S/S RD D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD U 2:1]", L"[S/S RD D 2:2]", Result::Stale));

    allCases.push_back(TestDescription::Create(L"[S/S SB D 2:1]", L"[S/S RD D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S IB D 2:1]", L"[S/S RD D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S RD D 2:1]", L"[S/S RD D 2:2]", Result::Stale));
    allCases.push_back(TestDescription::Create(L"[S/S DD D 2:1]", L"[S/S RD D 2:2]", Result::Stale));

#pragma endregion

#pragma endregion   

    for (auto & it : allCases)
    {
        it.ExecuteWithPrimaryToPrimary(*this);
        it.ExecuteWithSecondaryToPrimary(*this);
        it.ExecuteWithIdleToPrimary(*this);
        it.ExecuteWithDemoteToSecondary(*this);
    }
}

BOOST_AUTO_TEST_CASE(AddNewReplicas)
{
    RunTest(
        L"[P/P RD U 1:1] [I/S RD U 2:1]",
        L" ",
        L"[I/S RD U 2:1]");

    RunTest(
        L"[P/P RD U 1:1] [I/S RD U 2:1] [I/S RD U 3:1]",
        L" ",
        L"[I/S RD U 2:1] [I/S RD U 3:1]");
}

BOOST_AUTO_TEST_CASE(PrimaryToSecondaryInitialDemote)
{
    RunTest(
        L"[P/P RD U 1:1] [S/P RD U 2:1]",
        L"[S/S RD U 2:1]",
        L"[S/P RD U 2:1]");

    RunTest(
        L"[P/P RD U 1:1] [S/P RD U 2:1] [S/S RD D 3:1]",
        L"[S/S RD U 2:1] [S/S RD U 3:1]",
        L"[S/P RD U 2:1] [S/S RD D 3:1]");
}

BOOST_AUTO_TEST_CASE(S_To_N_Reconfiguration)
{
    RunTest(
        L"[P/P RD U 1:1] [S/N RD U 2:1]",
        L"[I/S RD U 2:1]",
        L"[S/N RD U 2:1]");
}

BOOST_AUTO_TEST_CASE(RemoveReplicas)
{
    RunTest(
        L"[P/P RD U 1:1]",
        L"[S/N RD U 2:1]",
        L" ");
}

BOOST_AUTO_TEST_CASE(PCAndCCCount)
{
    VerifyCount(L"[P/P RD U 1:1] [S/S RD U 1:1] [S/S RD U 2:2]", 3, 3, 3, 3);
    VerifyCount(L"[P/P RD U 1:1] [S/S RD D 1:1] [S/S RD U 2:2]", 3, 3, 3, 3);
    VerifyCount(L"[P/P RD U 1:1] [S/I RD D 1:1] [S/S RD U 2:2]", 2, 2, 3, 3);

    // Swap case
    VerifyCount(L"[P/S RD U 1:1] [S/S RD U 1:1] [S/P RD U 2:2]", 3, 3, 3, 3);

    // S/N
    VerifyCount(L"[S/P RD U 1:1] [S/N RD U 1:1] [S/N RD U 2:2]", 1, 1, 3, 3);

    VerifyCount(L"[S/P RD U 1:1] [S/I RD U 1:1] [S/N RD U 2:2]", 1, 1, 3, 3);

    // S/N & I/S
    VerifyCount(L"[S/P RD U 1:1] [S/I RD U 1:1] [S/N RD U 2:2] [I/S RD U 3:1]", 2, 2, 3, 3);

    // DD replica
    VerifyCount(L"[S/P RD U 1:1] [S/I DD D 1:1] [S/N RD U 2:2] [I/S RD U 3:1]", 2, 2, 3, 2);

    // I/P
    VerifyCount(L"[I/P RD U 1:1] [S/I DD D 1:1] [S/N RD U 2:2] [I/S RD U 3:1]", 2, 2, 2, 1);
}

BOOST_AUTO_TEST_CASE(ReplicatorConfiguration)
{
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1]",                     L"[S 1]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [S/S RD U 2:1]",      L"[S 1] [S 2]",     L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD D 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S SB U 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S IB U 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [S/I RD U 2:1]",      L"[S 1]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [S/N RD U 2:1]",      L"[S 1]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [I/S RD U 2:1]",      L"[S 1] [S 2]",     L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [I/S RD D 2:1]",      L"[S 1]",           L"");
    VerifyReplicatorConfiguration(false, false, false, L"[S/S RD U 1:1] [S/P RD U 2:1]",      L"[S 1] [S 2]",     L"");

    VerifyReplicatorConfiguration(true,  false, true,  L"[S/S RD U 1:1 2 2] [S/P RD U 2:1 3 3]", L"[S 1 2 2] [S 2 3 3]", L"");
    

    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1]",                     L"[S 1]",           L"[S 1]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [S/S RD U 2:1]",      L"[S 1] [S 2]",     L"[S 1] [S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD D 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"[S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S SB U 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"[S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S IB U 1:1] [S/S RD U 2:1]",      L"[S 2]",           L"[S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [S/I RD U 2:1]",      L"[S 1]",           L"[S 1] [S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [S/N RD U 2:1]",      L"[S 1]",           L"[S 1] [S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [I/S RD U 2:1]",      L"[S 1] [S 2]",     L"[S 1]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [I/S RD D 2:1]",      L"[S 1]",           L"[S 1]");
    VerifyReplicatorConfiguration(false, true, false, L"[S/S RD U 1:1] [S/P RD U 2:1]",      L"[S 1] [S 2 true]",     L"[S 1] [S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[P/S RD U 1:1] [S/S RD U 2:1]",      L"[S 1] [S 2]",     L"[S 1] [S 2]");
    VerifyReplicatorConfiguration(false, true, false, L"[P/S RD U 1:1] [S/S RD U 2:1]",      L"[S 1] [S 2]",     L"[S 1] [S 2]");

    VerifyReplicatorConfiguration(true, true, true, L"[P/S RD U 1:1 1 1] [S/N RD U 2:1 2 3]", L"[S 1 1 1]", L"[S 1 1 1] [S 2 2 3]");
    VerifyReplicatorConfiguration(true, true, true, L"[P/S RD U 1:1 1 1] [S/N RD U 2:1 -1 -1]", L"[S 1 1 1]", L"[S 1 1 1]");
    VerifyReplicatorConfiguration(true, true, true, L"[P/S RD U 1:1 1 1] [S/I RD U 2:1 -1 -1]", L"[S 1 1 1]", L"[S 1 1 1]");

}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
