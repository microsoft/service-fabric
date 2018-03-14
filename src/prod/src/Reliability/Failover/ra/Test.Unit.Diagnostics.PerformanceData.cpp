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
using namespace Diagnostics;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestEntityExecutionPerformanceData
{
protected:
    TestEntityExecutionPerformanceData()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~TestEntityExecutionPerformanceData()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void AdvanceTime(int seconds)
    {
        clock_.AdvanceTimeBySeconds(seconds);
    }

protected:
    TestClock clock_;
    ExecuteEntityJobItemListPerformanceData entityExecutePerfData_;
    CommitEntityPerformanceData commitPerfData_;
    PerformanceCountersSPtr perfCounters_;
    unique_ptr<PerformanceCounterHelper> perfCounterHelper_;
};

bool TestEntityExecutionPerformanceData::TestSetup()
{
    perfCounters_ = RAPerformanceCounters::CreateInstance(Guid::NewGuid().ToString());
    perfCounterHelper_ = make_unique<PerformanceCounterHelper>(perfCounters_);
    clock_.SetManualMode();
    return true;
}

bool TestEntityExecutionPerformanceData::TestCleanup()
{
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestPerformanceDataSuite, TestEntityExecutionPerformanceData)

BOOST_AUTO_TEST_CASE(ExecutePerfCountersAreReported)
{
    int processTime = 5;
    int postProcessTime = 10;
    int commitTime = 203;
    int lockTimeExpected = processTime + postProcessTime + commitTime;
    int jobItems = 44;

    entityExecutePerfData_.OnExecutionStart(jobItems, clock_);

    AdvanceTime(processTime);

    entityExecutePerfData_.OnProcessed(clock_);

    AdvanceTime(commitTime);

    entityExecutePerfData_.OnPostProcessStart(clock_);

    AdvanceTime(postProcessTime);

    entityExecutePerfData_.OnExecutionEnd(clock_);

    entityExecutePerfData_.ReportPerformanceData(*perfCounters_);

    perfCounterHelper_->VerifyAveragePerformanceCounter(
        perfCounters_->AverageEntityLockTimeBase,
        perfCounters_->AverageEntityLockTime,
        lockTimeExpected * 1000,
        L"Lock time");

    perfCounterHelper_->VerifyAveragePerformanceCounter(
        perfCounters_->AverageJobItemsPerEntityLockBase,
        perfCounters_->AverageJobItemsPerEntityLock,
        jobItems,
        L"Job items per lock");

    Verify::AreEqual(lockTimeExpected, entityExecutePerfData_.LockDuration.TotalSeconds(), L"Lock time");
    Verify::AreEqual(processTime, entityExecutePerfData_.ProcessDuration.TotalSeconds(), L"Process duration");
    Verify::AreEqual(postProcessTime, entityExecutePerfData_.PostProcessDuration.TotalSeconds(), L"post process duration");
}

BOOST_AUTO_TEST_CASE(CommitPerfCountersAreReported)
{
    int duration = 23;

    commitPerfData_.OnStoreCommitStart(clock_);

    AdvanceTime(duration);

    commitPerfData_.OnStoreCommitEnd(clock_);

    commitPerfData_.ReportPerformanceData(*perfCounters_);

    perfCounterHelper_->VerifyAveragePerformanceCounter(
        perfCounters_->AverageEntityCommitTimeBase,
        perfCounters_->AverageEntityCommitTime,
        23 * 1000,
        L"commit duration");

    Verify::AreEqual(duration, commitPerfData_.CommitDuration.TotalSeconds(), L"Commit time");
}

BOOST_AUTO_TEST_CASE(NoCommitPerfCountersAreReportedIfNoCommitHappens)
{
    commitPerfData_.ReportPerformanceData(*perfCounters_);

    perfCounterHelper_->VerifyAveragePerformanceCounterUnchanged(
        perfCounters_->AverageEntityCommitTimeBase,
        perfCounters_->AverageEntityCommitTime,
        L"commit duration");

    Verify::AreEqual(0, commitPerfData_.CommitDuration.TotalSeconds(), L"Commit time");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
