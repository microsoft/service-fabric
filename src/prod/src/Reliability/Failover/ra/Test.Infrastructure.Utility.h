// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            namespace ReliabilityUnitTest
            {
                namespace PersistenceResult
                {
                    enum Enum
                    {
                        Success,
                        Failure,
                        RAStoreNotUsable,
                        ObjectClosed,
                        StoreWriteConflict,
                        StoreRecordNotFound,
                        NotPrimary
                    };

                    inline void Set(Enum e, Storage::FaultInjectionAdapter & store)
                    {
                        auto error = Common::ErrorCodeValue::Success;
                        switch (e)
                        {
                        case PersistenceResult::Success:
                            break;

                        case PersistenceResult::Failure:
                            error = Storage::FaultInjectionAdapter::DefaultError;
                            break;

                        case PersistenceResult::RAStoreNotUsable:
                            error = Common::ErrorCodeValue::RAStoreNotUsable;
                            break;

                        case PersistenceResult::ObjectClosed:
                            error = Common::ErrorCodeValue::ObjectClosed;
                            break;

                        case PersistenceResult::StoreWriteConflict:
                            error = Common::ErrorCodeValue::StoreWriteConflict;
                            break;

                        case PersistenceResult::StoreRecordNotFound:
                            error = Common::ErrorCodeValue::StoreRecordNotFound;
                            break;

                        case PersistenceResult::NotPrimary:
                            error = Common::ErrorCodeValue::NotPrimary;
                            break;

                        default:
                            Common::Assert::CodingError("Unknwon persistence result {0}", static_cast<int>(e));
                        }

                        store.EnableFaultInjection(error);
                    }
                }

                class InfrastructureTestUtility
                {
                    DENY_COPY(InfrastructureTestUtility);
                public:
                    InfrastructureTestUtility(ReconfigurationAgentComponent::ReliabilityUnitTest::UnitTestContext & utContext);

                    static std::wstring const Key;

                    static ReconfigurationAgentComponent::ReliabilityUnitTest::UnitTestContextUPtr CreateUnitTestContextWithTestEntityMap();

                    template <typename T>
                    Common::ErrorCode WriteRecordIntoStore(
                        Storage::Api::OperationType::Enum operation,
                        std::wstring const & persistenceId,
                        T const * data)
                    {
                        return WriteRecordIntoStore<T>(*utContext_.RA.LfumStore, operation, persistenceId, data);
                    }

                    template <typename T>
                    static Common::ErrorCode WriteRecordIntoStore(
                        Storage::Api::IKeyValueStore & store,
                        Storage::Api::OperationType::Enum operation,
                        std::wstring const & persistenceId,
                        T const * data)
                    {
                        Common::ManualResetEvent ev;

                        std::vector<byte> v;

                        if (data != nullptr)
                        {
                            auto error = Common::FabricSerializer::Serialize(data, v);
                            ASSERT_IF(!error.IsSuccess(), "Expect this to succeed");
                        }

                        auto op = store.BeginStoreOperation(
                            operation,
                            Storage::Api::RowIdentifier(EntityTraits<T>::RowType, persistenceId),
                            std::move(v),
                            Common::TimeSpan::MaxValue,
                            [&](Common::AsyncOperationSPtr const &)
                            {
                                ev.Set();
                            },
                            Common::AsyncOperationSPtr());

                        ev.WaitOne();

                        return store.EndStoreOperation(op);
                    }

                    void ValidateEntityData(int expectedPersistedState, TestEntity const & actual);

                    void ValidateEntityData(int expectedPersistedState, int expectedInMemoryState, TestEntity const & actual);

                    void ValidateEntityData(std::wstring const & key, int expectedPersistedState, TestEntity const & actual);

                    void ValidateEntityData(std::wstring const & key ,int expectedPersistedState, int expectedInMemoryState, TestEntity const & actual);

                    void ValidateEntityData(int expectedPersistedState, int expectedInMemoryState, EntityEntryBaseSPtr const & entry);

                    void ValidateEntityIsNull(EntityEntryBaseSPtr const & entry);

                    void ValidateEntityIsDeleted(EntityEntryBaseSPtr const & entry);

                    TestEntitySPtr CreateEntity(int persistedState, int inMemoryState);

                    std::vector<TestEntitySPtr> GetAllEntitiesFromStore();

                    void VerifyStoreIsEmpty();

                    void VerifyStoreValue(int expected);

                    Common::ErrorCode PerformStoreOperation(
                        Storage::Api::OperationType::Enum operation,
                        TestEntity const * data);

                    Common::ErrorCode PerformInsertOperation(
                        int persistedState);

                    EntityEntryBaseSPtr SetupCreated();

                    EntityEntryBaseSPtr SetupInserted(int persistedState, int inMemoryState);

                    EntityEntryBaseSPtr SetupDeleted();

                    void Setup();

                    EntityEntryBaseSPtr GetOrCreate();

                    EntityEntryBaseSPtr GetOrCreate(std::wstring const & key);

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

                private:
                    void ExecuteUnderLockAndCommit(
                        EntityEntryBaseSPtr const & entry,
                        PersistenceResult::Enum persistenceResult,
                        bool isStorePersistenceExpected,
                        std::function<void(LockedTestEntityPtr &)> func);

                    TestEntityMap & entityMap_;
                    ReconfigurationAgentComponent::ReliabilityUnitTest::UnitTestContext & utContext_;
                };

                typedef std::unique_ptr<InfrastructureTestUtility> InfrastructureTestUtilityUPtr;
            }
        }
    }
}



