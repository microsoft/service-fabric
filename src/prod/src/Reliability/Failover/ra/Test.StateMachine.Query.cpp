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

#pragma region Query

class TestStateMachine_Query : public StateMachineTestBase
{
protected:

    void TranslationTestHelper(wstring const & ftShortName, wstring const & ft, wstring const & message)
    {
        Recreate();
        Test.AddFT(ftShortName, ft);

        ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");
        Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName SP1");

        Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", message);
    }
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_QueryHelperSuite, TestStateMachine_Query)

BOOST_AUTO_TEST_CASE(QueryHelper_InvalidQueryName)
{
    // Invalid Query name should return invalid operation
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"InvalidQueryName");

    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"FABRIC_E_INVALID_OPERATION");
}

BOOST_AUTO_TEST_CASE(QueryHelper_QueryTypeParallel)
{
    // RA Query doesn't support parallel query type. This should return invalid operation.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"QueryTypeParallel");

    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"FABRIC_E_INVALID_OPERATION");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_QueryDeployedServiceReplicaSuite, TestStateMachine_Query)

BOOST_AUTO_TEST_CASE(ReplicaListQuery_NoApplicationNameParameter)
{
    // ApplicationName is a mandatory parameter. This should return invalid argument
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest");

    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"E_INVALIDARG");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_EmptyGuidForPartitionId)
{
    // Empty guid for partitionId should return invalid argument
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");

    ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName SP1 00000000-0000-0000-0000-000000000000");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"E_INVALIDARG");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_EmptyApplicationCache)
{
    // There are no partitions. Query should return empty list.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest fooApp barServiceType");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_NoPartitons)
{
    // There are no partitons. Query parameter partitionId will have no matching partition. Query should return empty list.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName SP1 AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_PartitionIdNotFound)
{
    // Provided partitionId parameter does not match with existing partitions. Query should return empty list.
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    Test.AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    Test.AddFT(L"SL2", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    Test.AddFT(L"SV2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");

    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z1", L"DeployedServiceQueryRequest AppName SP1 AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAB");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z1", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_ClosedFT)
{
    // closed FT
    TranslationTestHelper(L"SP1", L"C None 000/000/411 1:1 -", L"Success DeployedServiceQueryResult [SP1 true 1 U DD]");

    // down and deleted FT
    TranslationTestHelper(L"SP1", L"C None 000/000/411 1:1 LP", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_InternalServiceAppName)
{
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");

    // GetServiceReplicaListDeployedOnNode does not return systerm services info. fabric:/System is a system service application name. Query should return empty list.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest fabric:/System");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_NoMatchApplicationName)
{
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");

    ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");

    // If provided application name parameter does not match any service, query should return empty list.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName1");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_NoMatchServiceManifestName)
{
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");

    ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");

    // If provided serviceManifestName parameter does not match any service, query should return empty list.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName InvalidServiceManifestName");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult");
}


BOOST_AUTO_TEST_CASE(ReplicaListQuery_ServiceManifestFilteringTest)
{
    Test.AddFT(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    Test.AddFT(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");
    Test.AddFT(L"SL2", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
    Test.AddFT(L"SV2", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    ASSERT_IF(L"AppName" != Default::GetInstance().ApplicationName, "App name must match default because it is used in a string below");

    // Provided service manifest name matches two replicas. Query should return list of two replicas.
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaQueryRequest>(L"z", L"DeployedServiceQueryRequest AppName SP2");
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaQueryResult>(L"z", L"Success DeployedServiceQueryResult [SL2 false 1 N RD] [SV2 true 1 P RD]");
}

BOOST_AUTO_TEST_CASE(ReplicaListQuery_TranslationTest)
{
    // Stateful
    TranslationTestHelper(L"SV1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]", L"Success DeployedServiceQueryResult [SV1 true 1 P RD]");
    TranslationTestHelper(L"SV1", L"O None 000/000/411 1:1 CM [N/N/S IB U N F 1:1]", L"Success DeployedServiceQueryResult [SV1 true 1 S IB]");
    TranslationTestHelper(L"SV1", L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1]", L"Success DeployedServiceQueryResult [SV1 true 1 S SB]");
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 M [N/N/S SB D N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 S D]");

    //Replica opening with service type registeration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 MS [N/N/S SB U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 S SB]");
    //Replica opening without service type registration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 S [N/N/S SB U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 S SB]");

    //Replica closing with service type registration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 P RD]");
    //Replica closing without service type registration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 Hc [N/N/P RD U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 P RD]");

    //Replica dropping with service type registration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 MHd [N/N/P ID U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 P DD]");
    //Replica dropping without service type registration
    TranslationTestHelper(L"SP1", L"O None 000/000/411 1:1 Hd [N/N/P ID U N F 1:1]", L"Success DeployedServiceQueryResult [SP1 true 1 P DD]");

    // Stateless
    TranslationTestHelper(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]", L"Success DeployedServiceQueryResult [SL1 false 1 N RD]");
    TranslationTestHelper(L"SL1", L"O None 000/000/411 1:1 CM [N/N/N IB U N F 1:1]", L"Success DeployedServiceQueryResult [SL1 false 1 N IB]");
}

BOOST_AUTO_TEST_SUITE_END()

class TestStateMachine_QueryDeployedServiceReplicaDetail
{
protected:
    TestStateMachine_QueryDeployedServiceReplicaDetail() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestStateMachine_QueryDeployedServiceReplicaDetail() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    __declspec(property(get = get_Test)) ScenarioTest & Test;
    ScenarioTest & get_Test() { return holder_->ScenarioTestObj; }

    void Recreate()
    {
        holder_->Recreate();
    }

    void InvalidFTStateTestHelper(wstring const & ftShortName, wstring const & ftState, wstring const & msg, wstring const & error);
    void ValidateErrorForRequest(wstring const & tag, wstring const & replyBody);
    void AddFTAndProcess();
    void AddFTAndProcess(wstring const & shortName, wstring const & contextId);
    void ProcessMessageAndValidateError(wstring const & message, wstring const & error);
    void ValidateRAPMessage(size_t index, wstring const & ft, wstring const & message);
    void ProcessQuery(wstring const & shortName, wstring const & contextId);

private:
    ScenarioTestHolderUPtr holder_;
};

bool TestStateMachine_QueryDeployedServiceReplicaDetail::TestSetup()
{
    holder_ = ScenarioTestHolder::Create();
    return true;
}

bool TestStateMachine_QueryDeployedServiceReplicaDetail::TestCleanup()
{
    holder_.reset();
    return true;
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::ProcessQuery(wstring const & shortName, wstring const & contextId)
{
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaDetailQueryRequest>(contextId, wformatString("DeployedServiceReplicaDetailQueryRequest {0} 1", shortName));
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::AddFTAndProcess(wstring const & shortName, wstring const & contextId)
{
    Test.AddFT(shortName, L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]");

    ProcessQuery(shortName, contextId);
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::ValidateErrorForRequest(wstring const & tag, wstring const & replyBody)
{
    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaDetailQueryReply>(tag, replyBody);
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::AddFTAndProcess()
{
    AddFTAndProcess(L"SP1", L"q1");
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::ProcessMessageAndValidateError(wstring const & message, wstring const & error)
{
    Test.ProcessRequestReceiverContextAndDrain<MessageType::DeployedServiceReplicaDetailQueryRequest>(L"z", message);

    ValidateErrorForRequest(L"z", error);
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::ValidateRAPMessage(size_t, wstring const & ft, wstring const & message)
{
    Test.ValidateRAPRequestReplyMessage<MessageType::ProxyDeployedServiceReplicaDetailQueryRequest>(ft, message);
}

void TestStateMachine_QueryDeployedServiceReplicaDetail::InvalidFTStateTestHelper(wstring const & ftShortName, wstring const & ftState, wstring const & msg, wstring const & error)
{
    Recreate();

    Test.AddFT(ftShortName, ftState);
    ProcessMessageAndValidateError(msg, error);
}

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_QueryDeployedServiceReplicaDetailSuite, TestStateMachine_QueryDeployedServiceReplicaDetail)

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery__NonExistantPartition)
{
    // PartitionId parameter does not match any partitions. Query should return Invalid Argument.
    Recreate();
    ProcessMessageAndValidateError(L"DeployedServiceReplicaDetailQueryRequest SP1 1", L"InvalidArgument");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery__ReplicaIdMismatch)
{
    // Provided replicaId parameter does not match local replicaId. Query should return REReplicaDoesNotExist error.
    InvalidFTStateTestHelper(L"SP1", L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]", L"DeployedServiceReplicaDetailQueryRequest SP1 2", L"FABRIC_E_REPLICA_DOES_NOT_EXIST");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery_RegistrationNotFoundTestCases)
{
    // FT is closed. Query should return ObjectClosed error.
    InvalidFTStateTestHelper(L"SP1", L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]", L"DeployedServiceReplicaDetailQueryRequest SP1 1", L"FABRIC_E_OBJECT_CLOSED");
    InvalidFTStateTestHelper(L"SP1", L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]", L"DeployedServiceReplicaDetailQueryRequest SP1 1", L"FABRIC_E_OBJECT_CLOSED");
    InvalidFTStateTestHelper(L"SP1", L"O None 000/000/411 1:1 S [N/N/P SB U N F 1:1]", L"DeployedServiceReplicaDetailQueryRequest SP1 1", L"FABRIC_E_OBJECT_CLOSED");
    InvalidFTStateTestHelper(L"SP1", L"C None 000/000/411 1:1 -", L"DeployedServiceReplicaDetailQueryRequest SP1 1", L"FABRIC_E_OBJECT_CLOSED");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery_IncomingQuerySendsMessageToRAP)
{
    AddFTAndProcess(L"SP1", L"z");

    ValidateRAPMessage(0, L"SP1", L"000/411 [N/P RD U 1:1] -");

    Test.ValidateRAPRequestReplyMessage<MessageType::ProxyDeployedServiceReplicaDetailQueryRequest>(L"SP1", L"000/411 [N/P RD U 1:1] -");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery_FailureFromTransportSendsBackFailure)
{
    auto & rapTransport = Test.UTContext.RapTransport;
    rapTransport.RequestReplyApi.SetCompleteSynchronouslyWithError(Common::ErrorCodeValue::GlobalLeaseLost);

    AddFTAndProcess(L"SP1", L"z");

    ValidateErrorForRequest(L"z", L"GlobalLeaseLost");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery_FailureFromRAPSendsBackFailure)
{
    auto & rapTransport = Test.UTContext.RapTransport;
    rapTransport.RequestReplyApi.SetCompleteAsynchronously();

    AddFTAndProcess(L"SP1", L"z");

    auto message = Test.ParseMessage<MessageType::ProxyDeployedServiceReplicaDetailQueryReply>(L"SP1", L"000/411 [N/P RD U 1:1] GlobalLeaseLost 2 [abc]");
    rapTransport.RequestReplyApi.FinishOperationWithSuccess(move(message));
    ValidateErrorForRequest(L"z", L"GlobalLeaseLost");
}

BOOST_AUTO_TEST_CASE(ReplicaDetailQuery_SuccessFromRAPSendsBackReply)
{
    auto & rapTransport = Test.UTContext.RapTransport;
    rapTransport.RequestReplyApi.SetCompleteAsynchronously();

    AddFTAndProcess(L"SP1", L"z");

    auto message = Test.ParseMessage<MessageType::ProxyDeployedServiceReplicaDetailQueryReply>(L"SP1", L"000/411 [N/P RD U 1:1] Success 2 [abc]");
    rapTransport.RequestReplyApi.FinishOperationWithSuccess(move(message));

    Test.ValidateRequestReceiverContextReply<MessageType::DeployedServiceReplicaDetailQueryReply>(L"z", L"Success DeployedServiceReplicaDetailQueryResult [abc]");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#pragma endregion
