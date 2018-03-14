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
using namespace Node;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    const int DefaultReopenSuccessWaitIntervalInSeconds = 5;
    const EntitySetIdentifier DefaultSetId = EntitySetIdentifier(EntitySetName::ReplicaDown, FailoverManagerId(true));
}

class TestUnitPendingReplicaUploadState : public TestBase
{
protected:
    TestUnitPendingReplicaUploadState() : lastResult_(FailoverManagerId(true))
    {
        BOOST_REQUIRE(TestSetup());
    }

    TEST_METHOD_SETUP(TestSetup);

    Infrastructure::EntityEntryBaseList GetList(int count)
    {
        Infrastructure::EntityEntryBaseList rv;

        switch (count)
        {
        case 2:
            rv.push_back(entity2_);
            __fallthrough;
        case 1:
            rv.push_back(entity1_);
            __fallthrough;
        case 0:
            break;
        default:
            Verify::Fail(L"Only have two entities right now");
            break;
        }

        return rv;
    }

    void OnNodeUpAckProcessed(int ftCount, bool isDeferredRequired)
    {
        auto & set = GetSet();
        
        for (auto const & it : GetList(ftCount))
        {
            set.Test_FTSet.insert(it);
        }

        nodeUpAckTime_ = Stopwatch::Now();
        lastApiTime_ = nodeUpAckTime_;
        lastResult_ = state_->OnNodeUpAckProcessed(L"a", nodeUpAckTime_, isDeferredRequired);
    }

    void VerifySendLastReplicaUp()
    {
        VerifyResult(true, false);
    }

    void VerifyNoOp()
    {
        VerifyResult(false, false);
    }

    void VerifyDeferred(int count)
    {
        VerifyResult(false, true);

        auto expected = GetList(count);
        auto actual = lastResult_.Entities;
        
        sort(expected.begin(), expected.end());
        sort(actual.begin(), actual.end());

        Verify::VectorStrict(expected, actual);
    }

    void VerifyComplete()
    {
        Verify::IsTrue(state_->IsComplete, L"IsComplete");
    }

    void VerifyResult(bool sendLastReplicaUp, bool invokeUpload)
    {
        Verify::AreEqual(sendLastReplicaUp, lastResult_.SendLastReplicaUp, L"Send Last Replica Up");
        Verify::AreEqual(invokeUpload, lastResult_.InvokeUploadOnEntities, L"Invoke upload");
        Verify::AreEqual(!invokeUpload, lastResult_.Entities.empty(), L"Entities");
        Verify::AreEqual(DefaultSetId.FailoverManager, lastResult_.TraceDataObj.Owner, L"Owner");
        Verify::AreEqual(state_->IsComplete, lastResult_.TraceDataObj.IsComplete, L"IsComplete");
        Verify::AreEqual(GetSet().Size, lastResult_.TraceDataObj.PendingReplicaCount, L"Pending count");
        Verify::AreEqual(lastApiTime_ - nodeUpAckTime_, lastResult_.TraceDataObj.Elapsed, L"Elapsed");
    }

    void OnTimer(int secondsSinceNodeUpAck)
    {
        lastResult_ = state_->OnTimer(L"a", GetTime(secondsSinceNodeUpAck));
    }

    void OnTimerAfterInterval(int secondsAfterReopenSuccessWaitInterval)
    {
        OnTimer(static_cast<int>(DefaultReopenSuccessWaitIntervalInSeconds + secondsAfterReopenSuccessWaitInterval));
    }

    void OnReply()
    {
        lastResult_ = state_->OnLastReplicaUpReply(L"A", GetTime(2992));
    }

    StopwatchTime GetTime(int secondsSinceNodeUpAck)
    {
        lastApiTime_ = nodeUpAckTime_ + TimeSpan::FromSeconds(secondsSinceNodeUpAck);
        return lastApiTime_;
    }

    void AcknowledgeAll()
    {
        GetSet().Test_FTSet.clear();
    }

protected:
    EntitySet & GetSet()
    {
        return setCollection_.Get(DefaultSetId).Set;
    }

    StopwatchTime lastApiTime_;
    StopwatchTime nodeUpAckTime_;
    TestEntityEntrySPtr entity1_;
    TestEntityEntrySPtr entity2_;
    TimeSpanConfigEntry config_;
    unique_ptr<PendingReplicaUploadState> state_;
    EntitySetCollection setCollection_;
    PendingReplicaUploadState::Action lastResult_;
    Storage::Api::IKeyValueStoreSPtr store_;
};

bool TestUnitPendingReplicaUploadState::TestSetup()
{
    ConfigurationManager().InitializeAssertConfig(true);

    store_ = make_shared<Storage::InMemoryKeyValueStore>();
    config_.Test_SetValue(TimeSpan::FromSeconds(DefaultReopenSuccessWaitIntervalInSeconds));
    EntitySet::Parameters setParams(L"a", DefaultSetId, nullptr);
    state_ = make_unique<PendingReplicaUploadState>(setParams, setCollection_, config_);
    entity1_ = CreateTestEntityEntry(L"a", store_);
    entity2_ = CreateTestEntityEntry(L"b", store_);
    return true;
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestUnitPendingReplicaUploadStateSuite, TestUnitPendingReplicaUploadState)

BOOST_AUTO_TEST_CASE(SetIsAddedToCollectionTest)
{
    // Should not assert
    setCollection_.Get(DefaultSetId);
}

BOOST_AUTO_TEST_CASE(NodeUpAckWithZeroFtsRequestsReplicaUp)
{
    OnNodeUpAckProcessed(0, true);

    VerifySendLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(MultipleNodeUpAckAsserts)
{
    OnNodeUpAckProcessed(0, true);

    Verify::Asserts([&]() {OnNodeUpAckProcessed(0, true); }, L"assert");
}

BOOST_AUTO_TEST_CASE(ReplicaUpReplyCompletes)
{
    OnNodeUpAckProcessed(0, true);

    OnReply();

    VerifyComplete();

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(MultipleReplicaUpReplyIsNoOp)
{
    OnNodeUpAckProcessed(0, true);

    OnReply();

    OnReply();

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(OnTimerWithNoItemsReturnsSendReplicaUp)
{
    OnNodeUpAckProcessed(0, true);

    OnTimer(1);

    VerifySendLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(OnTimerWithNoItemsAndIntervalExpiredReturnsSendReplicaUp)
{
    OnNodeUpAckProcessed(0, true);

    OnTimerAfterInterval(4);

    VerifySendLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(OnTimerAfterReplicaUpReplyIsNoOp)
{
    OnNodeUpAckProcessed(0, true);

    OnReply();

    OnTimerAfterInterval(1);

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(NodeUpAckIsNoOpIfReplicasArePending)
{
    OnNodeUpAckProcessed(2, true);

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(OnTimerIsNoOpIfReplicasArePending)
{
    OnNodeUpAckProcessed(2, true);

    OnTimer(3);

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(LastReplicaUpReplyAssertsIfPending)
{
    OnNodeUpAckProcessed(2, true);

    Verify::Asserts([&]() { OnReply(); }, L"LastReplicaUpReply should assert");
}

BOOST_AUTO_TEST_CASE(WithDeferredApplyNotRequiredThenEvenAfterDelayNoWork)
{
    OnNodeUpAckProcessed(2, false);

    OnTimerAfterInterval(2);

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(AfterReplicasCompleteThenOnTimerReturnsSendReplicaUp)
{
    OnNodeUpAckProcessed(2, true);

    AcknowledgeAll();

    OnTimer(1);

    VerifySendLastReplicaUp();
}

BOOST_AUTO_TEST_CASE(AfterReplicasAreCompleteLastReplicaUpReplyIsNoOp)
{
    OnNodeUpAckProcessed(2, true);

    AcknowledgeAll();

    OnReply();

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(WithPendingReplicasAndDeferredReopenOnTimerReturnsDeferredReopenAction)
{
    OnNodeUpAckProcessed(2, true);

    OnTimerAfterInterval(3);

    VerifyDeferred(2);
}

BOOST_AUTO_TEST_CASE(DeferredActionIsReturnedOnlyOnce)
{
    OnNodeUpAckProcessed(2, true);

    OnTimerAfterInterval(3);

    OnTimerAfterInterval(4);

    VerifyNoOp();
}

BOOST_AUTO_TEST_CASE(WithPendingReplicasAndDeferredReopenOnTimerEntityTest)
{
    OnNodeUpAckProcessed(2, true);

    // Acknowledge last
    GetSet().Test_FTSet.erase(entity2_);

    OnTimerAfterInterval(3);

    VerifyDeferred(1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
