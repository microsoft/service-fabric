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
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    const EntitySetIdentifier Id(EntitySetName::Test, FailoverManagerId(true));
}

class TestSetMembership
{
protected:
    TestSetMembership()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestSetMembership()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void FinalSetup(bool addTimer)
    {
        FinalSetup(addTimer, true);
    }

    void FinalSetup(bool addTimer, bool addPerfCounter)
    {
        set_ = make_unique<EntitySet>(EntitySet::Parameters(L"set name", Id, addPerfCounter ? &perfCtr_ : nullptr));
        utContext_->RA.Test_GetSetCollection().Add(Id, *set_, addTimer ? timer_.get() : nullptr);
    }

    void ExecuteChange(bool value)
    {
        flag_->SetValue(value, *queue_);
    }

    void Drain()
    {
        queue_->ExecuteAllActions(L"aid", entity_, utContext_->RA);
    }

    void RecreateQueue()
    {
        queue_ = make_unique<StateMachineActionQueue>();
    }

    void ExecuteAndDrain(bool value)
    {
        ExecuteChange(value);
        Drain();
    }

    void VerifyQueueIsEmpty()
    {
        Verify::IsTrue(queue_->IsEmpty, L"Queue empty");
    }

    void VerifyIsSet()
    {
        Verify(true);
    }

    void VerifyIsNotSet()
    {
        Verify(false);
    }

    void Verify(bool expected)
    {
        Verify::AreEqualLogOnError(expected, flag_->IsSet, L"Flag state");

        if (expected)
        {
            auto actual = set_->GetEntities();
            Verify::AreEqual(1, actual.size(), L"Size");
            Verify::AreEqual(entity_, actual[0], L"item");
        }
        else
        {
            Verify::AreEqual(0, set_->Size, L"Size");
        }
    }

    void VerifyPerfCounterValue(int expected)
    {
        Verify::AreEqual(expected, perfCtr_.RawValue, L"Perf ctr");
    }

    EntityEntryBaseSPtr entity_;
    PerformanceCounterData perfCtr_;
    unique_ptr<EntitySet> set_;
    unique_ptr<SetMembershipFlag> flag_;
    unique_ptr<RetryTimer> timer_;
    UnitTestContextUPtr utContext_;
    unique_ptr<StateMachineActionQueue> queue_;
private:
};

bool TestSetMembership::TestSetup()
{
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled | UnitTestContext::Option::StubJobQueueManager);
    perfCtr_ = PerformanceCounterData();
    entity_ = CreateTestEntityEntry(L"a", utContext_->RA);
    timer_ = make_unique<RetryTimer>(L"id", utContext_->RA, utContext_->Config.PeriodicStateCleanupIntervalEntry, L"p", [](wstring const &, ReconfigurationAgent&) {});
    flag_ = make_unique<SetMembershipFlag>(Id);    
    RecreateQueue();
    return true;
}

bool TestSetMembership::TestCleanup()
{
    if (timer_) { timer_->Close(); }
    timer_.reset();
    return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestSetMembershipSuite, TestSetMembership)

BOOST_AUTO_TEST_CASE(InitiallySetIsEmpty)
{
    FinalSetup(true);

    VerifyIsNotSet();
}

BOOST_AUTO_TEST_CASE(SetValueTransitionsToTrue)
{
    FinalSetup(true);

    ExecuteAndDrain(true);

    VerifyIsSet();
}

BOOST_AUTO_TEST_CASE(ClearValueAfterSet)
{
    FinalSetup(true);

    ExecuteAndDrain(true);

    RecreateQueue();

    ExecuteAndDrain(false);

    VerifyIsNotSet();
}

BOOST_AUTO_TEST_CASE(MultipleSetDoNotEnqueueAdditionalActions)
{
    FinalSetup(true);

    ExecuteAndDrain(true);

    RecreateQueue();

    ExecuteChange(true);

    VerifyQueueIsEmpty();
}

BOOST_AUTO_TEST_CASE(MultipleClearDoNotEnqueueAdditionalActions)
{
    FinalSetup(true);

    ExecuteChange(false);

    VerifyQueueIsEmpty();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
