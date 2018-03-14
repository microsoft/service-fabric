// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Common;
using namespace std;
using namespace Reliability::ReconfigurationAgentComponent::Infrastructure::ReliabilityUnitTest;

using Diagnostics::ScheduleEntityPerformanceData;

namespace
{
    const wstring Id1(L"id1");
    const wstring Id2(L"id2");
    const wstring Id3(L"id3");
    const wstring Id4(L"id4");
    const int TraceThreshold(40);
}


class TestFailoverUnitScheduler
{
protected:
    typedef EntityScheduler<TestEntity> EntitySchedulerType;
    

    TestFailoverUnitScheduler() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestFailoverUnitScheduler() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    ThreadpoolStub & GetThreadpool()
    {
        return testContext_->ThreadpoolStubObj;
    }

    TestJobItemSPtr CreateJobItem()
    {
        TestJobItem::Parameters parameters(
            entry_,
            L"aid",
            [](TestEntityHandlerParameters &, TestJobItemContext&) {return true; },
            JobItemCheck::Default,
            *JobItemDescription::MessageProcessing);

        return make_shared<TestJobItem>(move(parameters));
    }

    EntitySchedulerType & GetScheduler()
    {
        return entry_->Scheduler;
    }

    AsyncOperationSPtr Schedule(TestJobItemSPtr const & ji)
    {
        return GetScheduler().BeginScheduleWorkItem(
            ji,
            testContext_->RA.Threadpool,
            testContext_->Clock,
            testContext_->RA.PerfCounters,
            [](AsyncOperationSPtr const &) {},
            testContext_->RA.Root.CreateAsyncOperationRoot());
    }

    void ScheduleAndEnd(TestJobItemSPtr const & ji)
    {
        auto op = Schedule(ji);

        Verify::IsTrue(op->CompletedSynchronously, L"Should have been completed if calling schedule and end");
        End(op);
    }

    void CreateScheduleFinishAndEnd()
    {
        auto initialJI = CreateJobItem();
        auto initialOp = Schedule(initialJI);
        FinishQueuedThreadpoolWork();
        End(initialOp);
    }

    EntityJobItemList End(AsyncOperationSPtr const & op)
    {
        EntityJobItemList result;
        ScheduleEntityPerformanceData perfData;
        auto error = GetScheduler().EndScheduleWorkItem(op, result, perfData);
        Verify::IsTrue(error.IsSuccess(), L"Schedule can never fail");
        return result;
    }

    void EndAndVerifyPerfCounters(
        AsyncOperationSPtr const & op, 
        int64 expectedScheduleTime,
        int64 expectedQueueTime)
    {
        EntityJobItemList result;
        ScheduleEntityPerformanceData perfData;
        auto error = GetScheduler().EndScheduleWorkItem(op, result, perfData);
        Verify::IsTrue(error.IsSuccess(), L"Schedule can never fail");        
        Verify::AreEqual(expectedScheduleTime, perfData.ScheduleDuration.TotalSeconds(), L"Schedule time");
        Verify::AreEqual(expectedQueueTime, perfData.QueueDuration.TotalSeconds(), L"queue time");
    }

    void EndAndVerifyQueued(AsyncOperationSPtr const & op)
    {
        auto result = End(op);
        Verify::IsTrue(result.empty(), L"Must be empty");
    }

    void EndAndVerify(AsyncOperationSPtr const & op, TestJobItemSPtr const & ji)
    {
        vector<EntityJobItemBaseSPtr> v;
        v.push_back(ji);
        EndAndVerify(op, v);
    }

    void EndAndVerify(AsyncOperationSPtr const & op, TestJobItemSPtr const & ji1, TestJobItemSPtr const & ji2)
    {
        vector<EntityJobItemBaseSPtr> v;
        v.push_back(ji1);
        v.push_back(ji2);
        EndAndVerify(op, v);
    }

    void EndAndVerify(AsyncOperationSPtr const & op, EntityJobItemList const & expected)
    {
        EntityJobItemList result = End(op);
        
        Verify::IsTrue(!result.empty(), L"Result should not be empty if result is expected");
        Verify::VectorStrict(expected, result);;
    }

    void FinishQueuedThreadpoolWork()
    {
        Verify::IsTrue(!GetThreadpool().FTQueue.IsEmpty, L"Threadpool cant be empty");
        GetThreadpool().FTQueue.Drain(testContext_->RA);
    }

    void ReleaseLock()
    {
        GetScheduler().ReleaseLock(testContext_->Clock, testContext_->RA.PerfCounters);
    }

    void AdvanceTime(int seconds)
    {
        testContext_->Clock.AdvanceTimeBySeconds(seconds);
    }

    InfrastructureTestUtilityUPtr utility_;
    UnitTestContextUPtr testContext_;
    TestEntityEntrySPtr entry_;
};

bool TestFailoverUnitScheduler::TestSetup()
{
    testContext_ = InfrastructureTestUtility::CreateUnitTestContextWithTestEntityMap();
    testContext_->Clock.SetManualMode();
    utility_ = make_unique<InfrastructureTestUtility>(*testContext_); 
    entry_ = dynamic_pointer_cast<TestEntityEntry>(utility_->SetupCreated());
	return true;
}

bool TestFailoverUnitScheduler::TestCleanup()
{
	testContext_->Cleanup();
	return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFailoverUnitSchedulerSuite,TestFailoverUnitScheduler)

BOOST_AUTO_TEST_CASE(PerformanceDataIsCorrectForScheduledEntity)
{
    auto ji = CreateJobItem();

    auto op = Schedule(ji);

    AdvanceTime(1);

    FinishQueuedThreadpoolWork();

    EndAndVerifyPerfCounters(op, 1, 0);
}

BOOST_AUTO_TEST_CASE(WorkIsCompletedWithResultWhenScheduleCompletes)
{
    auto ji = CreateJobItem();

    auto op = Schedule(ji);

    Verify::IsTrue(!op->IsCompleted, L"Cannot be completed");

    FinishQueuedThreadpoolWork();

    EndAndVerify(op, ji);
}

BOOST_AUTO_TEST_CASE(BatchedWorkWhenScheduledReturnsIsPending)
{
    auto ji1 = CreateJobItem();
    auto ji2 = CreateJobItem();

    auto op1 = Schedule(ji1);

    auto op2 = Schedule(ji2);
    Verify::IsTrue(op2->CompletedSynchronously, L"Op must be completed");
    EndAndVerifyQueued(op2);

    FinishQueuedThreadpoolWork();
    End(op1);
}

BOOST_AUTO_TEST_CASE(MultipleWorkIsBatchedIfWorkArrivesDuringScheduling)
{
    auto ji1 = CreateJobItem();
    auto ji2 = CreateJobItem();

    auto op = Schedule(ji1);
    
    ScheduleAndEnd(ji2);

    FinishQueuedThreadpoolWork();

    EndAndVerify(op, ji1, ji2);
}

BOOST_AUTO_TEST_CASE(EnqueueAfterReleaseLock)
{
    CreateScheduleFinishAndEnd();

    ReleaseLock();

    auto ji2 = CreateJobItem();
    auto op2 = Schedule(ji2);

    FinishQueuedThreadpoolWork();

    EndAndVerify(op2, ji2);
}

BOOST_AUTO_TEST_CASE(NewWorkIsPendingUtilPreviousLockIsReleased)
{
    CreateScheduleFinishAndEnd();

    auto jiEnqueuedDuringLock = CreateJobItem();
    auto opDuringLock = Schedule(jiEnqueuedDuringLock);
    Verify::IsTrue(!opDuringLock->IsCompleted, L"Cannot be completed");
    Verify::IsTrue(GetThreadpool().FTQueue.IsEmpty, L"Cannot schedule while lock is held");

    ReleaseLock();

    // Work should now be scheduled and async op should complete
    FinishQueuedThreadpoolWork();
    EndAndVerify(opDuringLock, jiEnqueuedDuringLock);
}

BOOST_AUTO_TEST_CASE(MultipleNewWorkIsQueuedUntilPreviousLockIsReleased)
{
    CreateScheduleFinishAndEnd();

    auto firstJobScheduledDuringLock = CreateJobItem();
    auto firstJobOp = Schedule(firstJobScheduledDuringLock);
    Verify::IsTrue(!firstJobOp->IsCompleted, L"Cannot be completed");
    Verify::IsTrue(GetThreadpool().FTQueue.IsEmpty, L"Cannot schedule while lock is held");

    auto secondJobScheduledDuringLock = CreateJobItem();
    auto secondJobOp = Schedule(secondJobScheduledDuringLock);

    EndAndVerifyQueued(secondJobOp);

    ReleaseLock();

    // Work should now be scheduled and async op should complete
    FinishQueuedThreadpoolWork();
    EndAndVerify(firstJobOp, firstJobScheduledDuringLock, secondJobScheduledDuringLock);
}

BOOST_AUTO_TEST_CASE(QueueTimeIsMeasured)
{
    CreateScheduleFinishAndEnd();

    auto ji = CreateJobItem();
    auto op = Schedule(ji);

    AdvanceTime(2);

    ReleaseLock();

    FinishQueuedThreadpoolWork();

    EndAndVerifyPerfCounters(op, 0, 2);
}

BOOST_AUTO_TEST_CASE(ScheduleTimeForQueuedItemIsMeasured)
{
    CreateScheduleFinishAndEnd();

    auto ji = CreateJobItem();
    auto op = Schedule(ji);

    ReleaseLock();

    AdvanceTime(5);

    FinishQueuedThreadpoolWork();

    EndAndVerifyPerfCounters(op, 5, 0);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
