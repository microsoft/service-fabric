// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Common;
using namespace std;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;

namespace
{
    wstring const ActivityId1(L"id1");
    wstring const ActivityId2(L"id2");
    wstring const ActivityId3(L"id3");
}

class ThreadpoolStubEx : public ThreadpoolStub
{
    DENY_COPY(ThreadpoolStubEx);

public:
    ThreadpoolStubEx(ReconfigurationAgent & ra) : ThreadpoolStub(ra, false) {}

    virtual void ExecuteOnThreadpool(ThreadpoolCallback callback)
    {
        ThreadpoolStub::ExecuteOnThreadpool(callback);
        enqueueTimeStamps_.push_back(Stopwatch::Now());
    }
        
    static void WorkFunction(wstring const& activityId, ReconfigurationAgent& ra, BackgroundWorkManager& bgm)
    {
        ThreadpoolStubEx & tp = dynamic_cast<ThreadpoolStubEx&>(ra.Threadpool);
        Sleep(millisecondsToTakeToProcess_);
        tp.activityIds_.push_back(activityId);
        tp.workFunctionExecutionTimestamps_.push_back(Stopwatch::Now());

        bgm.OnWorkComplete();
    }

    void ProcessLastContext(ReconfigurationAgent & ra, int millisecondsToTakeToProcess = 0)
    {
        UNREFERENCED_PARAMETER(ra);
        TestLog::WriteInfo(wformatString("ProcessLastContext {0}", millisecondsToTakeToProcess).c_str());
        VERIFY_IS_TRUE(!ThreadpoolCallbacks.IsEmpty, L"Expected a context to complete");
        millisecondsToTakeToProcess_ = millisecondsToTakeToProcess;
        ThreadpoolCallbacks.ExecuteLastAndPop(ra);
    }

    void Verify(int expectedEnqueues, int expectedExecution)
    {
        TestLog::WriteInfo(wformatString("Verify. ExpectedEnqueues {0}. ActualEnqueues {1}. ExpectedExecution {2}. ActualExecution {3}", expectedEnqueues, enqueueTimeStamps_.size(), expectedExecution, workFunctionExecutionTimestamps_.size()).c_str());

        VERIFY_ARE_EQUAL(expectedEnqueues, static_cast<int>(enqueueTimeStamps_.size()), L"Expected enqueues mismatch");
        VERIFY_ARE_EQUAL(expectedExecution, static_cast<int>(workFunctionExecutionTimestamps_.size()), L"Expected exec mismatch");
    }

    bool TryVerify(int expectedEnqueues, int expectedExecution)
    {
        if (expectedEnqueues != static_cast<int>(enqueueTimeStamps_.size()))
        {
            return false;        
        }

        if(expectedExecution != static_cast<int>(workFunctionExecutionTimestamps_.size()))
        {
            return false;
        }

        return true;
    }

public:
    static int millisecondsToTakeToProcess_;

    vector<StopwatchTime> workFunctionExecutionTimestamps_;
    vector<StopwatchTime> enqueueTimeStamps_;
    vector<wstring> activityIds_;        
};

int ThreadpoolStubEx::millisecondsToTakeToProcess_;

class TestBackgroundWorkManager
{
protected:
    TestBackgroundWorkManager() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestBackgroundWorkManager() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    UnitTestContextUPtr utContext_;
    unique_ptr<BackgroundWorkManager> bgm_;
    ThreadpoolStubEx* tp_;
        
    void SetMinIntervalBetweenExecutionToInstantaneous()
    {
        GetConfigEntry().Test_SetValue(InstantaneousTimeSpan);
    }

    void VerifyTimeLessThan(StopwatchTime first, StopwatchTime second, TimeSpan expected)
    {
        VerifyTime(first, second, expected, true);
    }

    void VerifyTimeGreaterThan(StopwatchTime first, StopwatchTime second, TimeSpan expected)
    {
        VerifyTime(first, second, expected, false);
    }

    void VerifyTime(StopwatchTime first, StopwatchTime second, TimeSpan expected, bool isLessThan)
    {
        VerifyTime(first - second, expected, isLessThan);
    }

    void VerifyTime(TimeSpan elapsed, TimeSpan expected, bool isLessThan)
    {
        auto delta = static_cast<int64>(elapsed.TotalPositiveMilliseconds()) - static_cast<int64>(expected.TotalPositiveMilliseconds());

        wstring text = wformatString("Elapsed {0}. Expected {1}. Delta {2}", elapsed, expected, delta);
        TestLog::WriteInfo(Common::wformatString("{0}", text));

        // If isLessThan is true we expect the delta to be < 0 i.e. the elapsed time should have been less than expected
        // If isLessThan is false we expect the delta to be > 0
        // If these conditions do not hold then assert that the delta is within a confidence interval
        // This can happen because the measurements are based on time and context switches etc can cause the measurements to get skewed
        // Especially if there is load on the system
        if ((delta > 0 && isLessThan) ||
            (delta < 0 && !isLessThan))
        {
            // We should be within a confidence interval
            double compareTo = 4.0 * expected.TotalPositiveMilliseconds() / 5.0;
                
            // make positive
            if (delta < 0)
            {
                delta = delta * -1;
            }

            wstring text2 = wformatString("abs(delta) {0}. Comparing to {1}. isLessThan {2}", delta, compareTo, isLessThan); 
            TestLog::WriteInfo(Common::wformatString("{0}", text2));
            VERIFY_IS_TRUE(delta < compareTo);
        }
    }

    TimeSpanConfigEntry & GetConfigEntry()
    {
        return utContext_->Config.MinimumIntervalBetweenReconfigurationMessageRetryEntry;
    }

    TimeSpan InstantaneousTimeSpan;
};

bool TestBackgroundWorkManager::TestSetup()
{
	InstantaneousTimeSpan = TimeSpan::FromMilliseconds(500);

	utContext_ = UnitTestContext::Create(UnitTestContext::Option::StubJobQueueManager);

    tp_ = new ThreadpoolStubEx(utContext_->RA);
    unique_ptr<IThreadpool> tpPtr(tp_);

    utContext_->UpdateThreadpool(std::move(tpPtr));

    GetConfigEntry().Test_SetValue(TimeSpan::Zero);

    bgm_ = make_unique<BackgroundWorkManager>(L"name", ThreadpoolStubEx::WorkFunction, GetConfigEntry(), utContext_->RA);

	return true;
}

bool TestBackgroundWorkManager::TestCleanup()
{
	bgm_->Close();
	bgm_.reset();
	return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(TestBackgroundWorkManagerSuite,TestBackgroundWorkManager)

BOOST_AUTO_TEST_CASE(FirstRequestCausesEnqueueFunctionToBeCalledImmediately)
{
    StopwatchTime start = Stopwatch::Now();
    bgm_->Request(L"id1");

    tp_->Verify(1, 0);
    VerifyTimeLessThan(tp_->enqueueTimeStamps_[0], start, InstantaneousTimeSpan);        
}

BOOST_AUTO_TEST_CASE(RequestAndProcessExecutesWorkFunction)
{
    bgm_->Request(ActivityId1);

    tp_->ProcessLastContext(utContext_->RA);

    // work function should have been executed
    tp_->Verify(1, 1);

    Verify::VectorStrict(tp_->activityIds_, ActivityId1);
}
    
BOOST_AUTO_TEST_CASE(MultipleEnqueuesWithoutExecutionCauseOnlyOneContextToBeEnqueued)
{
    // simulate 100 threads requesting execution without anyone actually executing
    const int enqueueCount = 100;
    Common::atomic_long callCount;

    StopwatchTime start = Stopwatch::Now();
    bgm_->Request(ActivityId1);

    for(int i = 0; i < 100; i++)
    {
        Common::Threadpool::Post([this, &callCount]
        {
            bgm_->Request(ActivityId2);
            ++callCount;
        });
    }

    BusyWaitUntil([&callCount, &enqueueCount] { return callCount.load() == enqueueCount; });

    // only one item should have been enqueued
    tp_->Verify(1, 0);
        
    // the enqueue should have been instantaneous with respect to the very first enqueue
    VerifyTimeLessThan(tp_->enqueueTimeStamps_[0], start, InstantaneousTimeSpan);

    // execute
    tp_->ProcessLastContext(utContext_->RA);

    Verify::VectorStrict(tp_->activityIds_, ActivityId1);
}

BOOST_AUTO_TEST_CASE(RequestProcessAndThenRequestAfterMinIntervalCausesSecondRequestToBeEnqueuedImmediately)
{
    bgm_->Request(ActivityId1);

    tp_->Verify(1, 0);

    // execute the request
    tp_->ProcessLastContext(utContext_->RA);

    // nothing more should have been enqueued and the work function should have been called once
    tp_->Verify(1, 1);

    TestLog::WriteInfo(L"Setting min interval");

    // Set the min interval to a small value
    SetMinIntervalBetweenExecutionToInstantaneous();

    TestLog::WriteInfo(wformatString("Sleep {0}", Stopwatch::Now()).c_str());

    // Wait for more than that interval before resuming
    Sleep(static_cast<DWORD>(InstantaneousTimeSpan.TotalPositiveMilliseconds() * 2));

    TestLog::WriteInfo(wformatString("Request {0}", Stopwatch::Now()).c_str());

    // request again
    StopwatchTime start = Stopwatch::Now();
    bgm_->Request(ActivityId2);
        
    // enqueue function should have been called once more
    tp_->Verify(2, 1);
    VerifyTimeLessThan(tp_->enqueueTimeStamps_[1], start, InstantaneousTimeSpan);

    // process
    tp_->ProcessLastContext(utContext_->RA);

    Verify::VectorStrict(tp_->activityIds_, ActivityId1, ActivityId2);
}

BOOST_AUTO_TEST_CASE(RequestProcessAndThenRequestBeforeMinIntervalCausesSecondRequestToBeEnqueuedAfterADelay)
{
    bgm_->Request(ActivityId1);

    tp_->Verify(1, 0);

    // execute the request
    tp_->ProcessLastContext(utContext_->RA);

    // nothing more should have been enqueued and the work function should have been called once
    tp_->Verify(1, 1);

    // Set the min interval to a small value
    SetMinIntervalBetweenExecutionToInstantaneous();

    // request again
    StopwatchTime start = Stopwatch::Now();
    bgm_->Request(ActivityId2);
        
    // enqueue function should have been called once more
    // but it should take time
    tp_->Verify(1, 1);

    BusyWaitUntil([this] { return tp_->TryVerify(2, 1); });

    // the delta should be greater than min interval
    VerifyTimeGreaterThan(tp_->enqueueTimeStamps_[1], start, InstantaneousTimeSpan);
}

BOOST_AUTO_TEST_CASE(RequestRequestAndThenProcessShouldCauseSecondRequestToBeEnqueuedImmediatelyAfterWorkIsDoneIfProcessingTookLongerThanMinRetryInterval)
{
    bgm_->Request(ActivityId1);

    tp_->Verify(1, 0);

    // set the min retry interval
    SetMinIntervalBetweenExecutionToInstantaneous();

    // request again
    bgm_->Request(ActivityId2);

    // request again - we want to make sure that activity id2 is what is passed in
    bgm_->Request(ActivityId3);

    // no change should have occurred - bgm should track that one request is pending and one is currently executing
    tp_->Verify(1, 0);

    // execute the request (in effect simulating a scenario where request execution took longer than the min retry interval)
    tp_->ProcessLastContext(utContext_->RA, static_cast<int>(InstantaneousTimeSpan.TotalMilliseconds() * 2));

    // there was already a request waiting - it should be enqueued immediately
    tp_->Verify(2, 1);
        
    // proces
    tp_->ProcessLastContext(utContext_->RA);

    Verify::VectorStrict(tp_->activityIds_, ActivityId1, ActivityId2);
}

BOOST_AUTO_TEST_CASE(RequestRequestAndThenProcessShouldCauseSecondRequestToBeEnqueuedAfterWaitIfProcessingTookLessThanMinRetryInterval)
{
    bgm_->Request(ActivityId1);

    tp_->Verify(1, 0);

    // set the min retry interval
    SetMinIntervalBetweenExecutionToInstantaneous();

    StopwatchTime start = Stopwatch::Now();

    // request again
    bgm_->Request(ActivityId2);

    // no change should have occurred - bgm should track that one request is pending and one is currently executing
    tp_->Verify(1, 0);

    // execute the request immediately
    tp_->ProcessLastContext(utContext_->RA);

    // the last request was made very recently
    // the bgm should wait until the timeout
    tp_->Verify(1, 1);

    BusyWaitUntil([this] { return tp_->TryVerify(2, 1);});
    VerifyTimeGreaterThan(tp_->enqueueTimeStamps_[1], start, InstantaneousTimeSpan);
}    
    
BOOST_AUTO_TEST_CASE(RequestRequestProcessProcessShouldCauseTwoExecutions)
{
    bgm_->Request(ActivityId1);
        
    bgm_->Request(ActivityId2);

    bgm_->Request(ActivityId3);

    tp_->ProcessLastContext(utContext_->RA);

    tp_->Verify(2, 1);
        
    tp_->ProcessLastContext(utContext_->RA);

    // at this point we should have executed the second unit of work
    // verify that nothing more was enqueued for execution 
    tp_->Verify(2, 2);

    Verify::VectorStrict(tp_->activityIds_, ActivityId1, ActivityId2);
}

BOOST_AUTO_TEST_CASE(MultipleCloseSucceeds)
{
    bgm_->Close();

    bgm_->Close();
}

BOOST_AUTO_TEST_CASE(RequestingAfterCloseIsNoOp)
{
    bgm_->Close();

    bgm_->Request(ActivityId1);

    tp_->Verify(0, 0);
}

BOOST_AUTO_TEST_CASE(ClosingWhileTimerIsArmedDoesNotCauseCb)
{
    bgm_->Request(ActivityId1);

    tp_->Verify(1, 0);

    // set the min retry interval
    SetMinIntervalBetweenExecutionToInstantaneous();

    // request again
    bgm_->Request(ActivityId2);

    bgm_->Close();

    // no change should have occurred - bgm should track that one request is pending and one is currently executing
    tp_->Verify(1, 0);

    // execute the request immediately
    tp_->ProcessLastContext(utContext_->RA);

    // the last request was made very recently
    // the bgm should wait until the timeout
    tp_->Verify(1, 1);

    Sleep(static_cast<DWORD>(InstantaneousTimeSpan.TotalMilliseconds() * 3));

    tp_->Verify(1, 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
