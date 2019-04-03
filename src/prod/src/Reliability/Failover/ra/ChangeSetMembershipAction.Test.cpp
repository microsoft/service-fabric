// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;
using namespace Infrastructure;
using namespace Common;
using namespace std;

class Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::TestChangeSetMembershipAction 
{
protected:
	TestChangeSetMembershipAction() { BOOST_REQUIRE(TestSetup()); }
	~TestChangeSetMembershipAction() { BOOST_REQUIRE(TestCleanup()); }
	TEST_METHOD_SETUP(TestSetup);
	TEST_METHOD_CLEANUP(TestCleanup);

    static const EntitySetName::Enum SetName = EntitySetName::ReconfigurationMessageRetry;

	void AddToSetAddsItemToSet();
	void AddSetsTheRetryTimer();
	void RemoveFromSetRemovesFromSet();
	void RemoveDoesNotUnsetTheRetryTimer();
	void RemoveDoesNotSetTheRetryTimer();

private:
    EntityEntryBaseSPtr entry_;

    EntityEntryBaseSPtr GetEntity()
    {
        return entry_;
    }

    void Setup();

    unique_ptr<EntitySet::ChangeSetMembershipAction> CreateAction(bool addToSet);

    EntitySet & GetSet();
    RetryTimer & GetRetryTimer();

    UnitTestContextUPtr testContext_;
};

const EntitySetName::Enum TestChangeSetMembershipAction::SetName;

void TestChangeSetMembershipAction::Setup()
{
	testContext_ = UnitTestContext::Create();
    entry_ = make_shared<LocalFailoverUnitMapEntry>(FailoverUnitId());
}

bool TestChangeSetMembershipAction::TestSetup()
{
	return true;
}

bool TestChangeSetMembershipAction::TestCleanup()
{
	if (testContext_)
	{
		testContext_->Cleanup();
	}

	return true;
}

unique_ptr<EntitySet::ChangeSetMembershipAction> TestChangeSetMembershipAction::CreateAction(bool addToSet)
{
    return make_unique<EntitySet::ChangeSetMembershipAction>(TestChangeSetMembershipAction::SetName, addToSet);
}

EntitySet & TestChangeSetMembershipAction::GetSet()
{
	return testContext_->RA.GetSet(TestChangeSetMembershipAction::SetName);
}

RetryTimer & TestChangeSetMembershipAction::GetRetryTimer()
{
	return testContext_->RA.GetRetryTimer(TestChangeSetMembershipAction::SetName);
}

void TestChangeSetMembershipAction::AddToSetAddsItemToSet()
{
    Setup();

    auto action = CreateAction(true);
    action->OnPerformAction(DefaultActivityId, entry_, testContext_->RA);

    Verify::Vector(GetSet().GetEntities(), GetEntity());
}

void TestChangeSetMembershipAction::RemoveFromSetRemovesFromSet()
{
    Setup();

    GetSet().AddEntity(GetEntity());
        
    auto action = CreateAction(false);
    action->OnPerformAction(DefaultActivityId, entry_, testContext_->RA);

    Verify::Vector(GetSet().GetEntities());
}

void TestChangeSetMembershipAction::AddSetsTheRetryTimer()
{
    Setup();

    auto action = CreateAction(true);
    action->OnPerformAction(DefaultActivityId, entry_, testContext_->RA);

    VERIFY_IS_TRUE(GetRetryTimer().IsSet);
}

void TestChangeSetMembershipAction::RemoveDoesNotUnsetTheRetryTimer()
{
    Setup();

    GetRetryTimer().Set();

    auto action = CreateAction(false);
    action->OnPerformAction(DefaultActivityId, entry_, testContext_->RA);

    VERIFY_IS_TRUE(GetRetryTimer().IsSet);
}

void TestChangeSetMembershipAction::RemoveDoesNotSetTheRetryTimer()
{
    Setup();

    auto action = CreateAction(false);
    action->OnPerformAction(DefaultActivityId, entry_, testContext_->RA);

    VERIFY_IS_FALSE(GetRetryTimer().IsSet);
}

BOOST_FIXTURE_TEST_SUITE(TestChangeSetMembershipActionSuite, TestChangeSetMembershipAction)

BOOST_AUTO_TEST_CASE(AddToSetAddsItemToSet)
{
	AddToSetAddsItemToSet();
}
BOOST_AUTO_TEST_CASE(AddSetsTheRetryTimer)
{
	AddSetsTheRetryTimer();
}
BOOST_AUTO_TEST_CASE(RemoveFromSetRemovesFromSet)
{
	RemoveFromSetRemovesFromSet();
}
BOOST_AUTO_TEST_CASE(RemoveDoesNotUnsetTheRetryTimer)
{
	RemoveDoesNotUnsetTheRetryTimer();
}
BOOST_AUTO_TEST_CASE(RemoveDoesNotSetTheRetryTimer)
{
	RemoveDoesNotSetTheRetryTimer();
}

BOOST_AUTO_TEST_SUITE_END()
