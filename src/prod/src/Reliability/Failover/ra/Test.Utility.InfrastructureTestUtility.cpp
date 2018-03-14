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
using namespace Reliability::ReconfigurationAgentComponent::Infrastructure::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

wstring const InfrastructureTestUtility::Key(L"hello");

InfrastructureTestUtility::InfrastructureTestUtility(UnitTestContext & utContext) :
    utContext_(utContext),
    entityMap_(utContext.ContextMap.Get<TestEntityMap>(TestContextMap::TestEntityMapKey))
{
}

UnitTestContextUPtr InfrastructureTestUtility::CreateUnitTestContextWithTestEntityMap()
{
    UnitTestContext::Option options;
    options.EnableTestAssert = true;
    options.UseStubJobQueueManager = true;
    options.ConsoleTraceLevel = 1;
    auto test = UnitTestContext::Create(options);

    auto testEntityMap = make_shared<TestEntityMap>(test->RA, test->Clock, test->RA.PerfCounters, make_shared<EntityPreCommitNotificationSink<TestEntity>>());
    test->ContextMap.Add(TestContextMap::TestEntityMapKey, testEntityMap);

    return test;
}

void InfrastructureTestUtility::ValidateEntityData(int expectedPersistedState, TestEntity const & actual)
{
    ValidateEntityData(Key, expectedPersistedState, actual);
}

void InfrastructureTestUtility::ValidateEntityData(int expectedPersistedState, int expectedInMemoryState, TestEntity const & actual)
{
    ValidateEntityData(Key, expectedPersistedState, expectedInMemoryState, actual);
}

void InfrastructureTestUtility::ValidateEntityData(std::wstring const & key, int expectedPersistedState, TestEntity const & actual)
{
    Verify::AreEqual(key, actual.Key, L"Key");
    Verify::AreEqual(expectedPersistedState, actual.PersistedData, L"PersistedState");
}

void InfrastructureTestUtility::ValidateEntityData(std::wstring const & key, int expectedPersistedState, int expectedInMemoryState, TestEntity const & actual)
{
    ValidateEntityData(key, expectedPersistedState, actual);
    Verify::AreEqual(expectedInMemoryState, actual.InMemoryData, L"InMemoryData");
}

void InfrastructureTestUtility::ValidateEntityData(int expectedPersistedState, int expectedInMemoryState, EntityEntryBaseSPtr const & entry)
{
    auto & casted = entry->As<TestEntityEntry>();

    auto & state = casted.State.Test_Data;

    Verify::AreEqual(false, state.IsDeleted, L"IsDeleted must be false");
    Verify::IsTrue(state.Data != nullptr, L"State must not be null");
    ValidateEntityData(Key, expectedPersistedState, expectedInMemoryState, *state.Data);
}

void InfrastructureTestUtility::ValidateEntityIsNull(EntityEntryBaseSPtr const & entry)
{
    auto & casted = entry->As<TestEntityEntry>();

    auto & state = casted.State.Test_Data;

    Verify::AreEqual(false, state.IsDeleted, L"IsDeleted must be false");
    Verify::IsTrue(state.Data == nullptr, L"State must be null");
}

void InfrastructureTestUtility::ValidateEntityIsDeleted(EntityEntryBaseSPtr const & entry)
{
    auto & casted = entry->As<TestEntityEntry>();

    auto & state = casted.State.Test_Data;

    Verify::AreEqual(true, state.IsDeleted, L"IsDeleted must be true");
    Verify::IsTrue(state.Data == nullptr, L"State must be null");
}

TestEntitySPtr InfrastructureTestUtility::CreateEntity(int persistedState, int inMemoryState)
{
    return make_shared<TestEntity>(InfrastructureTestUtility::Key, persistedState, inMemoryState);
}

std::vector<TestEntitySPtr> InfrastructureTestUtility::GetAllEntitiesFromStore()
{
    vector<Storage::Api::Row> rows;
    auto error = utContext_.RA.LfumStore->Enumerate(Storage::Api::RowType::Test, rows);
    Verify::IsTrue(error.IsSuccess(), L"Get must succeed");

    vector<shared_ptr<TestEntity>> v;
    for (auto & it : rows)
    {
        auto e = std::make_shared<TestEntity>();
        error = FabricSerializer::Deserialize(*e, it.Data);
        Verify::IsTrue(error.IsSuccess(), L"Deserialize must succeed");

        v.push_back(e);
    }
    
    return v;
}

void InfrastructureTestUtility::VerifyStoreIsEmpty()
{
    Verify::AreEqual(0, GetAllEntitiesFromStore().size(), L"Store should be empty");
}

void InfrastructureTestUtility::VerifyStoreValue(int expected)
{
    auto v = GetAllEntitiesFromStore();
    Verify::AreEqual(1, v.size(), L"Store should have one item");

    ValidateEntityData(expected, *v[0]);
}

Common::ErrorCode InfrastructureTestUtility::PerformStoreOperation(
    Storage::Api::OperationType::Enum operation,
    TestEntity const * data)
{
    return WriteRecordIntoStore<TestEntity>(
        operation,
        InfrastructureTestUtility::Key,
        data);
}

Common::ErrorCode InfrastructureTestUtility::PerformInsertOperation(
    int persistedState)
{
    auto data = CreateEntity(persistedState, 0);

    return PerformStoreOperation(Storage::Api::OperationType::Insert, data.get());
}

EntityEntryBaseSPtr InfrastructureTestUtility::SetupCreated()
{
    Setup();

    return GetOrCreate();
}

EntityEntryBaseSPtr InfrastructureTestUtility::SetupInserted(int persistedState, int inMemoryState)
{
    auto entry = SetupCreated();

    Insert(entry, persistedState, inMemoryState, PersistenceResult::Success);

    return entry;
}

EntityEntryBaseSPtr InfrastructureTestUtility::SetupDeleted()
{
    auto entry = SetupInserted(1, 2);

    Delete(entry, PersistenceResult::Success, true);

    return entry;
}

void InfrastructureTestUtility::Setup()
{
    entityMap_.Open(TestEntityMap::InMemoryState());
}

EntityEntryBaseSPtr InfrastructureTestUtility::GetOrCreate()
{
    return GetOrCreate(InfrastructureTestUtility::Key);
}

EntityEntryBaseSPtr InfrastructureTestUtility::GetOrCreate(std::wstring const & key)
{
    return entityMap_.GetOrCreateEntityMapEntry(key, true);
}

void InfrastructureTestUtility::ExecuteUnderLockAndCommit(
    EntityEntryBaseSPtr const & entry,
    PersistenceResult::Enum persistenceResult,
    bool isStorePersistenceExpected,
    function<void(LockedTestEntityPtr &)> func)
{
    auto & store = utContext_.FaultInjectedLfumStore;
    auto initial = store.PersistenceResult;
    KFinally([&]() { store.EnableFaultInjection(initial); });

    PersistenceResult::Set(persistenceResult, store);

    auto & casted = entry->As<TestEntityEntry>();
    LockedTestEntityPtr lockedEntity(casted.Test_CreateLock());

    func(lockedEntity);

    auto error = casted.Test_Commit(entry, lockedEntity, utContext_.RA);
    Verify::AreEqual(!isStorePersistenceExpected || persistenceResult == PersistenceResult::Success, error.IsSuccess(), L"Error from commit is not correct");
}

void InfrastructureTestUtility::Insert(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState, PersistenceResult::Enum persistenceResult)
{
    ExecuteUnderLockAndCommit(
        entry,
        persistenceResult,
        true,
        [&](LockedTestEntityPtr & lockedEntity)
    {
        Insert(lockedEntity, persistedState, inMemoryState);
    });
}

void InfrastructureTestUtility::Insert(LockedTestEntityPtr & lockedEntity, int persistedState, int inMemoryState)
{
    lockedEntity.Insert(CreateEntity(persistedState, inMemoryState));
}

void InfrastructureTestUtility::Delete(
    EntityEntryBaseSPtr const & entry,
    PersistenceResult::Enum persistenceResult,
    bool isPersistenceExpected)
{
    ExecuteUnderLockAndCommit(
        entry,
        persistenceResult,
        isPersistenceExpected,
        [&](LockedTestEntityPtr & lockedEntity)
    {
        Delete(lockedEntity);
    });
}

void InfrastructureTestUtility::Delete(
    LockedTestEntityPtr & lockedEntity)
{
    lockedEntity.MarkForDelete();
}

void InfrastructureTestUtility::UpdateInMemory(
    EntityEntryBaseSPtr const & entry,
    int inMemoryState)
{
    ExecuteUnderLockAndCommit(
        entry,
        PersistenceResult::Success,
        false,
        [&](LockedTestEntityPtr & lockedEntity)
        {
            UpdateInMemory(lockedEntity, inMemoryState);
        });
}

void InfrastructureTestUtility::UpdateInMemory(
    LockedTestEntityPtr & ptr,
    int inMemoryState)
{
    ptr.EnableInMemoryUpdate();
    ptr->InMemoryData = inMemoryState;
}

void InfrastructureTestUtility::Update(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState, PersistenceResult::Enum persistenceResult)
{
    ExecuteUnderLockAndCommit(
        entry,
        persistenceResult,
        true,
        [&](LockedTestEntityPtr & lockedEntity)
    {
        Update(lockedEntity, persistedState, inMemoryState);
    });
}

void InfrastructureTestUtility::Update(
    LockedTestEntityPtr & lockedEntity,
    int persistedState,
    int inMemoryState)
{
    lockedEntity.EnableUpdate();
    lockedEntity->PersistedData = persistedState;
    lockedEntity->InMemoryData = inMemoryState;
}
