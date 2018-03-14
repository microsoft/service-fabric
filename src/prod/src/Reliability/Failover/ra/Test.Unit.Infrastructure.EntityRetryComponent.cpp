// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


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
    const int MinimumIntervalBetweenRetry = 3;
}

typedef EntityRetryComponent<int>::RetryContext RetryContext;

class MyComponent : public EntityRetryComponent<int>
{
public:
    MyComponent(TimeSpanConfigEntry const & minimumInterval) :
        EntityRetryComponent<int>(make_shared<Infrastructure::LinearRetryPolicy>(minimumInterval)),
        GeneratedData(0)
    {
    }

    int GeneratedData;

    void SetRetryRequired()
    {
        MarkRetryRequired();
    }

    void SetRetryNotRequired()
    {
        MarkRetryNotRequired();
    }

private:
    int GenerateInternal() const { return GeneratedData; }
};

class TestEntityRetryComponent
{
protected:
    TestEntityRetryComponent() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup) ;
    ~TestEntityRetryComponent() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup) ;

    void MarkRetryRequired()
    {
        entity_->SetRetryRequired();
    }

    void MarkRetryNotRequired()
    {
        entity_->SetRetryNotRequired();
    }

    RetryContext CreateContext(int secondsAfterStart)
    {
        return entity_->CreateRetryContext(GetTime(secondsAfterStart));
    }

    void VerifyRetryRequired(bool flagInitialValue, bool expected)
    {
        bool flag = flagInitialValue;
        entity_->UpdateFlagIfRetryPending(flag);
        Verify::AreEqual(expected, flag, L"VerifyRetryRequired: Flag");
    }

    void VerifyRetryNotRequired()
    {
        VerifyRetryRequired(true, true);
        VerifyRetryRequired(false, false);
    }

    void VerifyRetryRequired()
    {
        VerifyRetryRequired(true, true);
        VerifyRetryRequired(false, true);
    }

    void CreateAndVerifyHasData(int secondsAfterStart, bool expected)
    {
        auto context = CreateContext(secondsAfterStart);
        VerifyHasData(context, expected);
    }

    void VerifyHasData(RetryContext const & context, bool expected)
    {
        Verify::AreEqual(expected, context.HasData, L"hasData");
    }

    void CreateAndVerifyData(int secondsAfterStart)
    {
        int sentinel = 42;
        vector<int> v;
        v.push_back(sentinel);

        auto context = CreateContext(secondsAfterStart);
        context.Generate(v);

        Verify::AreEqual(2, v.size(), L"Item should be added to list");
        Verify::AreEqual(42, v[0], L"Sentinel value");
        Verify::AreEqual(entity_->GeneratedData, v[1], L"Generated");
    }

    void CreateAndRetry(int secondsAfterStart)
    {
        auto context = CreateContext(secondsAfterStart);
        Retry(context, secondsAfterStart);
    }

    void Retry(RetryContext const & context, int retryTime)
    {
        entity_->OnRetry(context, GetTime(retryTime));
    }

    StopwatchTime GetTime(int secondsAfterStart)
    {
        static const StopwatchTime Start = Stopwatch::Now();

        return Start + TimeSpan::FromSeconds(secondsAfterStart);
    }

    void SetData(int data)
    {
        entity_->GeneratedData = data;
    }

    unique_ptr<MyComponent> entity_;
    ScenarioTestHolderUPtr holder_;
};

bool TestEntityRetryComponent::TestSetup()
{
	holder_ = ScenarioTestHolder::Create();

	holder_->ScenarioTestObj.UTContext.Config.FMMessageRetryInterval = TimeSpan::FromSeconds(MinimumIntervalBetweenRetry);
	entity_ = make_unique<MyComponent>(holder_->ScenarioTestObj.UTContext.Config.FMMessageRetryIntervalEntry);

	return true;
}

bool TestEntityRetryComponent::TestCleanup()
{
	holder_.reset();
	return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestEntityRetryComponentSuite,TestEntityRetryComponent)

BOOST_AUTO_TEST_CASE(RetryPending_Initial_False)
{
    VerifyRetryNotRequired();
}

BOOST_AUTO_TEST_CASE(HasData_Initial_False)
{
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, false);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, false);
}

BOOST_AUTO_TEST_CASE(RetryPending_AfterRequest_True)
{
    MarkRetryRequired();
    VerifyRetryRequired();
}

BOOST_AUTO_TEST_CASE(HasData_AfterRequest_True)
{
    MarkRetryRequired();
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, true);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, true);
}

BOOST_AUTO_TEST_CASE(AddData_AfterRequest_AfterInterval_GeneratesData)
{
    MarkRetryRequired();
    CreateAndVerifyData(MinimumIntervalBetweenRetry + 1);
}

BOOST_AUTO_TEST_CASE(HasData_AfterRetry_WithinInterval_False)
{
    int retryTime = 1;
    MarkRetryRequired();
    
    CreateAndRetry(retryTime);

    CreateAndVerifyHasData(retryTime + MinimumIntervalBetweenRetry - 1, false);
}

BOOST_AUTO_TEST_CASE(HasData_AfterRetry_AfterInterval_True)
{
    int retryTime = 1;
    MarkRetryRequired();
    
    CreateAndRetry(retryTime);

    CreateAndVerifyHasData(retryTime + MinimumIntervalBetweenRetry + 1, true);
}

BOOST_AUTO_TEST_CASE(HasData_AfterRequestAfterRetry_True)
{
    MarkRetryRequired();
    CreateAndRetry(0);

    MarkRetryRequired();
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, true);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, true);
}

BOOST_AUTO_TEST_CASE(HasData_StaleRetry_True)
{
    MarkRetryRequired();

    auto context = CreateContext(0);

    MarkRetryRequired();

    // Retry happened for earlier context which is now stale
    Retry(context, MinimumIntervalBetweenRetry + 1);

    // Retry must happen immediately
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, true);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, true);
}

BOOST_AUTO_TEST_CASE(HasData_AfterReply_False)
{
    MarkRetryRequired();
    MarkRetryNotRequired();

    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, false);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, false);
}

BOOST_AUTO_TEST_CASE(RetryPending_AfterReply_False)
{
    MarkRetryRequired();
    MarkRetryNotRequired();

    VerifyRetryNotRequired();
}

BOOST_AUTO_TEST_CASE(HasData_AfterRequestAfterReplyAfterCompletion_True)
{
    MarkRetryRequired();

    auto context = CreateContext(0);

    MarkRetryNotRequired();

    Retry(context, 0);

    MarkRetryRequired();
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry - 1, true);
    CreateAndVerifyHasData(MinimumIntervalBetweenRetry + 1, true);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
