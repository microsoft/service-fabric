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
using namespace Infrastructure::ReliabilityUnitTest;

using namespace Storage::Api;

/*
    The tests in this file are run against two implementations of the store (InMemory and real ese)

    The tests are defined in the base class (TestRAStore)

    The derived class is a template (TestRAStoreImpl) which uses the template parameter to correctly initialize the specific type of the store

    There are two sets of test cases (using the boost framework) that derive from the specialization of TestRAStoreImpl

    The LocalStoreTraits template is what is used to setup the correct store
*/

#define STORE_TEST_CASE(methodName) BOOST_AUTO_TEST_CASE(Test_##methodName) { methodName(); } 

template<bool TUseRealEse>
struct LocalStoreTraits;

template<>
struct LocalStoreTraits<false>
{
    static UnitTestContextUPtr CreateTestContext()
    {
        UnitTestContext::Option options;
        options.EnableTestAssert = true;
        options.UseStubJobQueueManager = true;
        auto test = UnitTestContext::Create(options);

        auto testEntityMap = make_shared<TestEntityMap>(test->RA, test->RA.Clock, test->RA.PerfCounters, make_shared<EntityPreCommitNotificationSink<TestEntity>>());
        test->ContextMap.Add(TestContextMap::TestEntityMapKey, testEntityMap);

        return test;
    }
};

template<>
struct LocalStoreTraits<true>
{
    static UnitTestContextUPtr CreateTestContext()
    {
        wstring eseFolder(L"RAStoreTest");
        bool exists = Directory::Exists(eseFolder);
        TestLog::WriteInfo(wformatString("Path {0}. Exists {1}", eseFolder, exists));

        if (exists)
        {
            auto error = Directory::Delete(eseFolder, true, true);
            ASSERT_IF(!error.IsSuccess(), "Failed to clean {0} {1}", eseFolder, error);
        }

        Directory::Create(eseFolder);

        UnitTestContext::Option options;
        options.EnableTestAssert = true;
        options.UseStubJobQueueManager = true;
        options.UseRealEse = true;
        options.WorkingDirectory = eseFolder;
        options.FileTraceLevel = 5;
        auto test = UnitTestContext::Create(options);

        auto testEntityMap = make_shared<TestEntityMap>(test->RA, test->Clock, test->RA.PerfCounters, make_shared<EntityPreCommitNotificationSink<TestEntity>>());
        test->ContextMap.Add(TestContextMap::TestEntityMapKey, testEntityMap);

        return test;
    }
};

class TestRAStore
{
protected:
    void EmptyStoreHasNoState();
    void InsertIntoEmptyStoreSucceeds();
    void MultipleInsertForSameKeyFails();
    void DeleteForNonExistantKeyFails();
    void DeleteForExistingKeyPasses();
    void UpdateForExistingKeyUpdates();
    void UpdateForNonExistingKeyFails();

    unique_ptr<InfrastructureTestUtility> infrastructureUtility_;
    UnitTestContextUPtr utContext_;
};

template<bool TUseRealEse>
class TestRAStoreImpl : public TestRAStore
{
protected:
    TestRAStoreImpl()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestRAStoreImpl()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);
};

template<bool TUseRealEse>
bool TestRAStoreImpl<TUseRealEse>::TestSetup()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return true; }

    Config cfg;
    utContext_ = LocalStoreTraits<TUseRealEse>::CreateTestContext();
    infrastructureUtility_ = make_unique<InfrastructureTestUtility>(*utContext_);
    return true;
}

template<bool TUseRealEse>
bool TestRAStoreImpl<TUseRealEse>::TestCleanup()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return true; }

    utContext_->Cleanup();
    return true;
}

void TestRAStore::EmptyStoreHasNoState()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    infrastructureUtility_->VerifyStoreIsEmpty();
}

void TestRAStore::InsertIntoEmptyStoreSucceeds()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    auto error = infrastructureUtility_->PerformInsertOperation(3);
    Verify::IsTrue(error.IsSuccess(), wformatString("Expected op to succeed {0}", error));

    infrastructureUtility_->VerifyStoreValue(3);
}

void TestRAStore::MultipleInsertForSameKeyFails()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    infrastructureUtility_->PerformInsertOperation(4);

    auto actual = infrastructureUtility_->PerformInsertOperation(3);
    Verify::AreEqual(ErrorCodeValue::StoreWriteConflict, actual.ReadValue(), L"Insert+Insert");

    infrastructureUtility_->VerifyStoreValue(4);
}

void TestRAStore::DeleteForExistingKeyPasses()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    infrastructureUtility_->PerformInsertOperation(3);

    auto error = infrastructureUtility_->PerformStoreOperation(OperationType::Delete, nullptr);
    Verify::IsTrue(error.IsSuccess(), L"Delete must succeed");

    infrastructureUtility_->VerifyStoreIsEmpty();
}

void TestRAStore::DeleteForNonExistantKeyFails()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    auto actual = infrastructureUtility_->PerformStoreOperation(OperationType::Delete, nullptr);
    Verify::AreEqual(ErrorCodeValue::StoreRecordNotFound, actual.ReadValue(), L"Delete+Delete");

    infrastructureUtility_->VerifyStoreIsEmpty();
}

void TestRAStore::UpdateForExistingKeyUpdates()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    infrastructureUtility_->PerformInsertOperation(3);

    auto error = infrastructureUtility_->PerformStoreOperation(
        OperationType::Update,
        infrastructureUtility_->CreateEntity(4, 0).get());
    Verify::IsTrue(error.IsSuccess(), L"update must succeed");

    infrastructureUtility_->VerifyStoreValue(4);
}

void TestRAStore::UpdateForNonExistingKeyFails()
{
    if (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore) { return ; }

    auto actual = infrastructureUtility_->PerformStoreOperation(
        OperationType::Update,
        infrastructureUtility_->CreateEntity(4, 0).get());

    Verify::AreEqual(ErrorCodeValue::StoreRecordNotFound, actual.ReadValue(), L"Update");

    infrastructureUtility_->VerifyStoreIsEmpty();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestRAStoreSuite_InMemoryStore, TestRAStoreImpl<false>)

STORE_TEST_CASE(EmptyStoreHasNoState)
STORE_TEST_CASE(InsertIntoEmptyStoreSucceeds)
STORE_TEST_CASE(MultipleInsertForSameKeyFails)
STORE_TEST_CASE(DeleteForNonExistantKeyFails)
STORE_TEST_CASE(DeleteForExistingKeyPasses);
STORE_TEST_CASE(UpdateForExistingKeyUpdates);
STORE_TEST_CASE(UpdateForNonExistingKeyFails);

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

//
// TSLocalStore depends on KtlLoggerNode, which has several other
// dependencies like a fabric data root and a fabric node config.
// KtlLoggerNode must be added to Test.Utility.UnitTestContext.cpp
// before it will work in this unit test. The tests below
// will be skipped when TStore is enabled.
//
// RocksDB also not longer exists on Linux.
//
BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(TestRAStoreSuite_RealEseStore, TestRAStoreImpl<true>)

STORE_TEST_CASE(EmptyStoreHasNoState)
STORE_TEST_CASE(InsertIntoEmptyStoreSucceeds)
STORE_TEST_CASE(MultipleInsertForSameKeyFails)
STORE_TEST_CASE(DeleteForNonExistantKeyFails)
STORE_TEST_CASE(DeleteForExistingKeyPasses);
STORE_TEST_CASE(UpdateForExistingKeyUpdates);
STORE_TEST_CASE(UpdateForNonExistingKeyFails);

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
