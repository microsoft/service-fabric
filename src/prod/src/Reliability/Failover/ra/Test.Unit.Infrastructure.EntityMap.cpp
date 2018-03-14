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

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
                // 
                template<>
                struct ReaderImpl <TestEntityCommitDescription>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum , ReadContext const & context, TestEntityCommitDescription & rv)
                    {
                        ErrorLogger errLogger(L"CommitDescription", value);
                        Reader reader(value, context);
                        
                        Storage::Api::OperationType::Enum val;
                        if (!reader.Read(L' ', val))
                        {
                            errLogger.Log(L"StoreOp");
                            return false;
                        }
                        
                        bool isInMemory = false;
                        if (!reader.Read(L' ', isInMemory))
                        {
                            errLogger.Log(L"InMemory");
                            return false;
                        }

                        int persistedState = 0, inMemoryState = 0;
                        if (!reader.Read(L' ', persistedState))
                        {
                            errLogger.Log(L"PersistedState");
                            return false;
                        }

                        if (!reader.Read(L' ', inMemoryState))
                        {
                            errLogger.Log(L"inMemoryState");
                            return false;
                        }

                        rv.Data = make_shared<TestEntity>(InfrastructureTestUtility::Key, persistedState, inMemoryState);
                        rv.Type.IsInMemoryOperation = isInMemory;
                        rv.Type.StoreOperationType = val;
                        return true;
                    }
                };
            }
        }
    }
}

class EntityMapTest
{
protected:
    EntityMapTest()
    {
        BOOST_REQUIRE(TestSetup()) ;
    }

    ~EntityMapTest()
    {
        BOOST_REQUIRE(TestCleanup()) ;
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);
    
    // Set up store with empty and open map with empty
    void Setup();

    void VerifyStoreIsEmpty();
    void VerifyStoreValue(int expected);

    void VerifyNotInEntityMap();
    void VerifyEntityMapValue(int persistedState, int inMemoryState);
    void VerifyNullInEntityMap();

    void VerifyEntityIsDeleted(EntityEntryBaseSPtr const & entry);
    void VerifyEntityIsInserted(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState);
    void VerifyEntityIsCreated(EntityEntryBaseSPtr const & entry);

    LockedTestEntityPtr CreateLock(EntityEntryBaseSPtr const & entry);
    ReadOnlyLockedTestEntityPtr CreateReadLock(EntityEntryBaseSPtr const & entry);

    void Insert(
        EntityEntryBaseSPtr const & entry, 
        int persistedState, 
        int inMemoryState, 
        PersistenceResult::Enum persistenceResult);

    void Insert(
        LockedTestEntityPtr & ptr,
        int persistedState,
        int inMemoryState);

    void Delete(
        EntityEntryBaseSPtr const & entry, 
        PersistenceResult::Enum persistenceResult,
        bool isStorePersistenceExpected);

    void Delete(
        LockedTestEntityPtr & ptr);
        
    void Update(
        EntityEntryBaseSPtr const & entry, 
        int persistedState, 
        int inMemoryState, 
        PersistenceResult::Enum persistenceResult);

    void Update(
        LockedTestEntityPtr & ptr,
        int persistedState,
        int inMemoryState);

    void UpdateInMemory(
        EntityEntryBaseSPtr const & entry,
        int inMemoryState);

    void UpdateInMemory(
        LockedTestEntityPtr & ptr,
        int inMemoryState);

    void SetupAndVerifyFailure(
        function<EntityEntryBaseSPtr ()> factory,
        function<void(LockedTestEntityPtr &)> setup,
        function<void(LockedTestEntityPtr &)> func);

    void SetupCreatedAndVerifyFailure(
        function<void(LockedTestEntityPtr &)> setup,
        function<void(LockedTestEntityPtr &)> func);

    void SetupInsertedAndVerifyFailure(
        function<void(LockedTestEntityPtr &)> setup,
        function<void(LockedTestEntityPtr &)> func);

    void SetupDeletedAndVerifyFailure(
        function<void(LockedTestEntityPtr &)> setup,
        function<void(LockedTestEntityPtr &)> func);

    void SetupAndVerifyCommitDescription(
        function<EntityEntryBaseSPtr()> factor,
        function<void(LockedTestEntityPtr &)> func,
        wstring const & expected);

    void SetupAndVerifyLockedEntityAccessor(
        function<EntityEntryBaseSPtr()> factory,
        function<void(LockedTestEntityPtr &)> func,
        function<void(LockedTestEntityPtr &)> verification);

    void SetupAndVerifyLockedEntityAccessorIsDeleted(
        function<EntityEntryBaseSPtr()> factory,
        function<void(LockedTestEntityPtr &)> func);

    void SetupAndVerifyLockedEntityAccessorIsNull(
        function<EntityEntryBaseSPtr()> factory,
        function<void(LockedTestEntityPtr &)> func);

    void SetupAndVerifyLockedEntityAccessorHasValue(
        function<EntityEntryBaseSPtr()> factory,
        function<void(LockedTestEntityPtr &)> func,
        int persistedState,
        int inMemoryState);

    vector<TestEntitySPtr> GetAllEntitiesFromStore();    

    InfrastructureTestUtilityUPtr infrastructureUtility_;
    UnitTestContextUPtr utContext_;
    TestEntityMap * entityMap_;
};

bool EntityMapTest::TestSetup()
{
    utContext_ = InfrastructureTestUtility::CreateUnitTestContextWithTestEntityMap();
    entityMap_ = &utContext_->ContextMap.Get<TestEntityMap>(TestContextMap::TestEntityMapKey);
    infrastructureUtility_ = make_unique<InfrastructureTestUtility>(*utContext_);
    return true;
}

bool EntityMapTest::TestCleanup()
{
    utContext_->Cleanup();
    return true;
}

LockedTestEntityPtr EntityMapTest::CreateLock(EntityEntryBaseSPtr const & entry)
{
    return entry->As<EntityEntry<TestEntity>>().Test_CreateLock();
}

ReadOnlyLockedTestEntityPtr EntityMapTest::CreateReadLock(EntityEntryBaseSPtr const & entry)
{
    return entry->As<EntityEntry<TestEntity>>().CreateReadLock();
}

vector<TestEntitySPtr> EntityMapTest::GetAllEntitiesFromStore()
{
    return infrastructureUtility_->GetAllEntitiesFromStore();
}

void EntityMapTest::VerifyEntityIsDeleted(EntityEntryBaseSPtr const & entry)
{
    VerifyStoreIsEmpty();
    VerifyNotInEntityMap();
    infrastructureUtility_->ValidateEntityIsDeleted(entry);
}

void EntityMapTest::VerifyEntityIsInserted(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState)
{
    VerifyStoreValue(persistedState);
    VerifyEntityMapValue(persistedState, inMemoryState);
    infrastructureUtility_->ValidateEntityData(persistedState, inMemoryState, entry);
}

void EntityMapTest::VerifyEntityIsCreated(EntityEntryBaseSPtr const & entry)
{
    VerifyStoreIsEmpty();
    VerifyNullInEntityMap();
    infrastructureUtility_->ValidateEntityIsNull(entry);
}

void EntityMapTest::VerifyStoreIsEmpty()
{
    infrastructureUtility_->VerifyStoreIsEmpty();
}

void EntityMapTest::VerifyStoreValue(int expected)
{
    infrastructureUtility_->VerifyStoreValue(expected);
}

void EntityMapTest::VerifyNotInEntityMap()
{
    auto entry = entityMap_->GetEntry(InfrastructureTestUtility::Key);
    Verify::IsTrue(entry == nullptr, L"entry must exist");
}

void EntityMapTest::VerifyEntityMapValue(int persistedState, int inMemoryState)
{
    auto entry = entityMap_->GetEntry(InfrastructureTestUtility::Key);

    infrastructureUtility_->ValidateEntityData(persistedState, inMemoryState, entry);
}

void EntityMapTest::VerifyNullInEntityMap()
{
    auto entry = entityMap_->GetEntry(InfrastructureTestUtility::Key);
    
    infrastructureUtility_->ValidateEntityIsNull(entry);
}

void EntityMapTest::SetupAndVerifyFailure(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> setup,
    function<void(LockedTestEntityPtr &)> func)
{
    auto entry = factory();
    LockedTestEntityPtr ptr(CreateLock(entry));
    
    if (setup != nullptr)
    {
        setup(ptr);
    }

    Verify::Asserts([&]() { func(ptr); }, L"expect assert");
}

void EntityMapTest::SetupCreatedAndVerifyFailure(
    function<void(LockedTestEntityPtr &)> setup,
    function<void(LockedTestEntityPtr &)> func)
{
    SetupAndVerifyFailure(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        setup,
        func);
}    

void EntityMapTest::SetupInsertedAndVerifyFailure(
    function<void(LockedTestEntityPtr &)> setup,
    function<void(LockedTestEntityPtr &)> func)
{
    SetupAndVerifyFailure(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        setup,
        func);
}    

void EntityMapTest::SetupDeletedAndVerifyFailure(
    function<void(LockedTestEntityPtr &)> setup,
    function<void(LockedTestEntityPtr &)> func)
{
    SetupAndVerifyFailure(
        [&]() { return infrastructureUtility_->SetupDeleted(); },
        setup,
        func);
}

void EntityMapTest::SetupAndVerifyCommitDescription(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> func,
    wstring const & expectedStr)
{
    auto entry = factory();

    LockedTestEntityPtr ptr(CreateLock(entry));

    func(ptr);

    auto actual = ptr.CreateCommitDescription();
    auto expected = Reader::ReadHelper<TestEntityCommitDescription>(expectedStr);
    Verify::IsTrue(expected.Type.StoreOperationType == actual.Type.StoreOperationType, wformatString("StoreOperationType {0} {1}", static_cast<int>(expected.Type.StoreOperationType), static_cast<int>(actual.Type.StoreOperationType)));
    Verify::AreEqual(expected.Type.IsInMemoryOperation, actual.Type.IsInMemoryOperation, L"InMemoryDescription");
    if (expected.Type.StoreOperationType == Storage::Api::OperationType::Delete)
    {
        Verify::IsTrue(nullptr == actual.Data.get(), L"Data must be null");
    }
    else
    {
        Verify::IsTrue(actual.Data.get() != nullptr, L"Data cannot be null");
        Verify::AreEqual(expected.Data->PersistedData, actual.Data->PersistedData, L"PersistedState");
        Verify::AreEqual(expected.Data->InMemoryData, actual.Data->InMemoryData, L"InMemoryState");
    }
}

void EntityMapTest::SetupAndVerifyLockedEntityAccessor(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> func,
    function<void(LockedTestEntityPtr &)> verification)
{
    auto entry = factory();

    LockedTestEntityPtr ptr(CreateLock(entry));

    func(ptr);

    verification(ptr);
}

void EntityMapTest::SetupAndVerifyLockedEntityAccessorIsDeleted(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> func)
{
    SetupAndVerifyLockedEntityAccessor(
        factory, 
        func,
        [&](LockedTestEntityPtr & ptr)
        {
            Verify::IsTrue(ptr.IsEntityDeleted, L"Entity must be deleted");
        });
}

void EntityMapTest::SetupAndVerifyLockedEntityAccessorIsNull(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> func)
{
    SetupAndVerifyLockedEntityAccessor(
        factory,
        func,
        [&](LockedTestEntityPtr & ptr)
        {
            Verify::IsTrue(!ptr.IsEntityDeleted, L"Entity must not be deleted");
            Verify::IsTrue(!ptr, L"Entity is null");
        });
}

void EntityMapTest::SetupAndVerifyLockedEntityAccessorHasValue(
    function<EntityEntryBaseSPtr()> factory,
    function<void(LockedTestEntityPtr &)> func,
    int persistedState,
    int inMemoryState)
{
    SetupAndVerifyLockedEntityAccessor(
        factory,
        func,
        [&](LockedTestEntityPtr & ptr)
        {
            Verify::IsTrue(!ptr.IsEntityDeleted, L"Entity must not be deleted");
            Verify::IsTrue((bool) ptr, L"Entity is not null");
            Verify::AreEqual(persistedState, ptr->PersistedData, L"Persisted Data");
            Verify::AreEqual(inMemoryState, ptr->InMemoryData, L"In Memory Data");
        });
}


void EntityMapTest::Insert(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState, PersistenceResult::Enum persistenceResult)
{
    infrastructureUtility_->Insert(entry, persistedState, inMemoryState, persistenceResult);
}

void EntityMapTest::Insert(LockedTestEntityPtr & lockedEntity, int persistedState, int inMemoryState)
{
    infrastructureUtility_->Insert(lockedEntity, persistedState, inMemoryState);
}

void EntityMapTest::Delete(
    EntityEntryBaseSPtr const & entry, 
    PersistenceResult::Enum persistenceResult,
    bool isPersistenceExpected)
{
    infrastructureUtility_->Delete(entry, persistenceResult, isPersistenceExpected);
}

void EntityMapTest::Delete(
    LockedTestEntityPtr & lockedEntity)
{
    infrastructureUtility_->Delete(lockedEntity);
}

void EntityMapTest::UpdateInMemory(
    EntityEntryBaseSPtr const & entry,
    int inMemoryState)
{
    infrastructureUtility_->UpdateInMemory(entry, inMemoryState);
}

void EntityMapTest::UpdateInMemory(
    LockedTestEntityPtr & ptr,
    int inMemoryState)
{
    infrastructureUtility_->UpdateInMemory(ptr, inMemoryState);
}

void EntityMapTest::Update(EntityEntryBaseSPtr const & entry, int persistedState, int inMemoryState, PersistenceResult::Enum persistenceResult)
{
    infrastructureUtility_->Update(entry, persistedState, inMemoryState, persistenceResult);
}

void EntityMapTest::Update(
    LockedTestEntityPtr & lockedEntity,
    int persistedState,
    int inMemoryState)
{
    infrastructureUtility_->Update(lockedEntity, persistedState, inMemoryState);
}

void EntityMapTest::Setup()
{
    entityMap_->Open(TestEntityMap::InMemoryState());
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(EntityMapTestSuite, EntityMapTest)

BOOST_AUTO_TEST_CASE(InitiallyMapIsEmpty)
{
    Setup();

    Verify::AreEqual(0, entityMap_->GetAllEntries().size(), L"Empty map must be empty");
    VerifyNotInEntityMap();
}

/*
The test cases in this file validate various transitions that can happen to an entity

An entity can be in the following states:
- Not Existing: The entity is not present in the store or in the map. Get will fail.
- Created: The entity is present in the map but does not exist in the store.
- Inserted: The entity is present in both the store and the map. It has a value
- Deleted: The entity has been removed from the store. It may yet be in the map.

The first sets of test validate the success and failure cases for commit. They call
commit with various commit descriptions and verify that it succeeds/fails as expected

NotExisting:
- infrastructureUtility_->GetOrCreate must return the entity.
- No other method can be called

Created:
- Insert must transition to Inserted
- Insert Failure must stay Created
- Delete must delete. Persistence will not happen so both success/fail must pass
- Delete must only be in memory delete

Inserted:
- Delete with success must delete
- Delete with failure should stay as inserted
- Delete in memory is not allowed
- Update must allow for update
- Update in memory must be an in memory update

Deleted:
- Insert must fail
- Delete must fail
- Update must fail

*/

BOOST_AUTO_TEST_CASE(GetOrCreateInsertsAnEmptyEntityButDoesNotPersist)
{
    auto entry = infrastructureUtility_->SetupCreated();

    VerifyEntityIsCreated(entry);
}

BOOST_AUTO_TEST_CASE(Created_InsertWithSuccessIsInserted)
{
    auto entry = infrastructureUtility_->SetupCreated();

    Insert(entry, 1, 2, PersistenceResult::Success);

    VerifyEntityIsInserted(entry, 1, 2);
}

BOOST_AUTO_TEST_CASE(Created_InsertWithFailureStaysCreated)
{
    auto entry = infrastructureUtility_->SetupCreated();

    Insert(entry, 1, 2, PersistenceResult::Failure);

    VerifyEntityIsCreated(entry);
}

BOOST_AUTO_TEST_CASE(Created_DeleteWithSuccessIsDeleted)
{
    auto entry = infrastructureUtility_->SetupCreated();

    Delete(entry, PersistenceResult::Success, false);

    VerifyEntityIsDeleted(entry);
}

BOOST_AUTO_TEST_CASE(Created_DeleteWithFailureStaysCreated)
{
    auto entry = infrastructureUtility_->SetupCreated();

    Delete(entry, PersistenceResult::Failure, false);

    VerifyEntityIsDeleted(entry);
}

BOOST_AUTO_TEST_CASE(Inserted_DeleteWithSuccessTransitionsToDeleted)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    Delete(entry, PersistenceResult::Success, true);

    VerifyEntityIsDeleted(entry);
}

BOOST_AUTO_TEST_CASE(Inserted_DeleteWithFailureStaysInserted)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    Delete(entry, PersistenceResult::Failure, true);

    VerifyEntityIsInserted(entry, 1, 2);
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateWithSuccessUpdates)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    Update(entry, 3, 4, PersistenceResult::Success);

    VerifyEntityIsInserted(entry, 3, 4);
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateWithFailureDoesNotChangeValues)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    Update(entry, 3, 4, PersistenceResult::Failure);

    VerifyEntityIsInserted(entry, 1, 2);
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemoryUpdates)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    UpdateInMemory(entry, 4);

    VerifyEntityIsInserted(entry, 1, 4);
}

/*
The remainder of test cases validate the negative scenarios for a locked ft

Created:
- Update must assert
- Update in memory must assert
- Insert + Insert must fail
- Delete + x must fail
- Insert + Delete + x must fail
- Insert + UpdateInMemory + Insert must fail
- Insert + Update + Insert must fail
- Insert + Update + Delete + x must fail

Inserted:
- Insert must fail
- Delete + x must fail
- Update + Insert must fail
- Update + Delete + x must fail
- UpdateInMemory + Insert must fail
- UpdateInMemory + Delete + x must fail

Deleted:
- x must fail
*/

BOOST_AUTO_TEST_CASE(Failure_Created_Update_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 1, 2); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_UpdateInMemory_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 2); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_Insert_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_Delete_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_Delete_Delete_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_Delete_Update_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Failure_Created_Delete_UpdateInMemory_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Delete_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Delete_Delete_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Delete_Update_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Delete_UpdateInMemory_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Update_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_UpdateInMemory_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Update_Delete_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_UpdateInMemory_Delete_Insert_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 3, 4); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Update_Delete_Delete_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_UpdateInMemory_Delete_Delete_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_Update_Delete_Update_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Created_Insert_UpdateInMemory_Delete_Update_Fails)
{
    SetupCreatedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Deleted_Insert_Fails)
{
    SetupDeletedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Deleted_Delete_Fails)
{
    SetupDeletedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Deleted_Update_Fails)
{
    SetupDeletedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Deleted_UpdateInMemory_Fails)
{
    SetupDeletedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr &) {},
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Delete_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Delete_Delete_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Inserted_Delete_Update_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Delete_UpdateInMemory_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Update_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 6, 6); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemory_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Update_Delete_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 6, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemory_Delete_Insert_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 5, 6); });
}

BOOST_AUTO_TEST_CASE(Inserted_Update_Delete_Delete_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 6, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemory_Delete_Delete_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Inserted_Update_Delete_Update_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 6, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 8, 8); });
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemory_Delete_Update_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 8, 8); });
}

BOOST_AUTO_TEST_CASE(Inserted_Update_Delete_UpdateInMemory_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 6, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 8); });
}

BOOST_AUTO_TEST_CASE(Inserted_UpdateInMemory_Delete_UpdateInMemory_Fails)
{
    SetupInsertedAndVerifyFailure(
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 6); Delete(ptr); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 8); });
}

/*
The remainder of this validates that given a set of transitions the correct
commit description was being generated 

This test does not validate the commit itself

Created:
- Insert: Insert (tested above)
- Insert + Update: Insert
- Insert + UpdateInMemory: Insert
- Insert + Update + Update: Insert
- Insert + UpdateInMem + Update: Insert
- Insert + Update + UpdateInMem: Insert
- Insert + Delete: Delete in memory
- Insert + Update + Delete: Delete in memory
- Insert + UpdateInMemory + Delete: Delete in memory

Inserted:
- Delete: Delete
- Update: Update
- Update + Update: Update
- Update + Delete: Delete
*/

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); },
        L"i false 1 2");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        L"d true 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); },
        L"d true 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); },
        L"i false 3 4");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); },
        L"i false 1 4");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_Update_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Delete(ptr); },
        L"d true 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_UpdateInMemory_id_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); Delete(ptr); },
        L"d true 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_Update_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Update(ptr, 5, 6); },
        L"i false 5 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_Update_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); UpdateInMemory(ptr, 6); },
        L"i false 3 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_UpdateInMemory_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); Update(ptr, 5, 6); },
        L"i false 5 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Created_Insert_UpdateInMemory_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); UpdateInMemory(ptr, 4); UpdateInMemory(ptr, 6); },
        L"i false 1 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); },
        L"d false 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); },
        L"u false 3 4");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); },
        L"u true 1 4");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_Update_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); Delete(ptr); },
        L"d false 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_UpdateInMemory_Delete)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); Delete(ptr); },
        L"d false 0 0");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_Update_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); Update(ptr, 5, 6); },
        L"u false 5 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_UpdateInMemory_Update)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); Update(ptr, 5, 6); },
        L"u false 5 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_Update_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); UpdateInMemory(ptr, 6); },
        L"u false 3 6");
}

BOOST_AUTO_TEST_CASE(CommitDescription_Inserted_UpdateInMemory_UpdateInMemory)
{
    SetupAndVerifyCommitDescription(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { UpdateInMemory(ptr, 4); UpdateInMemory(ptr, 6); },
        L"u true 1 6");
}

/*
The remainder tests validate that a reader of the locked entity reads it correctly
*/
BOOST_AUTO_TEST_CASE(Read_Created)
{
    SetupAndVerifyLockedEntityAccessorIsNull(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { ptr; });
}

BOOST_AUTO_TEST_CASE(Read_Created_Insert)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); },
        1, 2);
}

BOOST_AUTO_TEST_CASE(Read_Created_Delete_IsDeleted)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Read_Created_Insert_Delete)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Read_Created_Insert_Update)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); },
        3, 4);
}

BOOST_AUTO_TEST_CASE(Read_Created_Insert_Update_Delete)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Read_Created_Insert_Update_Update)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupCreated(); },
        [&](LockedTestEntityPtr & ptr) { Insert(ptr, 1, 2); Update(ptr, 3, 4); Update(ptr, 5, 6); },
        5, 6);
}

BOOST_AUTO_TEST_CASE(Read_Inserted)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { ptr; },
        1, 2);
}

BOOST_AUTO_TEST_CASE(Read_Inserted_Delete)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Read_Inserted_Update)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) {  Update(ptr, 3, 4); },
        3, 4);
}

BOOST_AUTO_TEST_CASE(Read_Inserted_Update_Delete)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) { Update(ptr, 3, 4); Delete(ptr); });
}

BOOST_AUTO_TEST_CASE(Read_Inserted_Update_Update)
{
    SetupAndVerifyLockedEntityAccessorHasValue(
        [&]() { return infrastructureUtility_->SetupInserted(1, 2); },
        [&](LockedTestEntityPtr & ptr) {  Update(ptr, 3, 4); Update(ptr, 5, 6); },
        5, 6);
}

BOOST_AUTO_TEST_CASE(Read_Deleted)
{
    SetupAndVerifyLockedEntityAccessorIsDeleted(
        [&]() { return infrastructureUtility_->SetupDeleted(); },
        [&](LockedTestEntityPtr &) {});
}

BOOST_AUTO_TEST_CASE(LockedEntityPtr_MultipleLocksOnTheSameEntityAssert)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    LockedTestEntityPtr lockedEntity(CreateLock(entry));

    Verify::Asserts([&]() { LockedTestEntityPtr lock2(CreateLock(entry)); }, L"Multiple locks should assert");
}

BOOST_AUTO_TEST_CASE(LockedEntityPtr_ReadLockHasTheCorrectValueOfIsDeleted)
{
    auto entry = infrastructureUtility_->SetupDeleted();

    ReadOnlyLockedTestEntityPtr lockedFT(CreateReadLock(entry));

    Verify::IsTrue(lockedFT.IsEntityDeleted, L"Entity must be deleted");
}

BOOST_AUTO_TEST_CASE(LockedEntityPtr_ReadLockHasOldValueAfterWriteLockUpdates)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    ReadOnlyLockedTestEntityPtr readLock(CreateReadLock(entry));

    Update(entry, 3, 4, PersistenceResult::Success);

    Verify::AreEqual(1, readLock.Current->PersistedData, L"Read lock value is not correct");
}

BOOST_AUTO_TEST_CASE(ReadLockDoesNotChangeOnUpdatingWriteLock)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    ReadOnlyLockedTestEntityPtr readLock(CreateReadLock(entry));

    LockedTestEntityPtr writeLock(CreateLock(entry));
    writeLock.EnableUpdate();
    Update(writeLock, 3, 5);

    Verify::AreEqual(1, readLock.Current->PersistedData, L"Read lock is not correct");
}

BOOST_AUTO_TEST_CASE(MultipleReadLocksAreAllowed)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 2);

    ReadOnlyLockedTestEntityPtr readLock1(CreateReadLock(entry));
    ReadOnlyLockedTestEntityPtr readLock2(CreateReadLock(entry));

    Verify::AreEqual(1, readLock1.Current->PersistedData, L"Read lock is not correct");
    Verify::AreEqual(1, readLock2.Current->PersistedData, L"Read lock is not correct");
}

/*
    These tests verify that the entity map asserts when an inconsistency is observed

    The RA scheduling infrastructure guarantees that only one commit happens for a row at a time.

    Thus, the following results are not possible and should assert
 */
BOOST_AUTO_TEST_CASE(Asserts_InsertCannotFailWithWriteConflict)
{
    auto entry = infrastructureUtility_->SetupCreated();

    Verify::Asserts(
        [&]() { Insert(entry, 1, 1, PersistenceResult::StoreWriteConflict); },
        L"InsertInsert");
}

BOOST_AUTO_TEST_CASE(Asserts_UpdateCannotFailWithRecordNotFound)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 1);

    Verify::Asserts(
        [&]() { Update(entry, 2, 1, PersistenceResult::StoreRecordNotFound); },
        L"Updateupdate");
}

BOOST_AUTO_TEST_CASE(Asserts_DeleteCannotFailWithRecordNotFound)
{
    auto entry = infrastructureUtility_->SetupInserted(1, 1);

    Verify::Asserts(
        [&]() { Delete(entry, PersistenceResult::StoreRecordNotFound, true); },
        L"Delete");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
