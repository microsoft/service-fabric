// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Collections;
    using namespace TStoreTests;
    using namespace TxnReplicator;

    #define RCQTESTBASE_TAG 'tbCQ'

    // @TODO - This should inherit from StoreTestBase
    template<typename TValue>
    class ReliableConcurrentQueueTestBase
    {
    public:
        __declspec(property(get = get_RCQ)) KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> RCQ;
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> get_RCQ() const
        {
            return rcqSPtr_;
        }

        __declspec(property(get = get_Replicator)) KSharedPtr<MockTransactionalReplicator> Replicator;
        KSharedPtr<MockTransactionalReplicator> get_Replicator() const
        {
            return mockReplicatorSPtr_;
        }

        __declspec(property(get = get_RCQs)) KSharedPtr<KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>> RCQs;
        KSharedPtr<KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>> get_RCQs() const
        {
            return storesSPtr_;
        }
        ReliableConcurrentQueueTestBase(ULONG replicaCount = 1)
        {
            Setup(replicaCount);
        }

        ~ReliableConcurrentQueueTestBase()
        {
            Cleanup();
        }

        void Setup(ULONG replicaCount)
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);

            storesSPtr_ = _new(RCQTESTBASE_TAG, GetAllocator()) KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>();
            secondaryStoresSPtr_ = _new(RCQTESTBASE_TAG, GetAllocator()) KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>();
            secondariesReplicatorSPtr_ = _new(RCQTESTBASE_TAG, GetAllocator()) KSharedArray<MockTransactionalReplicator::SPtr>();

            mockReplicatorSPtr_ = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
            mockReplicatorSPtr_->SetReadable(true);

            // Set up the work folder.
            auto currentFolderPath = CreateTempDirectory();

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KUriView name(L"fabric:/RCQ");

            rcqSPtr_ = CreateRCQ(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);
            storesSPtr_->Append(rcqSPtr_);

            IStateProvider2::SPtr stateProviderSPtr(rcqSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(rcqSPtr_->Name, *stateProviderSPtr);
            SyncAwait(mockReplicatorSPtr_->OpenAsync());

            for (ULONG32 i = 0; i < replicaCount - 1; i++)
            {
                auto secondaryFolderPath = CreateTempDirectory();
                CreateSecondary(name, 5, *secondaryFolderPath, nullptr, nullptr, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            }
        }

        void Cleanup()
        {
            //close all secondaries.
            for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
            {
                SyncAwait((*secondariesReplicatorSPtr_)[i]->RemoveStateAndCloseAsync());
            }

            SyncAwait(mockReplicatorSPtr_->RemoveStateAndCloseAsync());

            rcqSPtr_ = __nullptr;
            storesSPtr_ = nullptr;
            secondaryStoresSPtr_ = nullptr;
            secondariesReplicatorSPtr_ = nullptr;
            mockReplicatorSPtr_ = nullptr;

            ktlSystem_->Shutdown();
        }

        KSharedPtr<TxnReplicator::Transaction> CreateReplicatorTransaction()
        {
            return Transaction::CreateTransaction(*mockReplicatorSPtr_, GetAllocator());
        }

        KSharedPtr<TxnReplicator::Transaction> CreateReplicatorTransaction(__in Data::TStore::Store<LONG64, TValue>& store)
        {
            CODING_ERROR_ASSERT(store.TransactionalReplicatorSPtr);
            return Transaction::CreateTransaction(*store.TransactionalReplicatorSPtr, GetAllocator());
        }

        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> CreateRCQ(
            __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef, 
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder, 
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children)
        {
            UNREFERENCED_PARAMETER(initializationContext);

            KAllocator& allocator = GetAllocator();

            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = nullptr;

            typename TestStateSerializer<TValue>::SPtr longSerializer = nullptr;
            NTSTATUS status = TestStateSerializer<TValue>::Create(allocator, longSerializer);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KUriView nameCast = const_cast<KUriView &>(name);
            status = Data::Collections::ReliableConcurrentQueue<TValue>::Create(
                allocator,
                *GetPartitionedReplicaId(),
                nameCast,
                stateProviderId,
                *longSerializer,
                rcq);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            rcq->Initialize(transactionalReplicatorWRef, workFolder, children);

            return rcq;
        }

        void CopyCheckpointToSecondary(__in Data::Collections::ReliableConcurrentQueue<TValue> & secondaryStore)
        {
            // Get the current state from the Primary
            auto copyState = Store->GetCurrentState();

            // Start copying the state to the Secondary
            SyncAwait(secondaryStore.BeginSettingCurrentStateAsync());

            // Copy all state to the Secondary
            LONG64 stateRecordNumber = 0;
            while (true)
            {
                auto operationData = SyncAwait(copyState->GetNextAsync(ktl::CancellationToken::None));
                if (operationData == nullptr)
                {
                    copyState->Dispose();
                    break; // Done with copy
                }

                SyncAwait(secondaryStore.SetCurrentStateAsync(stateRecordNumber, *operationData, ktl::CancellationToken::None));
                stateRecordNumber++;
            }

            SyncAwait(secondaryStore.EndSettingCurrentStateAsync(ktl::CancellationToken::None));
        }

        void Checkpoint()
        {
            for (ULONG32 i = 0; i < storesSPtr_->Count(); i++)
            {
                Checkpoint(*(*storesSPtr_)[i]);
            }
        }

        LONG64 Checkpoint(__in Data::Collections::ReliableConcurrentQueue<TValue>& store)
        {
            LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
            PerformCheckpoint(store, checkpointLSN);

            return checkpointLSN;
        }

        void PerformCheckpoint(
            __in Data::Collections::ReliableConcurrentQueue<TValue>& store,
            __in LONG64 checkpointLSN)
        {
            store.PrepareCheckpoint(checkpointLSN);
            SyncAwait(store.PerformCheckpointAsync(ktl::CancellationToken::None));
            SyncAwait(store.CompleteCheckpointAsync(ktl::CancellationToken::None));
        }

        void StoresPrepareCheckpoint()
        {
            LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                (*storesSPtr_)[i]->PrepareCheckpoint(checkpointLSN);
            }
        }

        void PerformCheckpointOnStores()
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                SyncAwait((*storesSPtr_)[i]->PerformCheckpointAsync(ktl::CancellationToken::None));
            }
        }

        void CompleteCheckpointOnStores()
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                SyncAwait((*storesSPtr_)[i]->CompleteCheckpointAsync(ktl::CancellationToken::None));
            }
        }

        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> CloseAndReOpenSecondary(__in Data::Collections::ReliableConcurrentQueue<TValue>& store)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &store;

            for (ULONG i = 0; i < secondaryStoresSPtr_->Count(); i++)
            {
                if (storeSPtr == (*secondaryStoresSPtr_)[i])
                {                
                    secondaryStoresSPtr_->Remove(i);
                    break;
                }
            }

            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                if (storeSPtr == (*storesSPtr_)[i])
                {
                    storesSPtr_->Remove(i);
                    break;
                }
            }

            for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
            {
                if (storeSPtr->TransactionalReplicatorSPtr == (*secondariesReplicatorSPtr_)[i])
                {
                    secondariesReplicatorSPtr_->Remove(i);
                    break;
                }
            }

            SyncAwait(storeSPtr->CloseAsync(ktl::CancellationToken::None));
            return CreateSecondary(storeSPtr->Name, storeSPtr->StateProviderId, *storeSPtr->WorkingDirectoryCSPtr, nullptr, nullptr);
        }

        void RemoveStateAndReopenStore()
        {
            auto currentFolderPath = rcqSPtr_->WorkingDirectoryCSPtr;
            SyncAwait(mockReplicatorSPtr_->RemoveStateAndCloseAsync());
            if (!Common::Directory::Exists(currentFolderPath->operator LPCWSTR()))
            {
                Common::ErrorCode errorCode = Common::Directory::Create2(currentFolderPath->operator LPCWSTR());
                if (errorCode.IsSuccess() == false)
                {
                    throw ktl::Exception(errorCode.ToHResult());
                }
            }

            KUriView name(L"fabric:/Store");

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            rcqSPtr_ = CreateStore(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);
            IStateProvider2::SPtr stateProviderSPtr(rcqSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(rcqSPtr_->Name, *stateProviderSPtr);
            SyncAwait(Store->OpenAsync(ktl::CancellationToken::None));
            SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, ktl::CancellationToken::None));
        }

        LONG64 CloseAndReOpenStore()
        {
            KArray<KString::CSPtr> workingFolders(this->GetAllocator());
            auto snappedReplicaCount = storesSPtr_->Count();

            //
            // Close all RCQ instances (primary and secondary)
            //
            for (ULONG i = 0; i < storesSPtr_->Count(); ++i)
            {
                auto rcq = (*storesSPtr_)[i];
                workingFolders.Append(rcq->WorkingDirectoryCSPtr);
                SyncAwait(rcq->CloseAsync(ktl::CancellationToken::None));
            }

            //
            // Clear secondary replicas
            //
            rcqSPtr_ = nullptr;
            storesSPtr_->Clear();
            secondaryStoresSPtr_->Clear();
            secondariesReplicatorSPtr_->Clear();
            mockReplicatorSPtr_->ClearSecondary();

            //
            // Recreate the RCQ
            //
            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KUriView name(L"fabric:/RCQ");

            rcqSPtr_ = CreateRCQ(*tmpWRef, name, 5, *workingFolders[0], nullptr, nullptr);

            IStateProvider2::SPtr stateProviderSPtr(rcqSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(rcqSPtr_->Name, *stateProviderSPtr);
            storesSPtr_->Append(rcqSPtr_);

            //
            // Time recovery
            //
            Common::Stopwatch stopwatch;
            stopwatch.Start();
            SyncAwait(mockReplicatorSPtr_->OpenAsync());
            stopwatch.Stop();

            LONG64 recoveryTime = stopwatch.ElapsedMilliseconds;

            //
            // create secondary replicator and queues
            //
            for (ULONG32 i = 1; i < snappedReplicaCount; i++)
            {
                auto secondary = CreateSecondary(name, 5, *workingFolders[i], nullptr, nullptr, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            }

            mockReplicatorSPtr_->UpdateSecondaryLSN();

            return recoveryTime;
        }

        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> CreateSecondary()
        {
            auto workFolder = CreateTempDirectory();
            return CreateSecondary(Store->Name, rcqSPtr_->StateProviderId, *workFolder, nullptr, nullptr);
        }

        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> CreateSecondary(
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder,
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children,
            __in FABRIC_REPLICA_ROLE role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            //create secondary replicator.
            auto secondaryReplicator = CreateMockReplicator(role);
            secondaryReplicator->SetReadable(true);
            secondariesReplicatorSPtr_->Append(secondaryReplicator);

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = secondaryReplicator->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            //create secondary store.
            auto secondaryRcqSPtr = CreateRCQ(*tmpWRef, name, stateProviderId, workFolder, initializationContext, children);
            storesSPtr_->Append(secondaryRcqSPtr);
            secondaryStoresSPtr_->Append(secondaryRcqSPtr);

            //add the secondary store.
            IStateProvider2::SPtr stateProviderSPtr(secondaryRcqSPtr.RawPtr());
            secondaryReplicator->RegisterStateProvider(secondaryRcqSPtr->Name, *stateProviderSPtr);

            //register the secondary replicator.
            mockReplicatorSPtr_->RegisterSecondary(*secondaryReplicator);

            //open secondary replicator.
            SyncAwait(secondaryReplicator->OpenAsync());

            return secondaryRcqSPtr;
        }

        PartitionedReplicaId::SPtr GetPartitionedReplicaId()
        {
            KAllocator& allocator = GetAllocator();
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            return PartitionedReplicaId::Create(guid, replicaId, allocator);
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

        KSharedPtr<MockTransactionalReplicator> CreateMockReplicator(FABRIC_REPLICA_ROLE role) //TODO: IStatefulServicePartition& partition
        {
            KAllocator& allocator = GetAllocator();
            MockTransactionalReplicator::SPtr replicator = nullptr;
            NTSTATUS status = MockTransactionalReplicator::Create(allocator, /*hasPersistedState:*/true, replicator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            replicator->Initialize(*GetPartitionedReplicaId());
            replicator->Role = role;
            return replicator;
        }

        KString::SPtr CreateGuidString()
        {
            KGuid guid;
            guid.CreateNew();
            KString::SPtr stringSPtr = nullptr;
            NTSTATUS status = KString::Create(stringSPtr, GetAllocator(), KStringView::MaxGuidString);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            BOOLEAN result = stringSPtr->FromGUID(guid);
            CODING_ERROR_ASSERT(result == TRUE);

            return stringSPtr;
        }

        KString::CSPtr CreateFileString(__in KStringView const & name)
        {
            KString::SPtr fileName;
            KAllocator& allocator = GetAllocator();

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        KString::CSPtr CreateTempDirectory()
        {
            // TODO: Create directories based on partition and replica ID
            KString::SPtr fileName = CreateGuidString();
            KString::CSPtr currentFolderPath = CreateFileString(*fileName);
            Common::ErrorCode errorCode = Common::Directory::Create2(currentFolderPath->operator LPCWSTR());
            if (errorCode.IsSuccess() == false)
            {
                throw ktl::Exception(errorCode.ToHResult());
            }
            return currentFolderPath;
        }

        Awaitable<void> EnqueueItem(TValue value)
        {
            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

            {
                KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
                KFinally([&] { transactionSPtr->Dispose(); });

                co_await rcq->EnqueueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                co_await transactionSPtr->CommitAsync();
            }
        }

        void SyncEnqueueItem(TValue value)
        {
            SyncAwait(EnqueueItem(value));
        }

        Awaitable<bool> DequeueItem(TValue &value)
        {
            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

            {
                KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
                KFinally([&] { transactionSPtr->Dispose(); });

                bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));            
                co_await transactionSPtr->CommitAsync();

                co_return succeeded;
            }
        }

        bool SyncDequeueItem(TValue &value)
        {
            return SyncAwait(DequeueItem(value));
        }

        void SyncExpectDequeue(TValue expectedValue)
        {
            SyncAwait(ExpectDequeue(expectedValue));
        }

        void  SyncExpectEmpty()
        {
            SyncAwait(ExpectEmpty());
        }

        Awaitable<void> ExpectDequeue(TValue expectedValue)
        {
            TValue value;
            bool succeeded = co_await DequeueItem(value);
            CODING_ERROR_ASSERT(succeeded);
            CODING_ERROR_ASSERT(value == expectedValue);
        }

        Awaitable<void> ExpectEmpty()
        {
            TValue value;
            bool succeeded = co_await DequeueItem(value);
            CODING_ERROR_ASSERT(!succeeded);
        }

        Awaitable<TValue> GetValueAsync(
            __in Data::TStore::Store<LONG64, TValue>& store,
            __in StoreTransaction<LONG64, TValue>& tx,
            __in LONG64 key)
        {
            KeyValuePair<LONG64, TValue> kvpair(-1, TValue());
            bool result = co_await store.ConditionalGetAsync(tx, key, Common::TimeSpan::MaxValue, kvpair, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(result);
            co_return kvpair.Value;
        }

        KSharedPtr<WriteTransaction<LONG64, TValue>> CreateWriteTransaction()
        {
            return CreateWriteTransaction(*storeSPtr_);
        }

        KSharedPtr<WriteTransaction<LONG64, TValue>> CreateWriteTransaction(__in Data::TStore::Store<LONG64, TValue>& store)
        {
            Transaction::SPtr tx = CreateReplicatorTransaction(store);
            KSharedPtr<StoreTransaction<LONG64, TValue>> storeTransaction = store.CreateStoreTransaction(*tx);

            KSharedPtr<WriteTransaction<LONG64, TValue>> writeTransactionSPtr = nullptr;
            NTSTATUS status = WriteTransaction<LONG64, TValue>::Create(store, *tx, *storeTransaction, GetAllocator(), writeTransactionSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return writeTransactionSPtr;
        }


    private:
        KSharedPtr<KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>> storesSPtr_ = nullptr;
        KSharedPtr<KSharedArray<KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>>>> secondaryStoresSPtr_ = nullptr;
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> rcqSPtr_ = nullptr;

        KSharedPtr<TStoreTests::MockTransactionalReplicator> mockReplicatorSPtr_ = nullptr;

        KSharedArray<MockTransactionalReplicator::SPtr>::SPtr secondariesReplicatorSPtr_;

        KtlSystem* ktlSystem_;
    };
}
