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

typedef pair<function<void()>, wstring> Transition;

class UnitTest_FMMessageState : public TestBase
{
protected:
    UnitTest_FMMessageState() 
    {
        EndpointUpdateTransition = make_pair([&]() {MarkEndpointUpdatePending(); }, L"MarkEndpoint");
        ResetTransition = make_pair([&]() {Reset(); }, L"Reset");
        OnDroppedTransition = make_pair([&]() {OnDropped(); }, L"Dropped");
        OnClosingTransition = make_pair([&]() {OnClosing(); }, L"Closing");
        OnDeletedTransition = make_pair([&]() {OnDeleted(); }, L"Deleted");
        OnUpTransition = make_pair([&]() {OnUp(); }, L"Up");
        OnPersistedReplicaDownTransition = make_pair([&]() {OnPersistedReplicaDown(1); }, L"PersistedDown");        
        OnVolatileReplicaDownTransition = make_pair([&]() {OnVolatileReplicaDown(1); }, L"VolatileDown");
        OnLfumLoadTransition = make_pair([&]() {OnLfumLoad(1); }, L"LfumLoad");
        OnReplicaUpReplyTransition = make_pair([&]() {OnUpReply(); }, L"OnUpReply");
        OnReplicaDownReplyTransition = make_pair([&]() {OnDownReply(1); }, L"DownReply");
        OnReplicaDroppedReplyTransition = make_pair([&]() {OnDroppedReply(); }, L"Droppedreply");
        OnLastReplicaUpPendingTransition = make_pair([&]() {OnLastReplicaUpPending(); }, L"OnLastReplicaUpPending");
        OnLastReplicaUpAcknowledgedTransition = make_pair([&]() {OnLastReplicaUpAcknowledged(); }, L"OnLastReplicaUpAcknowledged");

        BOOST_REQUIRE(TestSetup());
    }

    ~UnitTest_FMMessageState()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void VerifyMessageStageAndUpdate(FMMessageStage::Enum expected, bool retrySequenceChangeExpected = true)
    {
        VerifyMessageStage(expected);

        VerifyInMemoryCommit();

        VerifyFMMessageRetryActionEnqueued(expected != FMMessageStage::None);

        VerifyRetryStateSequenceNumberIsDifferent(retrySequenceChangeExpected);

        VerifyRetryStateConsistency();
    }

    void VerifyRetryStateConsistency()
    {
        bool shouldBeSet = holder_->FMMessageStateObj.FMMessageRetryPending.IsSet;
        Verify::AreEqual(shouldBeSet, GetSequenceNumber() != 0, wformatString("SequenceNumber is inconsistent {0}. {1}", shouldBeSet, GetSequenceNumber()));
    }

    void VerifyRetryStateSequenceNumberIsDifferent(bool expected)
    {
        int64 current = GetSequenceNumber();

        Verify::AreEqual(expected, current != sequenceNumber_, wformatString("Sequence number did not change. INit {0}. Current: {1}", sequenceNumber_, current));
    }    

    void VerifyMessageStage(FMMessageStage::Enum expected)
    {
        // Verify the enum
        Verify::AreEqual(expected, holder_->FMMessageStateObj.MessageStage, L"MessageStage");

        // Verify the flag value
        bool flagValue = expected != FMMessageStage::None;
        auto actual = holder_->FMMessageStateObj.FMMessageRetryPending.IsSet;
        Verify::AreEqualLogOnError(flagValue, actual, wformatString("Flag verification for failed MessageStage: {0}. Actual Value: {1}", expected, actual));
    }

    void VerifyFMMessageRetryActionEnqueued(bool expected)
    {
        // Verify that the request work action gets enqueued
        bool found = false;
        for (auto const & it : updateContextQueue_->ActionQueue)
        {
            auto casted = dynamic_cast<ReconfigurationAgentComponent::Communication::RequestFMMessageRetryAction*>(it.get());
            if (casted != nullptr)
            {
                found = true;
                break;
            }
        }

        Verify::AreEqual(expected, found, wformatString("Did not find a match for message retry action"));
    }

    void VerifyShouldRetry(bool expected, StopwatchTime now)
    {
        int64 sequenceNumber = 0;
        auto actual = holder_->FMMessageStateObj.ShouldRetry(now, sequenceNumber);
        Verify::AreEqual(expected, actual, L"Should retry");
    }

    void OnRetry(Common::StopwatchTime now, int64 sequenceNumber)
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnRetry(now, sequenceNumber, context);
    }

    void MarkEndpointUpdatePending()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.MarkReplicaEndpointUpdatePending(context);
    }

    void OnDropped()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnDropped(context);
    }

    void Reset()
    {
        holder_->LocalReplicaDeleted = false;
        auto context = CreateContext();
        holder_->FMMessageStateObj.Reset(context);
    }

    void OnDeleted()
    {
        holder_->LocalReplicaDeleted = true;
        auto context = CreateContext();
        holder_->FMMessageStateObj.Reset(context);
    }

    void OnClosing()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnClosing(context);
    }

    void OnUp()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaUp(context);
    }

    void OnPersistedReplicaDown(int instance)
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaDown(true, instance, context);
    }

    void OnVolatileReplicaDown(int instance)
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaDown(false, instance, context);
    }

    void OnLfumLoad(int instance)
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnLoadedFromLfum(instance, context);
    }

    void OnUpReply()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaUpAcknowledged(context);
    }

    void OnDownReply(int instance)
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaDownReply(instance, context);
    }

    void OnDroppedReply()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnReplicaDroppedReply(context);
    }

    void OnLastReplicaUpPending()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnLastReplicaUpPending(context);
    }

    void OnLastReplicaUpAcknowledged()
    {
        auto context = CreateContext();
        holder_->FMMessageStateObj.OnLastReplicaUpAcknowledged(context);
    }

    void VerifyInstance(int expected)
    {
        Verify::AreEqual(expected, GetInstance(), L"Instance");
    }

    void VerifyInstanceIsNotSet()
    {
        Verify::AreEqual(-1, holder_->FMMessageStateObj.Test_GetDownReplicaInstanceId(), L"Instance");
    }

    int64 GetInstance()
    {
        return holder_->FMMessageStateObj.DownReplicaInstanceId;
    }

    void VerifyNoOpTest(
        function<void()> init,
        function<void()> transition,
        FMMessageStage::Enum expected,
        wstring const & tag)
    {
        TestSetup();

        TestLog::WriteInfo(L"Running: " + tag);

        init();

        transition();

        VerifyMessageStage(expected);

        VerifyNoCommit();

        VerifyRetryStateSequenceNumberIsDifferent(false);

        VerifyRetryStateConsistency();
    }

    void VerifyNoOpTest(
        initializer_list<pair<function<void()>, wstring>> transitions,
        function<void()> init,
        FMMessageStage::Enum expected)
    {
        for (auto it : transitions)
        {
            VerifyNoOpTest(init, it.first, expected, it.second);
        }
    }

    void VerifyAssertsTest(
        function<void()> init,
        function<void()> transition,
        wstring const & tag)
    {
        TestSetup();

        TestLog::WriteInfo(L"Running: " + tag);

        init();

        Verify::Asserts(transition, tag);
    }

    void VerifyAssertsTest(
        initializer_list<pair<function<void()>, wstring>> transitions,
        function<void()> init)
    {
        for (auto it : transitions)
        {
            VerifyAssertsTest(init, it.first, it.second);
        }
    }

    Infrastructure::StateUpdateContext CreateContext() override
    {
        sequenceNumber_ = GetSequenceNumber();
        return TestBase::CreateContext();
    }

    int64 GetSequenceNumber() const
    {
        int64 sequenceNumber = 0;
        holder_->FMMessageStateObj.ShouldRetry(Stopwatch::Now(), sequenceNumber);
        return sequenceNumber;
    }

    unique_ptr<FMMessageStateHolder> holder_;

    Transition EndpointUpdateTransition;
    Transition ResetTransition;
    Transition OnDroppedTransition;
    Transition OnClosingTransition;
    Transition OnDeletedTransition;
    Transition OnUpTransition;
    Transition OnPersistedReplicaDownTransition;
    Transition OnVolatileReplicaDownTransition;
    Transition OnLfumLoadTransition;
    Transition OnReplicaUpReplyTransition;
    Transition OnReplicaDownReplyTransition;
    Transition OnReplicaDroppedReplyTransition;
    Transition OnLastReplicaUpPendingTransition;
    Transition OnLastReplicaUpAcknowledgedTransition;

    int64 sequenceNumber_;
};

bool UnitTest_FMMessageState::TestSetup()
{
    sequenceNumber_ = 0;
    holder_ = make_unique<FMMessageStateHolder>();
    return true;
}

bool UnitTest_FMMessageState::TestCleanup()
{
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(UnitTest_FMMessageStateSuite, UnitTest_FMMessageState)

BOOST_AUTO_TEST_CASE(InitiallyNothingIsPending)
{
    VerifyMessageStage(FMMessageStage::None);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(None_NoOpTransitions)
{
    VerifyNoOpTest(
        {
            ResetTransition,
            OnClosingTransition,
            OnDeletedTransition,

            OnReplicaUpReplyTransition,
            OnReplicaDownReplyTransition,
            OnReplicaDroppedReplyTransition,

            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {},
        FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(None_Asserts)
{
    VerifyAssertsTest(
        {
            OnUpTransition,        
        },
        [&]() {});
}

BOOST_AUTO_TEST_CASE(None_EndpointUpdatedMarksEndpointUpdatePending)
{
    // Replica gets created and reconfig starts and reconfig is slow
    MarkEndpointUpdatePending();

    VerifyMessageStageAndUpdate(FMMessageStage::EndpointAvailable);
}

BOOST_AUTO_TEST_CASE(None_DroppedMarksReplicaDropped)
{
    // Simple dropped scenario
    OnDropped();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDropped);
}

BOOST_AUTO_TEST_CASE(None_PersistedReplicaDownMarksReplicaAsDown)
{
    OnPersistedReplicaDown(1);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(None_VolatileReplicaDownMarksReplicaAsDown)
{
    OnVolatileReplicaDown(1);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(None_LfumLoadMarksReplicaInstance)
{
    OnLfumLoad(1);

    VerifyMessageStageAndUpdate(FMMessageStage::None, false);
    
    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(None_LastReplicaUpPendingTransitionsToLRUPending)
{
    OnLastReplicaUpPending();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUpload);
}

BOOST_AUTO_TEST_CASE(Endpoint_NoOpTests)
{
    VerifyNoOpTest(
        {
            // TODO: In the future this can be an assert
            // for now the state machine handles it
            EndpointUpdateTransition,

            // Let this class itself handle the message
            OnReplicaUpReplyTransition,
            OnReplicaDownReplyTransition,
            OnReplicaDroppedReplyTransition,
        },
        [&]() { MarkEndpointUpdatePending(); },
        FMMessageStage::EndpointAvailable);
}

BOOST_AUTO_TEST_CASE(Endpoint_Asserts)
{
    VerifyAssertsTest(
        {
            // Following transitions cannot happen without reset (during close)
            OnDroppedTransition,
            OnPersistedReplicaDownTransition,
            OnVolatileReplicaDownTransition,

            // Must be down to go up
            OnUpTransition,

            // LFUM upload can happen only once and initially
            OnLfumLoadTransition,        

            // Replica is transitioning to primary which must happen as a result of fm message
            OnLastReplicaUpPendingTransition,
            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {MarkEndpointUpdatePending(); });
}

BOOST_AUTO_TEST_CASE(EndpointUpdate_Reset)
{
    // Example: Reconfig completes
    MarkEndpointUpdatePending();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(EndpointUpdate_DeleteStarts)
{
    // Example: DeleteStarts
    MarkEndpointUpdatePending();

    OnDeleted();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(EndpointUpdate_OnClosing)
{
    // Example; Replica faults
    MarkEndpointUpdatePending();

    OnClosing();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Dropped_NoOpTests)
{
    VerifyNoOpTest(
        {         
            // Can't receive these messages
            OnReplicaUpReplyTransition,
            OnReplicaDownReplyTransition,
                
            // Replica dropped should be sent
            OnLastReplicaUpPendingTransition,
            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() { OnDropped(); },
        FMMessageStage::ReplicaDropped);
}

BOOST_AUTO_TEST_CASE(Dropped_Asserts)
{
    VerifyAssertsTest(
        {
            EndpointUpdateTransition,

            OnClosingTransition,

            // Can't go down or be dd from dropped
            OnDroppedTransition,
            OnPersistedReplicaDownTransition,
            OnVolatileReplicaDownTransition,

            // Must be down to go up
            OnUpTransition,

            // LFUM upload can happen only once and initially
            OnLfumLoadTransition,
        },
        [&]() {OnDropped(); });
}

BOOST_AUTO_TEST_CASE(Dropped_ResetTransitionsToNone)
{
    OnDropped();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Dropped_DeleteWithResetTransitionsToNone)
{
    OnDropped();

    OnDeleted();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Dropped_DroppedReplyTransitionsToNone)
{
    OnDropped();

    OnDroppedReply();
    
    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Deleted_ResetTransitionsToNone)
{
    OnDeleted();

    Reset();

    VerifyMessageStage(FMMessageStage::None);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(Deleted_NoOpTransitions)
{
    VerifyNoOpTest(
        {
            // FT could have been deleted and then load from lfum
            OnLfumLoadTransition,

            OnDeletedTransition,

            // Delete can be in progress and then host can go down
            // Easier for the fm state to handle that for such cases 
            // No message should be sent
            // example: deleting SV replica with app host down
            // example: queued delete or force delete from replica up reply
            OnClosingTransition,
            OnDroppedTransition,
            OnPersistedReplicaDownTransition,
            OnVolatileReplicaDownTransition,

            OnReplicaUpReplyTransition,
            OnReplicaDownReplyTransition,
            OnReplicaDroppedReplyTransition,

            // It is possible that LRU ack comes in when the ft is marked for delete
            // Consider open FT for which replica upload is pending and a Delete happens
            // (either explicit delete or due to generation update etc)
            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {OnDeleted();  },
        FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Deleted_AssertTransitions)
{
    VerifyAssertsTest(
        {
            EndpointUpdateTransition,

            // Must be down to go up
            OnUpTransition,

            // Can't have lru pending 
            OnLastReplicaUpPendingTransition,
        },
        [&]() {OnDeleted(); });
}

BOOST_AUTO_TEST_CASE(VolatileDown_Assert)
{
    VerifyAssertsTest(
        {
            EndpointUpdateTransition,

            // SV down is dropped
            OnDroppedTransition,
            OnUpTransition,            
            OnLfumLoadTransition,
        },
        [&]() {OnVolatileReplicaDown(1); });
}

BOOST_AUTO_TEST_CASE(VolatileDown_NoOp)
{
    VerifyNoOpTest(
        {
            OnClosingTransition,        
            OnReplicaUpReplyTransition,
            OnReplicaDroppedReplyTransition,

            // Replica down will be sent which will be the discovery message
            OnLastReplicaUpPendingTransition,
            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {OnPersistedReplicaDown(1); },
        FMMessageStage::ReplicaDown);
}

BOOST_AUTO_TEST_CASE(VolatileDown_ReplicaDownReplyClears)
{
    OnVolatileReplicaDown(1);

    OnDownReply(40);

    VerifyInMemoryCommit();

    VerifyMessageStage(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(VolatileDown_ResetClears)
{
    // SV down and then delete
    OnVolatileReplicaDown(1);

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(VolatileDown_OnDeletedClears)
{
    // SV down and then delete
    OnVolatileReplicaDown(1);

    OnDeleted();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Down_PersistedDownWithOldInstanceIsAssert)
{
    OnPersistedReplicaDown(2);

    Verify::Asserts([&]() {OnPersistedReplicaDown(1); }, L"Down with lower instance");
}

BOOST_AUTO_TEST_CASE(Down_PersistedDownWithUpdatedInstanceUpdatesInstance)
{
    // Down, reopen down and then down again 
    OnPersistedReplicaDown(1);

    OnPersistedReplicaDown(2);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstance(2);
}

BOOST_AUTO_TEST_CASE(Down_DroppedClearsInstanceAndTransitionsToDropped)
{
    // Force remove
    OnPersistedReplicaDown(1);

    OnDropped();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDropped);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(Down_StaleDownReply)
{
    OnPersistedReplicaDown(1);

    OnDownReply(0);

    VerifyMessageStage(FMMessageStage::ReplicaDown);

    VerifyNoCommit();

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(Down_HigherDownReply)
{
    OnPersistedReplicaDown(1);

    OnDownReply(2);

    VerifyMessageStage(FMMessageStage::ReplicaDown);

    VerifyNoCommit();

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(Down_DownReplyTransitionsToNoneButMaintainsInstance)
{
    // Standard replica down reply after replica down
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(Down_UpTransitionsToUp)
{
    // Restart completes without down reply
    OnPersistedReplicaDown(1);

    OnUp();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUp);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(Down_Deleted)
{
    // Closed SV gets delete replica
    OnPersistedReplicaDown(1);

    OnDeleted();

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(Down_Reset)
{
    // Closed SV gets create replica
    OnPersistedReplicaDown(1);

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(Down_NoOp)
{
    VerifyNoOpTest(
        {
            OnClosingTransition,
            OnPersistedReplicaDownTransition,
            OnReplicaUpReplyTransition,
            OnReplicaDroppedReplyTransition,

            OnLastReplicaUpPendingTransition,
            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {OnPersistedReplicaDown(1); },
        FMMessageStage::ReplicaDown);
}

BOOST_AUTO_TEST_CASE(Down_Asserts)
{
    VerifyAssertsTest(
        {
            EndpointUpdateTransition,

            OnLfumLoadTransition,
        },
        [&]() {OnPersistedReplicaDown(1); });
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_ResetResetsInstance)
{
    // SP gets dropped with down reply pending. down reply arrives. then new replica
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    OnDropped();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_UpTransitionsToReplicaUp)
{
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    OnUp();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUp);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_DownWithHigherInstanceTransitionsToDownPending)
{
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    OnPersistedReplicaDown(2);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstance(2);
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_Dropped)
{
    // Replica goes down and then force remove
    OnLfumLoad(1);

    OnDropped();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDropped);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_DeleteTransitionsToDeleted)
{
    OnPersistedReplicaDown(1);

    OnDeleted();

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_DownWithSameInstanceIsNoOp)
{
    // ReplicaClose with prereadwritestatus notification
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    OnPersistedReplicaDown(1);

    VerifyMessageStage(FMMessageStage::None);

    VerifyNoCommit();

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_ClosingIsNoOp)
{
    // ReplicaClose with prereadwritestatus notification
    OnPersistedReplicaDown(1);

    OnDownReply(1);

    OnClosing();

    VerifyMessageStage(FMMessageStage::None);

    VerifyNoCommit();

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(DownAcknowledged_Asserts)
{
    VerifyAssertsTest(
        {
            EndpointUpdateTransition,        
        },
        [&]() {OnPersistedReplicaDown(1); OnDownReply(1); });
}

BOOST_AUTO_TEST_CASE(DownAcknowledeged_LRUPending)
{
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUpload);

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(DownAcknowledeged_Noop)
{
    VerifyNoOpTest(
        {
            OnReplicaDownReplyTransition,
            OnReplicaUpReplyTransition,
            OnReplicaDroppedReplyTransition,

            OnLastReplicaUpAcknowledgedTransition,
        },
        [&]() {OnPersistedReplicaDown(1); OnDownReply(1); },
        FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Up_NoOp)
{
    VerifyNoOpTest(
    {
        OnLastReplicaUpPendingTransition,
        OnLastReplicaUpAcknowledgedTransition,
    },
    [&]() {OnPersistedReplicaDown(1); OnUp(); },
    FMMessageStage::ReplicaUp);
}

BOOST_AUTO_TEST_CASE(Up_UpReplyTransitionsToNone)
{
    OnPersistedReplicaDown(10);

    OnUp();

    OnUpReply();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Up_CreateTransitionsToNone)
{
    OnPersistedReplicaDown(10);

    OnUp();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Up_Closing)
{
    // Replica up pending and then starts to close
    // Send message after close completes
    OnPersistedReplicaDown(10);

    OnUp();

    OnClosing();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Up_DownTransitionsToDown)
{
    OnPersistedReplicaDown(10);

    OnUp();

    OnPersistedReplicaDown(3);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstance(3);
}

BOOST_AUTO_TEST_CASE(LRU_Asserts)
{
    VerifyAssertsTest(
    {
        // Shouldn't be called twice
        OnLastReplicaUpPendingTransition,

        // To get an end point update the replica must come up
        EndpointUpdateTransition,

        // Persisted because instance exists
        OnVolatileReplicaDownTransition,

        // if LRU is set without the instance then it is volatile Closed replica
        // it cannot go up
        OnUpTransition,

        OnLfumLoadTransition,

        OnReplicaUpReplyTransition,
        OnReplicaDownReplyTransition,
        OnReplicaDroppedReplyTransition,
    },
    [&]() {OnLastReplicaUpPending(); });
}

BOOST_AUTO_TEST_CASE(LRU_NoOp)
{
    VerifyNoOpTest(
    {
        // Replica can be upload pending and then close starts
        // Down
        // NodeUp
        // Open (Up is pending)
        // Closing  (none)
        // LRU (ReplicaUpload)
        // AppHostDOwn -> this will call OnClosing
        OnClosingTransition
    },
    [&]() {OnLastReplicaUpPending(); },
    FMMessageStage::ReplicaUpload);
}

BOOST_AUTO_TEST_CASE(LRU_ResetTransitionsToNone)
{
    OnLastReplicaUpPending();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(LRU_OnDropped)
{
    OnLastReplicaUpPending();

    OnDropped();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDropped);
}

BOOST_AUTO_TEST_CASE(LRU_OnLastReplicaUpReply)
{
    OnLastReplicaUpPending();

    OnLastReplicaUpAcknowledged();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(LRU_PersistedReplicaDown)
{
    // Consider:
    // Node comes up
    // LFUMLoad sets instance
    // Reopen completes and replica up is marked pending and instance is cleared
    // Replica starts to close due to drop
    // Now upload for replica discovery happens 
    // Now replica goes down -> should not assert
    OnLfumLoad(1);

    OnUp();

    OnClosing();

    OnLastReplicaUpPending();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUpload);

    OnPersistedReplicaDown(2);
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_Asserts)
{
    VerifyAssertsTest(
    {
        // Shouldn't be called twice
        OnLastReplicaUpPendingTransition,

        // To get an end point update the replica must come up
        EndpointUpdateTransition,

        // Persisted because instance exists
        OnVolatileReplicaDownTransition,

        OnLfumLoadTransition,

        OnReplicaUpReplyTransition,
        OnReplicaDownReplyTransition,
        OnReplicaDroppedReplyTransition,
    },
    [&]() { OnLfumLoad(1); OnLastReplicaUpPending(); });
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_NoOp)
{
    VerifyNoOpTest(
    {
        // If ReplicaUpload is pending and then close happens close continues
        OnClosingTransition
    },
    [&]() {OnLfumLoad(1); OnLastReplicaUpPending(); },
    FMMessageStage::ReplicaUpload);
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_ResetTransitionsToNone)
{
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    Reset();

    VerifyMessageStageAndUpdate(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_OnDropped)
{
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    OnDropped();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDropped);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_OnUp)
{
    // open is pending, then lru timer expires and finally open complets
    // replica discovery not complete so use replica up as the message
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    OnUp();

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaUp);

    VerifyInstanceIsNotSet();
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_OnPersistedReplicaDown)
{
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    OnPersistedReplicaDown(2);

    VerifyMessageStageAndUpdate(FMMessageStage::ReplicaDown);

    VerifyInstance(2);
}

BOOST_AUTO_TEST_CASE(LRULfumLoad_OnLastReplicaUpReply)
{
    OnLfumLoad(1);

    OnLastReplicaUpPending();

    OnLastReplicaUpAcknowledged();

    VerifyMessageStageAndUpdate(FMMessageStage::None);

    VerifyInstance(1);
}

BOOST_AUTO_TEST_CASE(OnRetryInvokesRetryPolicy)
{
    OnPersistedReplicaDown(1);

    auto now = Stopwatch::Now();

    OnRetry(now, GetSequenceNumber());

    VerifyInMemoryCommit();

    VerifyShouldRetry(false, now);
}

BOOST_AUTO_TEST_CASE(StaleOnRetryIsNoOp)
{
    OnPersistedReplicaDown(1);
    
    auto now = Stopwatch::Now();
    
    OnRetry(now, 0);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
