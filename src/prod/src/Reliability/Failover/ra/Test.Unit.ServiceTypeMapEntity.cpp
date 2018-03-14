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
using namespace Hosting;
using namespace Hosting2;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestServiceTypeMapEntity
{
protected:
    TestServiceTypeMapEntity() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestServiceTypeMapEntity() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void Create(ServiceTypeRegistrationSPtr const & registration);

    bool ExecuteAdd(ServiceTypeRegistrationSPtr const & registration);
    void ExecuteAddAndVerify(ServiceTypeRegistrationSPtr const & registration, bool expected);

    void ExecuteAddToListAndVerify(ServiceTypeRegistrationSPtr const & expected, bool expectedIsPending);

    void ExecuteOnReply(ServiceTypeRegistrationSPtr const & reply);
    
    bool ExecuteRemove();
    void ExecuteRemoveAndVerify(bool expected);


    UnitTestContextUPtr utContext_;
    
    ServiceTypeRegistrationSPtr CPA_V1;
    ServiceTypeRegistrationSPtr CPA_V2;
    ServiceTypeRegistrationSPtr CPB_V3;

    std::unique_ptr<ServiceTypeMapEntity> entity_;
};

bool TestServiceTypeMapEntity::TestSetup()
{
    utContext_ = UnitTestContext::Create();

    auto typeId = Default::GetInstance().SP1_SP1_STContext.ServiceTypeId;
    auto v1Instance = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"1.0:1.0:1");
    auto v2Instance = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"1.0:1.0:2");
    auto v3Instance = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(L"1.0:1.0:3");
    
    int randomPID = 8120;
    CPA_V1 = ServiceTypeRegistration::Test_Create(typeId, v1Instance, ServiceModel::ServicePackageActivationContext(), L"", L"a", L"b", L"A", 1, randomPID);
    CPA_V2 = ServiceTypeRegistration::Test_Create(typeId, v2Instance, ServiceModel::ServicePackageActivationContext(), L"", L"a", L"b", L"A", 2, randomPID);
    CPB_V3 = ServiceTypeRegistration::Test_Create(typeId, v3Instance, ServiceModel::ServicePackageActivationContext(), L"", L"a", L"b", L"B", 3, randomPID);

    return true;
}

bool TestServiceTypeMapEntity::TestCleanup()
{
    return utContext_->Cleanup();
}

void TestServiceTypeMapEntity::Create(ServiceTypeRegistrationSPtr const & registration)
{
    entity_ = make_unique<ServiceTypeMapEntity>(
        utContext_->RA.NodeInstance,
        registration, 
        utContext_->Config.PerServiceTypeMinimumIntervalBetweenMessageToFMEntry);
}

bool TestServiceTypeMapEntity::ExecuteAdd(ServiceTypeRegistrationSPtr const & registration)
{
    bool shouldSend = false;
    entity_->OnRegistrationAdded(L"A", registration, shouldSend);
    return shouldSend;
}

void TestServiceTypeMapEntity::ExecuteAddAndVerify(ServiceTypeRegistrationSPtr const & registration, bool expected)
{
    bool actual = ExecuteAdd(registration);
    Verify::AreEqual(expected, actual, L"Add: ShouldSend");
}

void TestServiceTypeMapEntity::ExecuteOnReply(ServiceTypeRegistrationSPtr const & expected)
{
    ServiceTypeInfo replyInfo(
        expected->VersionedServiceTypeId,
        expected->CodePackageName);

    entity_->OnReply(replyInfo);
}

void TestServiceTypeMapEntity::ExecuteAddToListAndVerify(ServiceTypeRegistrationSPtr const & expected, bool expectedIsPending)
{
    vector<ServiceTypeInfo> infos; 
    bool actualFlag = false;

    entity_->UpdateFlagIfRetryPending(actualFlag);
    Verify::AreEqual(expectedIsPending, actualFlag, L"IsPending");

    bool expectedHasData = expected != nullptr;
    auto context = entity_->CreateRetryContext(Stopwatch::Now());
    Verify::AreEqual(expectedHasData, context.HasData, L"HasData does not match");

    if (!expectedHasData)
    {
        return;
    }

    context.Generate(infos);
    Verify::AreEqual(1, infos.size(), L"Expected nothing to be added but something was added");
    if (infos.size() != 1)
    {
        return;
    }

    auto const & actual = infos[0];
    Verify::AreEqual(expected->VersionedServiceTypeId, actual.VersionedServiceTypeId, L"Versioned STID");
    Verify::AreEqual(expected->CodePackageName, actual.CodePackageName, L"Code package name");
}

bool TestServiceTypeMapEntity::ExecuteRemove()
{
    bool isDelete = false;
    entity_->OnRegistrationRemoved(L"A", isDelete);
    return isDelete;
}

void TestServiceTypeMapEntity::ExecuteRemoveAndVerify(bool expected)
{
    bool actual = ExecuteRemove();
    Verify::AreEqual(expected, actual, L"IsDelete: RemoveAndVerify");
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestServiceTypeMapEntitySuite, TestServiceTypeMapEntity)

BOOST_AUTO_TEST_CASE(Add_OnNewEntity_SendsMessage)
{
    Create(CPA_V1);

    ExecuteAddAndVerify(CPA_V1, true);
}

BOOST_AUTO_TEST_CASE(Add_ExistingEntity_DoesNotSendMessage)
{
    Create(CPA_V1);

    ExecuteAdd(CPA_V1);

    ExecuteAddAndVerify(CPA_V1, false);
}

BOOST_AUTO_TEST_CASE(Add_ExistingEntityWithDifferentVersion_DoesNotSendMessage)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);

    ExecuteAddAndVerify(CPB_V3, true);
}

BOOST_AUTO_TEST_CASE(Add_AfterReply_SameVersion_NoMessage)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);
    ExecuteOnReply(CPA_V1);

    ExecuteAddToListAndVerify(nullptr, false);
}

BOOST_AUTO_TEST_CASE(Add_AfterReply_DifferentVersion_SendsMessage)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);
    ExecuteOnReply(CPA_V1);

    ExecuteAddAndVerify(CPA_V2, true);
}

BOOST_AUTO_TEST_CASE(Message_Contents_InitialVersion)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);

    ExecuteAddToListAndVerify(CPA_V1, true);
}

BOOST_AUTO_TEST_CASE(Message_Contents_AfterUpdate)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);

    ExecuteAdd(CPB_V3);
    ExecuteAddToListAndVerify(CPB_V3, true);
}

BOOST_AUTO_TEST_CASE(MessageIsNotSentAfterReply)
{
    Create(CPA_V1);
    ExecuteAdd(CPA_V1);
    ExecuteOnReply(CPA_V1);

    ExecuteAddToListAndVerify(nullptr, false);
}

BOOST_AUTO_TEST_CASE(Reply_MustBeExactMatch)
{
    Create(CPA_V2);
    ExecuteAdd(CPA_V2);

    ExecuteOnReply(CPB_V3);
    ExecuteAddToListAndVerify(CPA_V2, true);
}

BOOST_AUTO_TEST_CASE(Remove_GreaterThanZeroCount_DeleteIsFalse)
{
    Create(CPA_V2);
    ExecuteAdd(CPA_V2);
    ExecuteAdd(CPA_V2);

    ExecuteRemoveAndVerify(false);
}

BOOST_AUTO_TEST_CASE(Remove_EqualToZeroCount_DeleteIsFalse)
{
    Create(CPA_V2);
    ExecuteAdd(CPA_V2);

    ExecuteRemoveAndVerify(true);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
