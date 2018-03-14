// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Infrastructure::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

typedef std::function<bool(ReadOnlyLockedTestEntityPtr &)> FilterFunc;

class TestEntityMapAlgorithm
{
protected:
    TestEntityMapAlgorithm()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestEntityMapAlgorithm()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    TestClock & GetClock()
    {
        return utContext_->Clock;
    }

    EntityEntryBaseList ExecuteAndReturnResult(
        JobItemCheck::Enum jobItemCheck,
        FilterFunc filter)
    {
        Diagnostics::FilterEntityMapPerformanceData perfData(utContext_->IClockSPtr);
        return FilterUnderReadLockOverMap<TestEntity>(*entityMap_, jobItemCheck, filter, perfData);
    }

    void ExecuteAndVerifyFilterNotCalled(
        JobItemCheck::Enum jobItemCheck)
    {
        bool wasCalled = false;

        auto result = ExecuteAndReturnResult(jobItemCheck, [&](ReadOnlyLockedTestEntityPtr &) { wasCalled = true; return true; });
        Verify::AreEqual(false, wasCalled, L"Filter should not be called for deleted");
        Verify::AreEqual(0, result.size(), L"No items should be returned");
    }

    Diagnostics::FilterEntityMapPerformanceData ExecuteAndReturnPerfData(FilterFunc filter)
    {
        Diagnostics::FilterEntityMapPerformanceData perfData(utContext_->IClockSPtr);
        FilterUnderReadLockOverMap<TestEntity>(*entityMap_, JobItemCheck::None, filter, perfData);
        return perfData;
    }

protected:
    InfrastructureTestUtilityUPtr infrastructureUtility_;
    UnitTestContextUPtr utContext_;
    TestEntityMap * entityMap_;
    
};

bool TestEntityMapAlgorithm::TestSetup()
{    
    utContext_ = InfrastructureTestUtility::CreateUnitTestContextWithTestEntityMap();
    entityMap_ = &utContext_->ContextMap.Get<TestEntityMap>(TestContextMap::TestEntityMapKey);
    infrastructureUtility_ = make_unique<InfrastructureTestUtility>(*utContext_);
    GetClock().SetManualMode();
    return true;
}

bool TestEntityMapAlgorithm::TestCleanup()
{
    utContext_->Cleanup();
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestEntityMapAlgorithmSuite, TestEntityMapAlgorithm)

BOOST_AUTO_TEST_CASE(Filter_DeletedItemIsSkipped)
{
    auto entity = infrastructureUtility_->SetupDeleted();

    ExecuteAndVerifyFilterNotCalled(JobItemCheck::None);
}

BOOST_AUTO_TEST_CASE(Filter_NotPassingCheckIsSkipped)
{
    auto entity = infrastructureUtility_->SetupCreated();

    ExecuteAndVerifyFilterNotCalled(JobItemCheck::FTIsNotNull);
}

BOOST_AUTO_TEST_CASE(Filter_FilterIsCalledForEachEntityAndFilteredAreReturned)
{
    auto entity1 = infrastructureUtility_->SetupInserted(1, 1);
    
    auto entity2 = infrastructureUtility_->GetOrCreate(L"key2");
    infrastructureUtility_->Insert(entity2, 1, 2, PersistenceResult::Success);

    auto result = ExecuteAndReturnResult(
        JobItemCheck::None, 
        [](ReadOnlyLockedTestEntityPtr & e)
        {
            return e->InMemoryData == 2;
        });

    Verify::Vector(result, entity2);
}

BOOST_AUTO_TEST_CASE(Filter_PerfDataIsCorrect)
{
    auto entity1 = infrastructureUtility_->SetupInserted(1, 1);

    auto entity2 = infrastructureUtility_->GetOrCreate(L"key2");
    infrastructureUtility_->Insert(entity2, 1, 2, PersistenceResult::Success);

    auto perfData = ExecuteAndReturnPerfData(
        [&](ReadOnlyLockedTestEntityPtr & e)
        {
            GetClock().AdvanceTimeBySeconds(3);
            return e->InMemoryData == 2;
        });

    Verify::AreEqual(2, perfData.MapItemCount, L"MapItemCount");
    Verify::AreEqual(1, perfData.FilteredItemCount, L"filtered item count");
    Verify::IsTrue(perfData.FilterDuration.TotalSeconds() >= 6, L"filter seconds");
    Verify::IsTrue(perfData.SnapshotDuration.TotalSeconds() >= 0 && perfData.SnapshotDuration.TotalSeconds() < 500, L"Total seconds");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
