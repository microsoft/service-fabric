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

using namespace Infrastructure;

class TestMultipleEntityWorkManager
{
protected:
    TestMultipleEntityWorkManager()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestMultipleEntityWorkManager()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

protected:
    MultipleEntityWorkSPtr CreateWork(bool hasCallback)
    {
        if (hasCallback)
        {
            return make_shared<MultipleEntityWork>(L"a", [&](MultipleEntityWork&, ReconfigurationAgent &) { callbackCount_++; });
        }
        else
        {
            return make_shared<MultipleEntityWork>(L"a");
        }
    }

    void VerifyCallback()
    {
        Verify::AreEqual(1, callbackCount_, L"Callback count");
    }

    void VerifyNoCallback()
    {
        Verify::AreEqual(0, callbackCount_, L"Callback count");
    }

    void Execute(MultipleEntityWorkSPtr const & work)
    {
        workManager_->Execute(work);
    }

    void Drain()
    {
        utContext_->ThreadpoolStubObj.DrainAll(utContext_->RA);
    }

    UnitTestContextUPtr utContext_;
    unique_ptr<MultipleEntityWorkManager> workManager_;
    int callbackCount_;
};

bool TestMultipleEntityWorkManager::TestSetup()
{
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled | UnitTestContext::Option::StubJobQueueManager);
    workManager_ = make_unique<MultipleEntityWorkManager>(utContext_->RA);
    callbackCount_ = 0;
    return true;
}

bool TestMultipleEntityWorkManager::TestCleanup()
{
    workManager_.reset();
    return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestMultipleEntityWorkManagerSuite, TestMultipleEntityWorkManager)

BOOST_AUTO_TEST_CASE(WorkWithNoJobItemsAndNoCallbackCompletes)
{
    auto work = CreateWork(false);

    Execute(work);

    Drain();

    VerifyNoCallback();
}

BOOST_AUTO_TEST_CASE(WorkWithJobItemsAndNoCallbackCompletes)
{
    auto work = CreateWork(true);

    Execute(work);

    Drain();

    VerifyCallback();
}

BOOST_AUTO_TEST_CASE(WorkWithJobItemsCompletesAfterJobItemsComplete)
{
    auto work = CreateWork(true);

    auto entity = EntityTraits<TestEntity>::Create(L"a", utContext_->RA);

    bool wasInvoked = false;
    auto ji = make_shared<TestJobItem>(
        TestJobItem::Parameters(
            entity,
            work,
            [&](TestEntityHandlerParameters&, TestJobItemContext &) { wasInvoked = true; return true; },
            JobItemCheck::None,
            *JobItemDescription::AppHostClosed));

    work->AddFailoverUnitJob(ji);

    Execute(work);

    VerifyNoCallback();

    Drain();

    VerifyCallback();

    Verify::IsTrue(wasInvoked, L"was ji invoked");

}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
