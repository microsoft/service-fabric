// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STORETESTBASE_TAG 'tbTS'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::TStore;
    using namespace Data::Utilities;
    using namespace TxnReplicator;

    template<typename TKey, typename TValue, typename TKeyComparer, typename TKeySerializer, typename TValueSerializer>
    class TStoreTestBase
    {
    public:
        const Common::TimeSpan DefaultTimeout = Common::TimeSpan::FromSeconds(4);

        typedef KDelegate<bool(TValue & one, TValue & two)> ValueEqualityFunctionType;

        __declspec(property(get = get_Replicator)) KSharedPtr<MockTransactionalReplicator> Replicator;
        KSharedPtr<MockTransactionalReplicator> get_Replicator() const
        {
            return mockReplicatorSPtr_;
        }

        __declspec(property(get = get_Store)) KSharedPtr<Data::TStore::Store<TKey, TValue>> Store;
        KSharedPtr<Data::TStore::Store<TKey, TValue>> get_Store() const
        {
            return storeSPtr_;
        }

        __declspec(property(get = get_Stores)) KSharedPtr<KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>> Stores;
        KSharedPtr<KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>> get_Stores() const
        {
            return storesSPtr_;
        }

        __declspec(property(get = get_SecondaryReplicators)) KSharedPtr<KSharedArray<MockTransactionalReplicator::SPtr>> SecondaryReplicators;
        KSharedPtr<KSharedArray<MockTransactionalReplicator::SPtr>> get_SecondaryReplicators() const
        {
            return secondariesReplicatorSPtr_;
        }

        void SetupKtl()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
            ktlInitialized_ = true;
        }

        virtual void Setup(
            ULONG replicaCount = 1, 
            KDelegate<ULONG(const TKey & Key)> hashFunc = DefaultHash,
            KString::CSPtr startDirectory = nullptr,
            bool hasPersistedState = true)
        {
            CODING_ERROR_ASSERT(replicaCount >= 1);
            NTSTATUS status;

            if (!ktlInitialized_)
            {
                SetupKtl();
            }

            storesSPtr_ = _new(STORETESTBASE_TAG, GetAllocator()) KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>();
            secondaryStoresSPtr_ = _new(STORETESTBASE_TAG, GetAllocator()) KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>();
            secondariesReplicatorSPtr_ = _new(STORETESTBASE_TAG, GetAllocator()) KSharedArray<MockTransactionalReplicator::SPtr>();

            //Dictionary<KSharedPtr<Data::TStore::Store<TKey, TValue>>, KSharedPtr<MockTransactionalReplicator>>::Create(storeReplicatorMappingSPtr_, GetAllocator());

            hashFunction_ = hashFunc;
            KUriView name(L"fabric:/Store");

            //create primary replicator
            mockReplicatorSPtr_ = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY, hasPersistedState);
            mockReplicatorSPtr_->SetReadable(true);

            // Set up the work folder.
            KString::CSPtr currentFolderPath = nullptr;

            if (hasPersistedState == false)
            {
                // Volatile store scenario
                // Create a filename for compatibility with existing APIs. 
                KString::SPtr fileName = CreateGuidString();
                currentFolderPath = CreateFileString(*fileName);
            }
            else if (startDirectory != nullptr)
            {
                currentFolderPath = startDirectory;
            }
            else
            {
                currentFolderPath = CreateTempDirectory();
            }

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Instantiate primary
            storeSPtr_ = CreateStore(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);
            IStateProvider2::SPtr stateProviderSPtr(storeSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(storeSPtr_->Name, *stateProviderSPtr);
            storesSPtr_->Append(storeSPtr_);
            SyncAwait(mockReplicatorSPtr_->OpenAsync());

            //create secondary replicator and stores.
            for (ULONG32 i = 0; i < replicaCount - 1; i++)
            {
                KString::CSPtr secondaryFolderPath = hasPersistedState ? CreateTempDirectory() : currentFolderPath;
                CreateSecondary(name, 5, *secondaryFolderPath, nullptr, nullptr, mockReplicatorSPtr_->HasPeristedState ,FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            }
        }

        void Cleanup(bool removeStateForStores = true)
        {
           //close all secondaries.
           for (ULONG i = 0; i < secondariesReplicatorSPtr_->Count(); i++)
           {
              SyncAwait((*secondariesReplicatorSPtr_)[i]->RemoveStateAndCloseAsync(!removeStateForStores));
           }

           //close primary.
           SyncAwait(mockReplicatorSPtr_->RemoveStateAndCloseAsync(!removeStateForStores));

           storeSPtr_ = nullptr;
           secondaryStoresSPtr_ = nullptr;
           secondariesReplicatorSPtr_ = nullptr;
           storesSPtr_ = nullptr;
           mockReplicatorSPtr_ = nullptr;
           ktlSystem_->Shutdown();
        };

        // For tests that call Close manually
        // Responsibility of the test to clean up files
        void CleanupWithoutClose()
        {
           storeSPtr_ = nullptr;
           secondaryStoresSPtr_ = nullptr;
           secondariesReplicatorSPtr_ = nullptr;
           storesSPtr_ = nullptr;
           mockReplicatorSPtr_ = nullptr;
           ktlSystem_->Shutdown();
        }

      ktl::Awaitable<void> ChangeRole(
          __in Data::TStore::Store<TKey, TValue>& store,
          __in FABRIC_REPLICA_ROLE newRole)
      {
          co_await store.ChangeRoleAsync(newRole, ktl::CancellationToken::None);
      }

      Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyAddOperationAsync(
          __in Data::TStore::Store<TKey, TValue>& store,
          __in TKey key,
          __in TValue value,
          __in LONG64 commitSequenceNumber,
          __in TxnReplicator::ApplyContext::Enum applyContext)
      {
          auto transactionSPtr = Transaction::CreateTransaction(*mockReplicatorSPtr_, GetAllocator());

          auto keyBytes = store.GetKeyBytes(key);
          auto valueBytes = store.GetValueBytes(value);

          KSharedPtr<MetadataOperationDataKV<TKey, TValue> const> metadataKCSPtr = nullptr;
          NTSTATUS status = MetadataOperationDataKV<TKey, TValue>::Create(
              key,
              value,
              Constants::SerializedVersion,
              StoreModificationType::Enum::Add,
              transactionSPtr->TransactionId,
              keyBytes,
              GetAllocator(),
              metadataKCSPtr);
          Diagnostics::Validate(status);

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          status = RedoUndoOperationData::Create(GetAllocator(), valueBytes, nullptr, redoDataSPtr);
          Diagnostics::Validate(status);

          OperationData::CSPtr metadataCSPtr = static_cast<const OperationData* const>(metadataKCSPtr.RawPtr());

          transactionSPtr->CommitSequenceNumber = commitSequenceNumber;
          auto result = co_await store.ApplyAsync(commitSequenceNumber, *transactionSPtr, applyContext, metadataCSPtr.RawPtr(), redoDataSPtr.RawPtr());
          co_return result;
      }

      Awaitable<TxnReplicator::OperationContext::CSPtr> ApplyRemoveOperationAsync(
          __in Data::TStore::Store<TKey, TValue>& store,
          __in TKey key,
          __in TValue value,
          __in LONG64 commitSequenceNumber,
          __in TxnReplicator::ApplyContext::Enum applyContext)
      {
          auto transactionSPtr = Transaction::CreateTransaction(*mockReplicatorSPtr_, GetAllocator());

          auto keyBytes = store.GetKeyBytes(key);
          auto valueBytes = store.GetValueBytes(value);

          KSharedPtr<MetadataOperationDataK<TKey> const> metadataKCSPtr = nullptr;
          NTSTATUS status = MetadataOperationDataK<TKey>::Create(
              key,
              Constants::SerializedVersion,
              StoreModificationType::Enum::Remove,
              transactionSPtr->TransactionId,
              keyBytes,
              GetAllocator(),
              metadataKCSPtr);
          Diagnostics::Validate(status);

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          status = RedoUndoOperationData::Create(GetAllocator(), valueBytes, nullptr, redoDataSPtr);
          Diagnostics::Validate(status);

          OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
          OperationData::CSPtr metadataCSPtr = static_cast<const OperationData* const>(metadataKCSPtr.RawPtr());

          transactionSPtr->CommitSequenceNumber = commitSequenceNumber;
          return store.ApplyAsync(commitSequenceNumber, *transactionSPtr, applyContext, metadataCSPtr.RawPtr(), redoSPtr.RawPtr());
      }

      void CopyCheckpointToSecondary(__in Data::TStore::Store<TKey, TValue> & secondaryStore)
      {
          // Get the current state from the Primary
          auto copyState = Store->GetCurrentState();

          // Start copying the state to the Secondary
          SyncAwait(secondaryStore.BeginSettingCurrentStateAsync());

          // Copy all state to the Secondary
          LONG64 stateRecordNumber = 0;
          while (true)
          {
              OperationData::CSPtr operationData;
              SyncAwait(copyState->GetNextAsync(ktl::CancellationToken::None, operationData));
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

      ktl::Awaitable<void> FullCopyToSecondaryAsync(__in Data::TStore::Store<TKey, TValue> & secondaryStore)
      {
          // Checkpoint the Primary, to get all current state
          co_await CheckpointAsync(*Store);
          co_await CopyCheckpointToSecondaryAsync(secondaryStore);
          co_return;
      }
        
      ktl::Awaitable<void> CopyCheckpointToSecondaryAsync(__in Data::TStore::Store<TKey, TValue> & secondaryStore)
      {
          // Get the current state from the Primary
          auto copyState = Store->GetCurrentState();

          // Start copying the state to the Secondary
          co_await secondaryStore.BeginSettingCurrentStateAsync();

          // Copy all state to the Secondary
          LONG64 stateRecordNumber = 0;
          while (true)
          {
              OperationData::CSPtr operationData;
              co_await copyState->GetNextAsync(ktl::CancellationToken::None, operationData);
              if (operationData == nullptr)
              {
                  copyState->Dispose();
                  break; // Done with copy
              }

              co_await secondaryStore.SetCurrentStateAsync(stateRecordNumber, *operationData, ktl::CancellationToken::None);
              stateRecordNumber++;
          }

          co_await secondaryStore.EndSettingCurrentStateAsync(ktl::CancellationToken::None);
          co_return;
      }

      void Checkpoint(__in bool waitForConsolidation = true)
      {
          SyncAwait(CheckpointAsync(waitForConsolidation));
      }

      LONG64 Checkpoint(__in Data::TStore::Store<TKey, TValue> & store, __in bool waitForConsolidation = true)
      {
          return SyncAwait(CheckpointAsync(store, waitForConsolidation));
      }

      ktl::Awaitable<void> CheckpointAsync(__in bool waitForConsolidation = true)
      {
          for (ULONG32 i = 0; i < storesSPtr_->Count(); i++)
          {
              co_await CheckpointAsync(*(*storesSPtr_)[i], waitForConsolidation);
          }
      }

      ktl::Awaitable<LONG64> CheckpointAsync(__in Data::TStore::Store<TKey, TValue> & store, __in bool waitForConsolidation = true)
      {
          LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
          co_await PerformCheckpointAsync(store, checkpointLSN);

          if (waitForConsolidation && store.EnableBackgroundConsolidation)
          {
              auto consolidationTcsSPtr = store.ConsolidationTcs;
              if (consolidationTcsSPtr != nullptr)
              {
                  co_await consolidationTcsSPtr->GetAwaitable();
              }

              checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
              co_await PerformCheckpointAsync(store, checkpointLSN);

              consolidationTcsSPtr = store.ConsolidationTcs;
              if (consolidationTcsSPtr != nullptr)
              {
                  co_await consolidationTcsSPtr->GetAwaitable();
              }
          }

          co_return checkpointLSN;
      }

      void PerformCheckpoint(
          __in Data::TStore::Store<TKey, TValue>& store,
          __in LONG64 checkpointLSN)
      {
          SyncAwait(PerformCheckpointAsync(store, checkpointLSN));
      }

      ktl::Awaitable<void> PerformCheckpointAsync(
          __in Data::TStore::Store<TKey, TValue>& store,
          __in LONG64 checkpointLSN)
      {
          bool acquired = false;
          if (Replicator->ShouldSynchronizePrepareAndApply)
          {
              acquired = co_await Replicator->PrepareApplyLockSPtr->AcquireWriteLockAsync(10'000);
          }

          store.PrepareCheckpoint(checkpointLSN);

          if (acquired)
          {
              Replicator->PrepareApplyLockSPtr->ReleaseWriteLock();
          }

          co_await store.PerformCheckpointAsync(ktl::CancellationToken::None);
          co_await store.CompleteCheckpointAsync(ktl::CancellationToken::None);
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

        ktl::Awaitable<void> PerformCheckpointOnStoresAsync()
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                co_await (*storesSPtr_)[i]->PerformCheckpointAsync(ktl::CancellationToken::None);
            }
            co_return;
        }

      void CompleteCheckpointOnStores()
      {
          for (ULONG i = 0; i < storesSPtr_->Count(); i++)
          {
              SyncAwait((*storesSPtr_)[i]->CompleteCheckpointAsync(ktl::CancellationToken::None));
          }
      }

      ktl::Awaitable<void> CompleteCheckpointOnStoresAsync()
      {
          for (ULONG i = 0; i < storesSPtr_->Count(); i++)
          {
              co_await (*storesSPtr_)[i]->CompleteCheckpointAsync(ktl::CancellationToken::None);
          }
          co_return;
      }

      KSharedPtr<Data::TStore::Store<TKey, TValue>> CloseAndReOpenSecondary(__in Data::TStore::Store<TKey, TValue>& store)
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

      ktl::Awaitable<KSharedPtr<Data::TStore::Store<TKey, TValue>>> CloseAndReOpenSecondaryAsync(__in Data::TStore::Store<TKey, TValue>& store)
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

          co_await storeSPtr->CloseAsync(ktl::CancellationToken::None);
          co_return co_await CreateSecondaryAsync(storeSPtr->Name, storeSPtr->StateProviderId, *storeSPtr->WorkingDirectoryCSPtr, nullptr, nullptr);
      }

        void RemoveStateAndReopenStore()
        {
            auto currentFolderPath = storeSPtr_->WorkingDirectoryCSPtr;
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
            storeSPtr_ = CreateStore(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);
            IStateProvider2::SPtr stateProviderSPtr(storeSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(storeSPtr_->Name, *stateProviderSPtr);
            SyncAwait(Store->OpenAsync(ktl::CancellationToken::None));
            SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, ktl::CancellationToken::None));

            (*storesSPtr_)[0] = storeSPtr_; // Replace the old closed primary store with the new one
        }

        ktl::Awaitable<void> RemoveStateAndReopenStoreAsync()
        {
            auto currentFolderPath = storeSPtr_->WorkingDirectoryCSPtr;
            co_await mockReplicatorSPtr_->RemoveStateAndCloseAsync();
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
            storeSPtr_ = CreateStore(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);
            IStateProvider2::SPtr stateProviderSPtr(storeSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(storeSPtr_->Name, *stateProviderSPtr);
            co_await Store->OpenAsync(ktl::CancellationToken::None);
            co_await Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, ktl::CancellationToken::None);

            (*storesSPtr_)[0] = storeSPtr_; // Replace the old closed primary store with the new one

            co_return;
        }

        LONG64 CloseAndReOpenStore(KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> changeHandlerSPtr, DictionaryChangeEventMask::Enum mask = DictionaryChangeEventMask::Enum::All)
        {
            KArray<KString::CSPtr> workingFolders(this->GetAllocator());

            bool snappedShouldLoadValuesOnRecovery = storeSPtr_->ShouldLoadValuesOnRecovery;

            auto snappedReplicaCount = storesSPtr_->Count();
            // Close
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                auto store = (*storesSPtr_)[i];
                workingFolders.Append(store->WorkingDirectoryCSPtr);
                SyncAwait(store->CloseAsync(ktl::CancellationToken::None));
            }

            storeSPtr_ = nullptr;
            secondaryStoresSPtr_->Clear();
            storesSPtr_->Clear();
            secondariesReplicatorSPtr_->Clear();
            mockReplicatorSPtr_->ClearSecondary();

            KUriView name(L"fabric:/Store");

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Instantiate primary      
            storeSPtr_ = CreateStore(*tmpWRef, name, 5, *workingFolders[0], nullptr, nullptr);
            storeSPtr_->DictionaryChangeHandlerSPtr = changeHandlerSPtr;
            storeSPtr_->DictionaryChangeHandlerMask = mask;
            storeSPtr_->ShouldLoadValuesOnRecovery = snappedShouldLoadValuesOnRecovery;

            IStateProvider2::SPtr stateProviderSPtr(storeSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(storeSPtr_->Name, *stateProviderSPtr);
            storesSPtr_->Append(storeSPtr_);

            // Time recovery
            Common::Stopwatch stopwatch;
            stopwatch.Start();
            SyncAwait(mockReplicatorSPtr_->OpenAsync());
            stopwatch.Stop();

            LONG64 recoveryTime = stopwatch.ElapsedMilliseconds;

            //create secondary replicator and stores.
            for (ULONG32 i = 1; i < snappedReplicaCount; i++)
            {
                auto secondary = CreateSecondary(name, 5, *workingFolders[i], nullptr, nullptr, mockReplicatorSPtr_->HasPeristedState, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, changeHandlerSPtr, mask);
            }

            mockReplicatorSPtr_->UpdateSecondaryLSN();

            return recoveryTime;
        }

        ktl::Awaitable<LONG64> CloseAndReOpenStoreAsync(KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> changeHandlerSPtr, DictionaryChangeEventMask::Enum mask = DictionaryChangeEventMask::Enum::All)
        {
            KArray<KString::CSPtr> workingFolders(this->GetAllocator());

            bool snappedShouldLoadValuesOnRecovery = storeSPtr_->ShouldLoadValuesOnRecovery;

            auto snappedReplicaCount = storesSPtr_->Count();
            // Close
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                auto store = (*storesSPtr_)[i];
                workingFolders.Append(store->WorkingDirectoryCSPtr);
                co_await store->CloseAsync(ktl::CancellationToken::None);
            }

            storeSPtr_ = nullptr;
            secondaryStoresSPtr_->Clear();
            storesSPtr_->Clear();
            secondariesReplicatorSPtr_->Clear();
            mockReplicatorSPtr_->ClearSecondary();

            KUriView name(L"fabric:/Store");

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Instantiate primary      
            storeSPtr_ = CreateStore(*tmpWRef, name, 5, *workingFolders[0], nullptr, nullptr);
            storeSPtr_->DictionaryChangeHandlerSPtr = changeHandlerSPtr;
            storeSPtr_->DictionaryChangeHandlerMask = mask;
            storeSPtr_->ShouldLoadValuesOnRecovery = snappedShouldLoadValuesOnRecovery;

            IStateProvider2::SPtr stateProviderSPtr(storeSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(storeSPtr_->Name, *stateProviderSPtr);
            storesSPtr_->Append(storeSPtr_);

            // Time recovery
            Common::Stopwatch stopwatch;
            stopwatch.Start();
            co_await mockReplicatorSPtr_->OpenAsync();
            stopwatch.Stop();

            LONG64 recoveryTime = stopwatch.ElapsedMilliseconds;

            //create secondary replicator and stores.
            for (ULONG32 i = 1; i < snappedReplicaCount; i++)
            {
                auto secondary = co_await CreateSecondaryAsync(name, 5, *workingFolders[i], nullptr, nullptr, mockReplicatorSPtr_->HasPeristedState, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, changeHandlerSPtr, mask);
            }

            mockReplicatorSPtr_->UpdateSecondaryLSN();

            co_return recoveryTime;
        }

        LONG64 CloseAndReOpenStore()
        {
           return CloseAndReOpenStore(nullptr);
        }

        ktl::Awaitable<LONG64> CloseAndReOpenStoreAsync()
        {
            return CloseAndReOpenStoreAsync(nullptr);
        }

        KSharedPtr<Data::TStore::Store<TKey, TValue>> CreateStore(
            __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder,
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children)
        {
            UNREFERENCED_PARAMETER(initializationContext);

            KAllocator& allocator = GetAllocator();

            KSharedPtr<TKeyComparer> comparerSPtr = nullptr;
            NTSTATUS status = TKeyComparer::Create(GetAllocator(), comparerSPtr);
            Diagnostics::Validate(status);

            typename TKeySerializer::SPtr keyStateSerializer = nullptr;
            typename TValueSerializer::SPtr valueStateSerializer = nullptr;

            status = TKeySerializer::Create(allocator, keyStateSerializer);
            Diagnostics::Validate(status);
            status = TValueSerializer::Create(allocator, valueStateSerializer);
            Diagnostics::Validate(status);

            typename Data::TStore::Store<TKey, TValue>::SPtr storeSPtr = nullptr;

            KUriView nameCast = const_cast<KUriView &>(name);
            status = Data::TStore::Store<TKey, TValue>::Create(
                *GetPartitionedReplicaId(), 
                *comparerSPtr, 
                hashFunction_, 
                allocator, 
                nameCast, 
                stateProviderId,
                *keyStateSerializer, 
                *valueStateSerializer, 
                storeSPtr);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            storeSPtr->Initialize(transactionalReplicatorWRef, workFolder, children);

            return storeSPtr;
        }

        KSharedPtr<Data::TStore::Store<TKey, TValue>> CreateSecondary(
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder,
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children,
            __in bool hasPersistedState = true,
            __in FABRIC_REPLICA_ROLE role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            __in_opt KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> changeHandlerSPtr = nullptr,
            __in_opt DictionaryChangeEventMask::Enum mask = DictionaryChangeEventMask::Enum::All)
        {
            //create secondary replicator.
            auto secondaryReplicator = CreateMockReplicator(role, hasPersistedState);
            secondaryReplicator->SetReadable(true);
            secondariesReplicatorSPtr_->Append(secondaryReplicator);

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = secondaryReplicator->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            //create secondary store.
            auto secondaryStoreSPtr = CreateStore(*tmpWRef, name, stateProviderId, workFolder, initializationContext, children);
            secondaryStoreSPtr->DictionaryChangeHandlerSPtr = changeHandlerSPtr;
            secondaryStoreSPtr->DictionaryChangeHandlerMask = mask;
            secondaryStoresSPtr_->Append(secondaryStoreSPtr);
            storesSPtr_->Append(secondaryStoreSPtr);

            //add the secondary store.
            IStateProvider2::SPtr stateProviderSPtr(secondaryStoreSPtr.RawPtr());
            secondaryReplicator->RegisterStateProvider(secondaryStoreSPtr->Name, *stateProviderSPtr);

            //register the secondary replicator.
            mockReplicatorSPtr_->RegisterSecondary(*secondaryReplicator);

            //open secondary replicator.
            SyncAwait(secondaryReplicator->OpenAsync());

            return secondaryStoreSPtr;
        }

        ktl::Awaitable<KSharedPtr<Data::TStore::Store<TKey, TValue>>> CreateSecondaryAsync(
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder,
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children,
            __in bool hasPersistedState = true,
            __in FABRIC_REPLICA_ROLE role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            __in_opt KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> changeHandlerSPtr = nullptr,
            __in_opt DictionaryChangeEventMask::Enum mask = DictionaryChangeEventMask::Enum::All)
        {
            //create secondary replicator.
            auto secondaryReplicator = CreateMockReplicator(role, hasPersistedState);
            secondaryReplicator->SetReadable(true);
            secondariesReplicatorSPtr_->Append(secondaryReplicator);

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            NTSTATUS status = secondaryReplicator->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            //create secondary store.
            auto secondaryStoreSPtr = CreateStore(*tmpWRef, name, stateProviderId, workFolder, initializationContext, children);
            secondaryStoreSPtr->DictionaryChangeHandlerSPtr = changeHandlerSPtr;
            secondaryStoreSPtr->DictionaryChangeHandlerMask = mask;
            secondaryStoresSPtr_->Append(secondaryStoreSPtr);
            storesSPtr_->Append(secondaryStoreSPtr);

            //add the secondary store.
            IStateProvider2::SPtr stateProviderSPtr(secondaryStoreSPtr.RawPtr());
            secondaryReplicator->RegisterStateProvider(secondaryStoreSPtr->Name, *stateProviderSPtr);

            //register the secondary replicator.
            mockReplicatorSPtr_->RegisterSecondary(*secondaryReplicator);

            //open secondary replicator.
            co_await secondaryReplicator->OpenAsync();

            co_return secondaryStoreSPtr;
        }

        KSharedPtr<Data::TStore::Store<TKey, TValue>> CreateSecondary()
        {
            bool hasPersistedState = mockReplicatorSPtr_->HasPeristedState;
            KString::CSPtr workFolder = hasPersistedState ? CreateTempDirectory() : Store->WorkingDirectoryCSPtr;
            return CreateSecondary(Store->Name, storeSPtr_->StateProviderId, *workFolder, nullptr, nullptr, hasPersistedState);
        }

        ktl::Awaitable<KSharedPtr<Data::TStore::Store<TKey, TValue>>> CreateSecondaryAsync()
        {
            bool hasPersistedState = mockReplicatorSPtr_->HasPeristedState;
            KString::CSPtr workFolder = hasPersistedState ? CreateTempDirectory() : Store->WorkingDirectoryCSPtr;
            co_return co_await CreateSecondaryAsync(Store->Name, storeSPtr_->StateProviderId, *workFolder, nullptr, nullptr, hasPersistedState);
        }

        KSharedPtr<Data::TStore::Store<TKey, TValue>> CloseAndRecoverSecondary(
            __in Data::TStore::Store<TKey, TValue> & secondaryStore,
            __in MockTransactionalReplicator & secondaryReplicator)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &secondaryStore;
            MockTransactionalReplicator::SPtr replicatorSPtr = &secondaryReplicator;

            auto currentFolderPath = secondaryStore.WorkingDirectoryCSPtr;

            SyncAwait(storeSPtr->CloseAsync(ktl::CancellationToken::None));
            TxnReplicator::Transaction::SPtr transactionSPtr = CreateReplicatorTransaction();
            SyncAwait(replicatorSPtr->RemoveAsync(*transactionSPtr, storeSPtr->Name, DefaultTimeout, ktl::CancellationToken::None));

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            auto status = replicatorSPtr->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto newSecondarySPtr = CreateStore(*tmpWRef, storeSPtr->Name, 5, *currentFolderPath, nullptr, nullptr);
            replicatorSPtr->RegisterStateProvider(newSecondarySPtr->Name, *newSecondarySPtr);

            SyncAwait(newSecondarySPtr->OpenAsync(ktl::CancellationToken::None));
            SyncAwait(newSecondarySPtr->RecoverCheckpointAsync(ktl::CancellationToken::None));
            SyncAwait(newSecondarySPtr->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, ktl::CancellationToken::None));
            SyncAwait(newSecondarySPtr->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ktl::CancellationToken::None));

            return newSecondarySPtr;
        }

        ktl::Awaitable<KSharedPtr<Data::TStore::Store<TKey, TValue>>> CloseAndRecoverSecondaryAsync(
            __in Data::TStore::Store<TKey, TValue> & secondaryStore,
            __in MockTransactionalReplicator & secondaryReplicator)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &secondaryStore;
            MockTransactionalReplicator::SPtr replicatorSPtr = &secondaryReplicator;

            auto currentFolderPath = secondaryStore.WorkingDirectoryCSPtr;

            co_await storeSPtr->CloseAsync(ktl::CancellationToken::None);
            TxnReplicator::Transaction::SPtr transactionSPtr = CreateReplicatorTransaction();
            co_await replicatorSPtr->RemoveAsync(*transactionSPtr, storeSPtr->Name, DefaultTimeout, ktl::CancellationToken::None);

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            auto status = replicatorSPtr->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto newSecondarySPtr = CreateStore(*tmpWRef, storeSPtr->Name, 5, *currentFolderPath, nullptr, nullptr);
            replicatorSPtr->RegisterStateProvider(newSecondarySPtr->Name, *newSecondarySPtr);

            co_await newSecondarySPtr->OpenAsync(ktl::CancellationToken::None);
            co_await newSecondarySPtr->RecoverCheckpointAsync(ktl::CancellationToken::None);
            co_await newSecondarySPtr->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, ktl::CancellationToken::None);
            co_await newSecondarySPtr->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ktl::CancellationToken::None);

            co_return newSecondarySPtr;
        }

        KSharedPtr<StoreTransaction<TKey, TValue>> CreateTransaction()
        {
            KAllocator& allocator = GetAllocator();
            auto transactionId = InterlockedIncrement64(&transactionId_);
            typename StoreTransaction<TKey, TValue>::SPtr storeTxn = nullptr;

            // todo: once store id is generated, remove this workaround.
            LONG64 storeId = 1;
            NTSTATUS status = StoreTransaction<TKey, TValue>::Create(
                transactionId, 
                storeId, 
                *storeSPtr_->KeyComparerSPtr, 
                *Store->TraceComponent,
                allocator, 
                storeTxn);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            return storeTxn;
        }

        KSharedPtr<TxnReplicator::Transaction> CreateReplicatorTransaction(__in Data::TStore::Store<TKey, TValue>& store)
        {
            CODING_ERROR_ASSERT(store.TransactionalReplicatorSPtr);
            return Transaction::CreateTransaction(*store.TransactionalReplicatorSPtr, GetAllocator());
        }


        KSharedPtr<TxnReplicator::Transaction> CreateReplicatorTransaction()
        {
            return Transaction::CreateTransaction(*mockReplicatorSPtr_, GetAllocator());
        }

        __declspec(noinline) // Necessary to work around issue in compiler retail builds
        KSharedPtr<WriteTransaction<TKey, TValue>> CreateWriteTransaction()
        {
            return CreateWriteTransaction(*storeSPtr_);
        }

        __declspec(noinline) // Necessary to work around issue in compiler retail builds
        KSharedPtr<WriteTransaction<TKey, TValue>> CreateWriteTransaction(__in Data::TStore::Store<TKey, TValue>& store)
        {
            Transaction::SPtr tx = CreateReplicatorTransaction(store);
            KSharedPtr<StoreTransaction<TKey, TValue>> storeTransaction = store.CreateStoreTransaction(*tx);

            KSharedPtr<WriteTransaction<TKey, TValue>> writeTransactionSPtr = nullptr;
            NTSTATUS status = WriteTransaction<TKey, TValue>::Create(store, *tx, *storeTransaction, GetAllocator(), writeTransactionSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return writeTransactionSPtr;
        }

        PartitionedReplicaId::SPtr GetPartitionedReplicaId()
        {
            KAllocator& allocator = GetAllocator();
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            return PartitionedReplicaId::Create(guid, replicaId, allocator);
        }

        void VerifyCount(__in LONG64 expectedCount = -1)
        {
            VerifyCount(*Store, expectedCount);
        }

        void VerifyCount(__in Data::TStore::Store<TKey, TValue> & store, __in LONG64 expectedCount = -1)
        {
            auto actualCount = store.Count;
            CODING_ERROR_ASSERT(expectedCount >= -1);
            CODING_ERROR_ASSERT(actualCount >= 0);

            if (expectedCount != -1)
            {
                CODING_ERROR_ASSERT(actualCount == expectedCount);
            }
        }

        void VerifyCountInAllStores(__in LONG64 expectedCount = -1)
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                VerifyCount(*(*storesSPtr_)[i], expectedCount);
            }
        }

        Awaitable<void> VerifyKeyExistsAsync(
            __in Data::TStore::Store<TKey, TValue>& store,
            __in StoreTransaction<TKey, TValue>& tx,
            __in TKey key,
            __in TValue defaultValue,
            __in TValue expectedValue,
            __in ValueEqualityFunctionType equalityFunction = DefaultEqualityFunction)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &store;
            KSharedPtr<StoreTransaction<TKey, TValue>> txSPtr = &tx;
            
            bool exists = co_await storeSPtr->ContainsKeyAsync(*txSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(exists);
                
            KeyValuePair<LONG64, TValue> kvpair(-1, defaultValue);
            bool result = co_await storeSPtr->ConditionalGetAsync(*txSPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(result);
            TValue value = kvpair.Value;
            CODING_ERROR_ASSERT(equalityFunction(expectedValue, value));
            co_return;
        }

        Awaitable<void> VerifyKeyExistsAsync(
            __in Data::TStore::Store<TKey, TValue>& store,
            __in TKey key,
            __in TValue defaultValue,
            __in TValue expectedValue,
            __in ValueEqualityFunctionType equalityFunction = DefaultEqualityFunction)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &store;

            KeyValuePair<LONG64, TValue> kvpair(-1, defaultValue);

            // Repeatable read
            KSharedPtr<StoreTransaction<TKey, TValue>> storeTxn1SPtr = CreateTransaction();
            storeTxn1SPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::ReadRepeatable;
            bool exists = co_await storeSPtr->ContainsKeyAsync(*storeTxn1SPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(exists);
            bool result = co_await storeSPtr->ConditionalGetAsync(*storeTxn1SPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(result);
            TValue value = kvpair.Value;
            CODING_ERROR_ASSERT(equalityFunction(expectedValue, value));

            storeTxn1SPtr->Unlock();

            // Read committed
            //KSharedPtr<StoreTransaction<TKey, TValue>> storeTxn2SPtr = CreateTransaction();
            //storeTxn2SPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::ReadCommitted;
            //result = co_await store.ConditionalGetAsync(*storeTxn2SPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            //CODING_ERROR_ASSERT(result);
            //value = kvpair.Value;
            //CODING_ERROR_ASSERT(equalityFunction(expectedValue, value));
            //storeTxn2SPtr->Unlock();
            co_return;
        }

        Awaitable<void> VerifyKeyExistsInStoresAsync(
            __in TKey key,
            __in TValue defaultValue,
            __in TValue expectedValue,
            __in ValueEqualityFunctionType equalityFunction = DefaultEqualityFunction)
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                co_await VerifyKeyExistsAsync(*(*storesSPtr_)[i], key, defaultValue, expectedValue, equalityFunction);
            }
        }

        Awaitable<void> VerifyKeyDoesNotExistAsync(
            __in Data::TStore::Store<TKey, TValue>& store,
            __in StoreTransaction<TKey, TValue>& tx,
            __in TKey key)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &store;
            KSharedPtr<StoreTransaction<TKey, TValue>> txSPtr = &tx;

            bool exists = co_await storeSPtr->ContainsKeyAsync(*txSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(exists == false);
            
            KeyValuePair<LONG64, TValue> kvpair;
            bool result = co_await storeSPtr->ConditionalGetAsync(*txSPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(result == false);
            co_return;
        }

        Awaitable<void> VerifyKeyDoesNotExistAsync(
            __in Data::TStore::Store<TKey, TValue>& store,
            __in TKey key)
        {
            KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr = &store;

            KeyValuePair<LONG64, TValue> kvpair;

            // Repeatable read
            KSharedPtr<StoreTransaction<TKey, TValue>> storeTxn1SPtr = CreateTransaction();
            storeTxn1SPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::ReadRepeatable;
            bool exists = co_await storeSPtr->ContainsKeyAsync(*storeTxn1SPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(exists == false);
            bool result = co_await storeSPtr->ConditionalGetAsync(*storeTxn1SPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(result == false);
            storeTxn1SPtr->Unlock();

            // Read committed
            //KSharedPtr<StoreTransaction<TKey, TValue>> storeTxn2SPtr = CreateTransaction();
            //storeTxn2SPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Enum::ReadCommitted;
            //result = co_await store.ConditionalGetAsync(*storeTxn2SPtr, key, DefaultTimeout, kvpair, ktl::CancellationToken::None);
            //CODING_ERROR_ASSERT(result == false);
            //storeTxn2SPtr->Unlock();
            co_return;
        }

        Awaitable<void> VerifyKeyDoesNotExistInStoresAsync(__in TKey key)
        {
            for (ULONG i = 0; i < storesSPtr_->Count(); i++)
            {
                co_await VerifyKeyDoesNotExistAsync(*(*storesSPtr_)[i], key);
            }
        }

        Awaitable<void> TraceMemoryUsagePeriodically(
            __in Common::TimeSpan period,  
            __in bool useMegabytes,
            __in CancellationToken cancellationToken)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
            while (!cancellationToken.IsCancellationRequested)
            {
                LONG64 processMemory = Diagnostics::GetProcessMemoryUsageBytes();
                LONG64 bufferSize = -1;

                KSharedPtr<ConsolidationManager<TKey, TValue>> consolidationManagerSPtr = this->Store->ConsolidationManagerSPtr;
                if (consolidationManagerSPtr != nullptr)
                { 
                    bufferSize = Store->GetMemorySize();
                }

                if (useMegabytes)
                {
                    Trace.WriteInfo("Memory", "Count={0} keys; Process Memory={1} MB; Buffer Size={2} MB", Store->Count, processMemory >> 20, bufferSize >> 20);
                }
                else
                {
                    Trace.WriteInfo("Memory", "Count={0} keys; Process Memory={1} KB; Buffer Size={2} KB", Store->Count, processMemory >> 10, bufferSize >> 10);
                }

                co_await KTimer::StartTimerAsync(GetAllocator(), STORETESTBASE_TAG, static_cast<ULONG>(period.TotalMilliseconds()), nullptr);
            }
        }

        KSharedPtr<MockTransactionalReplicator> CreateMockReplicator(__in FABRIC_REPLICA_ROLE role, __in bool hasPersistedState) //TODO: IStatefulServicePartition& partition
        {
            KAllocator& allocator = GetAllocator();
            MockTransactionalReplicator::SPtr replicator = nullptr;
            NTSTATUS status = MockTransactionalReplicator::Create(allocator, hasPersistedState, replicator);
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

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        static bool DefaultEqualityFunction(TValue & one, TValue & two)
        {
            return one == two;
        }

        template <typename KeyType>
        static ULONG DefaultHash(__in KeyType const & key)
        {
            return K_DefaultHashFunction(key);
        }

        template <>
        static ULONG DefaultHash(__in int const & key)
        {
            return K_DefaultHashFunction(LONGLONG(key));
        }

        template <>
        static ULONG DefaultHash(__in KBuffer::SPtr const & key)
        {
            return KBufferComparer::Hash(key);
        }


    private:

        KSharedArray<MockTransactionalReplicator::SPtr>::SPtr secondariesReplicatorSPtr_ = nullptr;
        KSharedPtr<MockTransactionalReplicator> mockReplicatorSPtr_ = nullptr;
        KSharedPtr<Data::TStore::Store<TKey, TValue>> storeSPtr_ = nullptr;
        KSharedPtr<KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>> secondaryStoresSPtr_ = nullptr;
        KSharedPtr<KSharedArray<KSharedPtr<Data::TStore::Store<TKey, TValue>>>> storesSPtr_ = nullptr;
        KSharedPtr<Dictionary<KSharedPtr<Data::TStore::Store<TKey, TValue>>, KSharedPtr<MockTransactionalReplicator>>> storeReplicatorMappingSPtr_ = nullptr;
        KtlSystem* ktlSystem_;
        bool ktlInitialized_ = false;
        LONG64 transactionId_ = 0;
        KDelegate<ULONG(const TKey & Key)> hashFunction_;
    };
}

