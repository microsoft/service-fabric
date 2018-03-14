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


class TestEntitySet : public TestBase
{
protected:
    TestEntitySet()
    {
        utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled | UnitTestContext::Option::StubJobQueueManager);
        perfCtr_ = PerformanceCounterData();
        entity1_ = CreateTestEntityEntry(L"a", utContext_->RA);
        entity2_ = CreateTestEntityEntry(L"b", utContext_->RA);
        timer_ = make_unique<RetryTimer>(L"id", utContext_->RA, utContext_->Config.PeriodicStateCleanupIntervalEntry, L"p", [](wstring const &, ReconfigurationAgent&) {});
        RecreateQueue();
    }

    ~TestEntitySet()
    {
        if (timer_)
        {
            timer_->Close();
        }

        timer_.reset();

        utContext_->Cleanup();
        utContext_.reset();
    }

    void FinalSetup(bool addTimer)
    {
        FinalSetup(addTimer, true);
    }

    void FinalSetup(bool addTimer, bool addPerfCounter)
    {
        set_ = make_unique<EntitySet>(EntitySet::Parameters(L"set name", Id, addPerfCounter ? &perfCtr_ : nullptr));
        utContext_->RA.Test_GetSetCollection().Add(Id, *set_, addTimer ? timer_.get() : nullptr);
    }

    void Add(EntityEntryBaseSPtr const & entity)
    {
        Change(entity, true);
    }

    void Remove(EntityEntryBaseSPtr const & entity)
    {
        Change(entity, false);
    }

    void Change(EntityEntryBaseSPtr const & entity, bool isAdd)
    {
        RecreateQueue();
        auto action = make_unique<EntitySet::ChangeSetMembershipAction>(Id, isAdd);
        queue_->Enqueue(move(action));
        Drain(entity);
    }

    void Drain(EntityEntryBaseSPtr const & entity)
    {
        queue_->ExecuteAllActions(L"aid", entity, utContext_->RA);
    }

    void RecreateQueue()
    {
        queue_ = make_unique<StateMachineActionQueue>();
    }

    void VerifyQueueIsEmpty()
    {
        Verify::IsTrue(queue_->IsEmpty, L"Queue empty");
    }

    void Verify(bool entity1Expected, bool entity2Expected)
    {
        vector<EntityEntryBaseSPtr> expected;
        
        if (entity1Expected)
        {
            expected.push_back(entity1_);
        }

        if (entity2Expected)
        {
            expected.push_back(entity2_);
        }

        sort(expected.begin(), expected.end());

        auto actual = set_->GetEntities();
        sort(actual.begin(), actual.end());

        Verify::AreEqual(expected.size(), actual.size(), L"Set size");

        Verify::Vector(expected, actual);
    }

    void VerifyPerfCounterValue(int expected)
    {
        Verify::AreEqual(expected, perfCtr_.RawValue, L"Perf ctr");
    }

    void IsSubsetOf(bool entity1, bool entity2, bool expected)
    {
        EntityEntryBaseSet input;
        
        if (entity1)
        {
            input.insert(entity1_);
        }

        if (entity2)
        {
            input.insert(entity2_);
        }

        auto actual = set_->IsSubsetOf(input);
        Verify::AreEqual(expected, actual, L"IsSubsetOf");
    }

    EntityEntryBaseSPtr entity1_;
    EntityEntryBaseSPtr entity2_;
    PerformanceCounterData perfCtr_;
    unique_ptr<EntitySet> set_;
    unique_ptr<RetryTimer> timer_;
    UnitTestContextUPtr utContext_;
    unique_ptr<StateMachineActionQueue> queue_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestEntitySetSuite, TestEntitySet)

BOOST_AUTO_TEST_CASE(InitiallySetIsEmpty)
{
    FinalSetup(true);

    Verify(false, false);
}

BOOST_AUTO_TEST_CASE(AddItem)
{
    FinalSetup(true);

    Add(entity1_);

    Verify(true, false);
}

BOOST_AUTO_TEST_CASE(AddMultiple)
{
    FinalSetup(true);

    Add(entity1_);
    Add(entity2_);

    Verify(true, true);
}

BOOST_AUTO_TEST_CASE(RemoveErasesItem)
{
    FinalSetup(true);

    Add(entity1_);

    Remove(entity1_);

    Verify(false, false);
}

BOOST_AUTO_TEST_CASE(RemoveErasesSpecificItem)
{
    FinalSetup(true);

    Add(entity1_);
    Add(entity2_);

    Remove(entity1_);

    Verify(false, true);
}

BOOST_AUTO_TEST_CASE(SetValueIncrementsCounter)
{
    FinalSetup(true);

    Add(entity1_);

    VerifyPerfCounterValue(1);
}

BOOST_AUTO_TEST_CASE(ClearDecrementsCounter)
{
    FinalSetup(true);

    Add(entity1_);

    Remove(entity1_);

    VerifyPerfCounterValue(0);
}

BOOST_AUTO_TEST_CASE(MultipleSetAsserts)
{
    FinalSetup(true);

    Add(entity1_);

    Verify::Asserts([this]() { Add(entity1_); }, L"MultipleAdd");
}

BOOST_AUTO_TEST_CASE(MultipleClearAsserts)
{
    FinalSetup(true);

    Verify::Asserts([this]() { Remove(entity1_); }, L"MultipleClear");
}

BOOST_AUTO_TEST_CASE(SetArmsTheRetryTimerIfSetIsEmpty)
{
    FinalSetup(true);

    Add(entity1_);

    Verify::IsTrue(timer_->IsSet, L"Timer should be armed");
}

BOOST_AUTO_TEST_CASE(SetDoesNotArmTheTimerIfSetAlreadyHadItemsInIt)
{
    FinalSetup(true);

    Add(entity1_);

    timer_->TryCancel(timer_->SequenceNumber);

    Add(entity2_);
    Verify::IsTrue(!timer_->IsSet, L"Timer should not be armed");
}

BOOST_AUTO_TEST_CASE(SetWithNullTimer)
{
    FinalSetup(false);

    Add(entity1_);

    Verify::IsTrue(!timer_->IsSet, L"timer should not be set");
}

BOOST_AUTO_TEST_CASE(ClearWithNullTimer)
{
    FinalSetup(false);

    Add(entity1_);
    
    Remove(entity1_);

    Verify::IsTrue(!timer_->IsSet, L"timer should not be set");
}

BOOST_AUTO_TEST_CASE(SetWithNullPerfCounter)
{
    FinalSetup(false, false);

    Add(entity1_);

    Remove(entity1_);

    VerifyPerfCounterValue(0);
}

BOOST_AUTO_TEST_CASE(GetRandomEntityId)
{
    FinalSetup(false, false);

    Add(entity1_);
    Add(entity2_);

    for(int i = 0; i < 100; i++)
    {
        auto actual = set_->GetRandomEntityTraceId();

        auto e1 = entity1_->GetEntityIdForTrace();
        auto e2 = entity2_->GetEntityIdForTrace();
        if (actual != e1 && actual != e2)
        {
            auto msg = wformatString("Failed as rv '{0}' is not '{1}' or '{2}'. Iteration: {3}", actual, e1, e2, i);
            Verify::Fail(msg);
        }
    }
}

BOOST_AUTO_TEST_CASE(GetRandomEntityIdWithempty)
{
    FinalSetup(false, false);

    auto actual = set_->GetRandomEntityTraceId();
    Verify::AreEqual(string(), actual, L"Empty");
}

BOOST_AUTO_TEST_CASE(EmptyIsSubsetOfEmpty)
{
    FinalSetup(false);

    IsSubsetOf(false, false, true);
}

BOOST_AUTO_TEST_CASE(EmptyIsSubSetOfItems)
{
    FinalSetup(false);

    IsSubsetOf(true, false, true);
}

BOOST_AUTO_TEST_CASE(IsSubsetOfPositiveTest)
{
    FinalSetup(false);

    Add(entity1_);

    IsSubsetOf(true, true, true);
}

BOOST_AUTO_TEST_CASE(IsSubsetOfNegativeTest)
{
    FinalSetup(false);

    Add(entity1_);

    IsSubsetOf(false, true, false);
}

BOOST_AUTO_TEST_CASE(FullIsNotSubsetOfEmpty)
{
    FinalSetup(false);

    Add(entity1_);

    IsSubsetOf(false, false, false);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
