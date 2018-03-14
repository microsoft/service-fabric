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

class TestUnit_ReplicaUploadState : public TestBase
{
protected:
    TestUnit_ReplicaUploadState() :
    uploadState_(&holder_.FMMessageStateObj, *FailoverManagerId::Fmm)
    {        
    }

    void MarkUploadPending()
    {
        auto context = CreateContext();
        uploadState_.MarkUploadPending(context);
    }

    void Upload()
    {
        auto context = CreateContext();
        uploadState_.UploadReplica(context);
    }

    void OnUploaded()
    {
        auto context = CreateContext();
        uploadState_.OnUploaded(context);
    }

    void VerifyUploadPendingFlagIs(bool expected)
    {
        Verify::AreEqual(expected, uploadState_.IsUploadPending, L"IsUploadPending");
    }

    void VerifyMessageStageIs(FMMessageStage::Enum expected)
    {
        Verify::AreEqual(expected, holder_.FMMessageStateObj.MessageStage, L"MessageStage");
    }

    void VerifyMessageStageIsNoneAndNoCommit()
    {
        VerifyMessageStageIs(FMMessageStage::None);

        VerifyNoCommit();
    }

    FMMessageStateHolder holder_;
    ReplicaUploadState uploadState_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestUnit_ReplicaUploadStateSuite, TestUnit_ReplicaUploadState)

BOOST_AUTO_TEST_CASE(InitialValues)
{
    VerifyUploadPendingFlagIs(false);

    VerifyMessageStageIs(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(MarkAsUploadPending_SetsFlag)
{
    MarkUploadPending();

    VerifyUploadPendingFlagIs(true);

    VerifyInMemoryCommit();
}

BOOST_AUTO_TEST_CASE(MarkAsUploadPending_DoesNotChangeFMState)
{
    MarkUploadPending();

    VerifyMessageStageIs(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(MarkAsUploadPendingWhenUploadAlreadyPendingAsserts)
{
    MarkUploadPending();

    Verify::Asserts([&]() {MarkUploadPending(); }, L"Upload pending should assert");
}

BOOST_AUTO_TEST_CASE(Upload_InformsMessageStageIfNeeded)
{
    MarkUploadPending();

    Upload();

    VerifyMessageStageIs(FMMessageStage::ReplicaUpload);
}

BOOST_AUTO_TEST_CASE(Upload_NoOpIfRequestNotPending)
{
    // Should not happen but no way to validate this without additional state
    Upload();

    VerifyMessageStageIsNoneAndNoCommit();
}

BOOST_AUTO_TEST_CASE(Upload_DuplicateIsNoOp)
{
    MarkUploadPending();

    Upload();

    Upload();

    VerifyMessageStageIs(FMMessageStage::ReplicaUpload);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_CASE(Upload_NoOpIfAlreadyAcknowledged)
{
    MarkUploadPending();

    OnUploaded();

    Upload();

    VerifyMessageStageIsNoneAndNoCommit();
}

BOOST_AUTO_TEST_CASE(Uploaded_NoOpIfNotPending)
{
    // Deleted replica gets a message from fm
    OnUploaded();

    VerifyMessageStageIsNoneAndNoCommit();
}

BOOST_AUTO_TEST_CASE(Uploaded_TellsFMMessageStageThatUploadIsCompleteIfPending)
{
    MarkUploadPending();

    OnUploaded();

    VerifyMessageStageIs(FMMessageStage::None);
}

BOOST_AUTO_TEST_CASE(Uploaded_ClearsFlag)
{
    MarkUploadPending();

    OnUploaded();

    VerifyUploadPendingFlagIs(false);

    VerifyInMemoryCommit();
}

BOOST_AUTO_TEST_CASE(Uploaded_NoOpIfAlreadyAcknowledeged)
{
    MarkUploadPending();

    OnUploaded();

    OnUploaded();

    VerifyMessageStageIs(FMMessageStage::None);

    VerifyNoCommit();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
