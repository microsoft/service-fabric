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
using namespace Reliability::ReconfigurationAgentComponent::Infrastructure::ReliabilityUnitTest;

namespace
{
    const int DefaultPersistedState = 1;
    const int DefaultInMemoryState = 2;

    const JobItemDescription DefaultTraceParameters(JobItemName::Test, JobItemTraceFrequency::Always, false);
    const JobItemDescription DefaultTraceParametersWithFailure(JobItemName::Test, JobItemTraceFrequency::Always, true);

    class StateMachineActionStub : public StateMachineAction
    {
    public:
        typedef function<void (EntityEntryBaseSPtr const & entry)> OnPerformActionFunctionPtr;
        typedef function<void ()> OnCancelActionFunctionPtr;

        OnPerformActionFunctionPtr OnPerformActionStub;
        OnCancelActionFunctionPtr OnCancelActionStub;

        void OnPerformAction(wstring const & activityId, EntityEntryBaseSPtr const & entry, ReconfigurationAgent & ) override
        {
            ASSERT_IF(activityId.empty(), "activity id cant be empty to action stub");
            if (OnPerformActionStub != nullptr)
            {
                OnPerformActionStub(entry);
            }
        }

        void OnCancelAction(ReconfigurationAgent&) override
        {
            if (OnCancelActionStub != nullptr)
            {
                OnCancelActionStub();
            }
        }
    };

    typedef std::function<bool(TestEntityHandlerParameters &, TestJobItemContext &)> ProcessorFunc;
}

class EntityJobItemTest
{
protected:
    EntityJobItemTest()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~EntityJobItemTest()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void SetupInserted();
    TestJobItemSPtr CreateJobItem(ProcessorFunc func);
    TestJobItemSPtr CreateJobItem(ProcessorFunc func, JobItemCheck::Enum check);
    TestJobItemSPtr CreateJobItem(ProcessorFunc func, MultipleEntityWorkSPtr const & work);
    TestJobItemSPtr CreateJobItemWithDescription(ProcessorFunc func, JobItemDescription const & description);

    bool CreateAndExecuteJobItem(ProcessorFunc func);
    bool CreateAndExecuteJobItem(ProcessorFunc func, PersistenceResult::Enum persistenceResult);

    bool ExecuteJobItem(TestJobItemSPtr const & ji);
    bool ExecuteJobItem(TestJobItemSPtr const & ji, PersistenceResult::Enum persistenceResult);

    void EnqueueActionStub(
        TestEntityHandlerParameters & handlerParameters,
        StateMachineActionStub::OnPerformActionFunctionPtr performFunc,
        StateMachineActionStub::OnCancelActionFunctionPtr cancelFunc);

    void JobItemFlowTestHelper(bool callEnableUpdate, bool returnValue, bool expectedShouldCommit);

    void CreateAndExecuteJobItemAndValidateWorkIsCompleted(
        function<void()> setup,
        ProcessorFunc func, 
        PersistenceResult::Enum persistenceResult);

    void CreateAndExecuteJobItemWithWork(
        function<void()> setup,
        ProcessorFunc func,
        function<void()> workCallback,
        PersistenceResult::Enum persistenceResult);

    void PostProcessWithCommitFailureTestHelper(
        JobItemDescription const & description,
        PersistenceResult::Enum persistenceResult,
        bool expectApiCall);

    void ScheduleJobItem(TestJobItemSPtr const & ji);

    void Drain();

    void JobItemCheckHelper(
        JobItemCheck::Enum check,
        RALifeCycleState::Enum raState,
        bool expected);

    UnitTestContextUPtr utContext_;
    InfrastructureTestUtilityUPtr utility_;
    EntityEntryBaseSPtr entry_;
};

bool EntityJobItemTest::TestSetup()
{
    utContext_ = InfrastructureTestUtility::CreateUnitTestContextWithTestEntityMap();
    utility_ = make_unique<InfrastructureTestUtility>(*utContext_);
    return true;
}

bool EntityJobItemTest::TestCleanup()
{
    utContext_->Cleanup();
    return true;
}

void EntityJobItemTest::SetupInserted()
{
    entry_ = utility_->SetupInserted(DefaultPersistedState, DefaultInMemoryState);
}

TestJobItemSPtr EntityJobItemTest::CreateJobItem(ProcessorFunc func)
{
    return CreateJobItemWithDescription(func, DefaultTraceParameters);
}

TestJobItemSPtr EntityJobItemTest::CreateJobItemWithDescription(ProcessorFunc func, JobItemDescription const & description)
{
    TestJobItem::Parameters parameters(
        entry_,
        L"aid",
        func,
        JobItemCheck::None,
        description);

    return make_shared<TestJobItem>(move(parameters));
}

TestJobItemSPtr EntityJobItemTest::CreateJobItem(ProcessorFunc func, MultipleEntityWorkSPtr const & work)
{
    TestJobItem::Parameters parameters(
        entry_,
        work,
        func,
        JobItemCheck::None,
        DefaultTraceParameters);

    return make_shared<TestJobItem>(move(parameters));
}

TestJobItemSPtr EntityJobItemTest::CreateJobItem(ProcessorFunc func, JobItemCheck::Enum check)
{
    TestJobItem::Parameters parameters(
        entry_,
        L"aid",
        func,
        check,
        DefaultTraceParameters);

    return make_shared<TestJobItem>(move(parameters));
}

void EntityJobItemTest::ScheduleJobItem(TestJobItemSPtr const & ji)
{
    utContext_->RA.JobQueueManager.ScheduleJobItem(ji);
}

void EntityJobItemTest::Drain()
{
    utContext_->DrainAllQueues();
}

bool EntityJobItemTest::ExecuteJobItem(TestJobItemSPtr const & ji, PersistenceResult::Enum persistenceResult)
{
    auto & store = utContext_->FaultInjectedLfumStore;
    store.IsAsync = true;

    PersistenceResult::Set(persistenceResult, store);
    
    ScheduleJobItem(ji);

    Drain();

    auto committed = store.FinishPendingCommits();
    Drain();

    return committed != 0;
}

bool EntityJobItemTest::ExecuteJobItem(TestJobItemSPtr const & ji)
{
    return ExecuteJobItem(ji, PersistenceResult::Success);
}

bool EntityJobItemTest::CreateAndExecuteJobItem(ProcessorFunc func)
{
    return CreateAndExecuteJobItem(func, PersistenceResult::Success);
}

bool EntityJobItemTest::CreateAndExecuteJobItem(ProcessorFunc func, PersistenceResult::Enum persistenceResult)
{
    SetupInserted();

    auto ji = CreateJobItem(func);

    return ExecuteJobItem(ji, persistenceResult);
}

void EntityJobItemTest::CreateAndExecuteJobItemWithWork(
    function<void()> setup,
    ProcessorFunc func,
    function<void()> workCallback,
    PersistenceResult::Enum)
{
    auto work = make_shared<MultipleEntityWork>(L"aid", [&](MultipleEntityWork const &, ReconfigurationAgent &)
    {
        workCallback();
    });

    setup();

    auto ji = CreateJobItem(func, work);
    work->AddFailoverUnitJob(move(ji));

    utContext_->RA.JobQueueManager.ExecuteMultipleFailoverUnitWork(move(work));

    Drain();
}

void EntityJobItemTest::CreateAndExecuteJobItemAndValidateWorkIsCompleted(
    function<void()> setup,
    ProcessorFunc func,
    PersistenceResult::Enum persistenceResult)
{
    bool callbackCalled = false;

    CreateAndExecuteJobItemWithWork(
        setup,
        func,
        [&]() { callbackCalled = true; },
        persistenceResult);

    Verify::IsTrue(callbackCalled, L"Work must be completed");
}

void EntityJobItemTest::EnqueueActionStub(
    TestEntityHandlerParameters & handlerParameters,
    StateMachineActionStub::OnPerformActionFunctionPtr performFunc,
    StateMachineActionStub::OnCancelActionFunctionPtr cancelFunc)
{
    auto action = make_unique<StateMachineActionStub>();
    action->OnPerformActionStub = performFunc;
    action->OnCancelActionStub = cancelFunc;
    handlerParameters.ActionQueue.Enqueue(move(action));
}

void EntityJobItemTest::JobItemFlowTestHelper(bool callEnableUpdate, bool returnValue, bool expectedShouldCommit)
{
    bool performActionCalled = false;

    bool didCommit = CreateAndExecuteJobItem([&](TestEntityHandlerParameters & handlerParameters, TestJobItemContext &)
    {
        if (callEnableUpdate) { handlerParameters.Entity.EnableUpdate(); }

        EnqueueActionStub(
            handlerParameters,
            [&](EntityEntryBaseSPtr const &) { performActionCalled = true; },
            [&]() { performActionCalled = true; });
        return returnValue;
    });

    Verify::IsTrue(performActionCalled, L"PerformActionCalled");
    Verify::AreEqual(expectedShouldCommit, didCommit, L"did commit");
}

void EntityJobItemTest::PostProcessWithCommitFailureTestHelper(
    JobItemDescription const & description,
    PersistenceResult::Enum persistenceResult,
    bool expectApiCall)
{
    SetupInserted();

    auto ji = CreateJobItemWithDescription(
        [](TestEntityHandlerParameters & handlerParameters, TestJobItemContext &) { handlerParameters.Entity.EnableUpdate(); return true; }, 
        description);

    ExecuteJobItem(ji, persistenceResult);

    if (expectApiCall)
    {
        utContext_->ProcessTerminationServiceStubObj.Calls.VerifyCallCount(1);
    }
    else
    {
        utContext_->ProcessTerminationServiceStubObj.Calls.VerifyNoCalls();
    }
}

void EntityJobItemTest::JobItemCheckHelper(
    JobItemCheck::Enum check,
    RALifeCycleState::Enum raState,
    bool expected)
{
    bool called = false;

    SetupInserted();

    auto ji = CreateJobItem([&](TestEntityHandlerParameters &, TestJobItemContext &)
        {
            called = true;
            return true;
        },
        check);

    ScheduleJobItem(ji);

    RALifeCycleStateHelper helper(utContext_->RA);
    helper.SetState(raState);

    Drain();

    Verify::AreEqual(expected, called, L"WasCalled");
}


BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestEntityJobItemSuite, EntityJobItemTest)

BOOST_AUTO_TEST_CASE(Flow_NoUpdate_PerformsPostProcess)
{
    JobItemFlowTestHelper(false, true, false);
}

BOOST_AUTO_TEST_CASE(Flow_Update_PerformsPostProcess)
{
    JobItemFlowTestHelper(true, true, true);
}

BOOST_AUTO_TEST_CASE(Flow_Update_ReturnFalse_PerformsPostProcess)
{
    JobItemFlowTestHelper(true, false, true);
}

BOOST_AUTO_TEST_CASE(Flow_JobItemThatDeletes)
{
    CreateAndExecuteJobItem([&](TestEntityHandlerParameters & handlerParameters, TestJobItemContext &)
        {
            handlerParameters.Entity.MarkForDelete();
            return true;
        });

    utility_->ValidateEntityIsDeleted(entry_);
}

BOOST_AUTO_TEST_CASE(Flow_DeletedEntity_DoesNotExecuteHandler)
{
    bool called = false;

    entry_ = utility_->SetupDeleted();

    auto ji = CreateJobItem([&](TestEntityHandlerParameters &, TestJobItemContext &)
        {
            called = true;
            return true;
        });

    ExecuteJobItem(ji);

    Verify::IsTrue(!called, L"should not execute job");
}

BOOST_AUTO_TEST_CASE(Flow_NoUpdate_DoesNotTrace)
{
    SetupInserted();

    const JobItemDescription traceOnProcessDescription(JobItemName::Test, JobItemTraceFrequency::OnSuccessfulProcess, false);

    auto ji = CreateJobItemWithDescription([&](TestEntityHandlerParameters &, TestJobItemContext &)
                                            {
                                                return false;
                                            },
                                            traceOnProcessDescription);

    ExecuteJobItem(ji);

    vector<EntityJobItemTraceInformation> traces;
    ji->Trace("id", traces, utContext_->RA);

    Verify::IsTrue(traces.empty(), L"should not trace");
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpen_Open_IsCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpen, RALifeCycleState::Open, true);
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpen_Closing_IsNotCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpen, RALifeCycleState::Closing, false);
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpen_Closed_IsNotCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpen, RALifeCycleState::Closed, false);
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpenOrClosing_Open_IsCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpenOrClosing, RALifeCycleState::Open, true);
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpenOrClosing_Closing_IsNotCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpenOrClosing, RALifeCycleState::Closing, true);
}

BOOST_AUTO_TEST_CASE(JobItemCheck_RAIsOpenOrClosing_Closed_IsNotCalled)
{
    JobItemCheckHelper(JobItemCheck::RAIsOpenOrClosing, RALifeCycleState::Closed, false);
}

BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsTerminatedOnCommitFailure)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParametersWithFailure, PersistenceResult::Failure, true);
}

BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsNotTerminatedOnStoreClosed)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParametersWithFailure, PersistenceResult::RAStoreNotUsable, false);
}

// Specifically for the V2 local store based on the native replicator stack
BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsNotTerminatedOnNotPrimary)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParametersWithFailure, PersistenceResult::NotPrimary, false);
}

BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsNotTerminatedOnObjectClosed)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParametersWithFailure, PersistenceResult::ObjectClosed, false);
}

BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsNotTerminatedTerminatedOnCommitSuccess)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParametersWithFailure, PersistenceResult::Success, false);
}

BOOST_AUTO_TEST_CASE(PostProcess_ProcessIsNotTerminatedOnCommitFailure_IfNotSpecified)
{
    PostProcessWithCommitFailureTestHelper(DefaultTraceParameters, PersistenceResult::Failure, false);
}

BOOST_AUTO_TEST_CASE(PostProcess_UpdateFailure_CancelsActionQueue)
{
    bool cancelActionCalled = false;

    CreateAndExecuteJobItem([&](TestEntityHandlerParameters & handlerParameters, TestJobItemContext & )
                            {
                                handlerParameters.Entity.EnableUpdate();
                                EnqueueActionStub(
                                    handlerParameters,
                                    nullptr,
                                    [&]() { cancelActionCalled = true; });
                                return true;
                            },
                            PersistenceResult::Failure);

    Verify::IsTrue(cancelActionCalled, L"CancelActionCalled");
}

BOOST_AUTO_TEST_CASE(PostProcess_EntityIsLockedWhileQueueIsExecuting)
{
    bool isLocked = false;

    CreateAndExecuteJobItem([&](TestEntityHandlerParameters & handlerParameters, TestJobItemContext & )
        {        
            EnqueueActionStub(
                handlerParameters,
                [&](EntityEntryBaseSPtr const & entry) { isLocked = entry->As<TestEntityEntry>().State.Test_IsWriteLockHeld; },
                nullptr);
            return true;
        });

    Verify::IsTrue(isLocked, L"IsWriteLockHeld");
}

BOOST_AUTO_TEST_CASE(PostProcess_IfEntityReturnedFalseFromProcessorThenQueueIsDropped)
{
    bool isLocked = false;

    CreateAndExecuteJobItem([&](TestEntityHandlerParameters & handlerParameters, TestJobItemContext &)
        {
            EnqueueActionStub(
                handlerParameters,
                [&](EntityEntryBaseSPtr const & entry) { isLocked = entry->As<TestEntityEntry>().State.Test_IsWriteLockHeld; },
                nullptr);
            return true;
        });

    Verify::IsTrue(isLocked, L"IsWriteLockHeld");
}

BOOST_AUTO_TEST_CASE(WorkIsCompleted_SimpleJobItem)
{
    CreateAndExecuteJobItemAndValidateWorkIsCompleted(
        [&]() { SetupInserted(); },
        [&](TestEntityHandlerParameters &, TestJobItemContext &) { return true; },
        PersistenceResult::Success);
}

BOOST_AUTO_TEST_CASE(WorkIsCompleted_CommitFailure)
{
    CreateAndExecuteJobItemAndValidateWorkIsCompleted(
        [&]() { SetupInserted(); },
        [&](TestEntityHandlerParameters & parameters, TestJobItemContext &)
        {
            parameters.Entity.EnableUpdate();
            return true;
        },
        PersistenceResult::Failure);
}

BOOST_AUTO_TEST_CASE(WorkIsCompleted_DeletedEntity)
{
    CreateAndExecuteJobItemAndValidateWorkIsCompleted(
        [&]() { entry_ = utility_->SetupDeleted(); },
        [&](TestEntityHandlerParameters &, TestJobItemContext &)
        {
            return true;
        },
        PersistenceResult::Success);    
}

BOOST_AUTO_TEST_CASE(WorkIsCompleted_AfterLockIsReleased)
{
    bool isLocked = false;
    CreateAndExecuteJobItemWithWork(
        [&]() { SetupInserted(); },
        [&](TestEntityHandlerParameters & , TestJobItemContext &)
        {
            return true;
        },
        [&]() { isLocked = entry_->As<TestEntityEntry>().State.Test_IsWriteLockHeld; },
        PersistenceResult::Success);

    Verify::IsTrue(!isLocked, L"Cannot be locked when work callback is executing");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
