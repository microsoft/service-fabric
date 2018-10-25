// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DIFFERENTIALSTORECOMPONENT_TAG 'pcSD'

namespace Data
{
   namespace TStore
   {
      template <typename TKey, typename TValue>
      class DifferentialStoreComponent : public KObject<DifferentialStoreComponent<TKey, TValue>>, public KShared<DifferentialStoreComponent<TKey, TValue>>
      {
         K_FORCE_SHARED(DifferentialStoreComponent)

      public:
         typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;
         static NTSTATUS Create(
            __in HashFunctionType func,
            __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
            __in SnapshotContainer<TKey, TValue> & snapshotContainer,
            __in LONG64 stateProviderId,
            __in IComparer<TKey> & keyComparer,
            __in StoreTraceComponent & traceComponent,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTORECOMPONENT_TAG, allocator) DifferentialStoreComponent(func, transactionalReplicator, snapshotContainer, stateProviderId, keyComparer, traceComponent);

            if (!output)
            {
               status = STATUS_INSUFFICIENT_RESOURCES;
               return status;
            }

            status = output->Status();
            if (!NT_SUCCESS(status))
            {
               return status;
            }

            result = Ktl::Move(output);
            return STATUS_SUCCESS;
         }

         __declspec(property(get = get_SortedItemDataSPtr)) KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> SortedItemDataSPtr;
         KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> get_SortedItemDataSPtr() const
         {
            STORE_ASSERT(sortedItemDataSPtr_ != nullptr, "sortedItemDataSPtr_ should not be null");
            STORE_ASSERT(isReadOnly_ == true, "Component should be read-only");
            return sortedItemDataSPtr_;
         }

         // Best effort estimate of the size in memory for the component
         __declspec(property(get = get_Size)) LONG64 Size;
         LONG64 get_Size() const
         {
             return size_;
         }

         void DecrementSize(__in LONG64 size)
         {
            // Decrement value size on sweep.
            InterlockedAdd64(&size_, -size);

            // Disabling until #10584838 is resolved 
            //STORE_ASSERT(size_ >= 0, "Size {1} should not be negative", size_);
         }

         void Sort()
         {
            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSptr != nullptr");

            KSharedPtr<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> fastSkipListSPtr = nullptr;
            fastSkipListSPtr = static_cast<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(cachedComponentSPtr.RawPtr());
            STORE_ASSERT(fastSkipListSPtr != nullptr, "fastSkipListSPtr != nullptr");

            KSharedPtr<ReadOnlySortedList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> sortedComponentSPtr = nullptr;
            auto status = ReadOnlySortedList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>::Create(*fastSkipListSPtr, *keyComparerSPtr_, true, true, this->GetThisAllocator(), sortedComponentSPtr);
            Diagnostics::Validate(status);

            auto sortedComponentSPtr_ = sortedComponentSPtr.DownCast<IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>>();

            STORE_ASSERT(sortedComponentSPtr_ != nullptr, "sortedComponentSPtr_ != nullptr");
            componentSPtr_.Put(Ktl::Move(sortedComponentSPtr_));
            isReadOnly_ = true;
         }

         void Add(
            __in TKey& key,
            __in VersionedItem<TValue>& value,
            __in IReadableStoreComponent<TKey, TValue>& consolidationManager)
         {
             UNREFERENCED_PARAMETER(consolidationManager);
            STORE_ASSERT(isReadOnly_ == false, "isReadOnly_ == false");

            auto dictionarySPtr = componentSPtr_.Get();
            STORE_ASSERT(dictionarySPtr != nullptr, "dictionarySPtr != nullptr");

            if (value.GetRecordKind() != RecordKind::DeletedVersion)
            {
               STORE_ASSERT(value.GetValueSize() >= 0, "value size for item {1} is negative", value.GetVersionSequenceNumber());
            }

            KSharedPtr<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> fastSkipListSPtr = nullptr;
            fastSkipListSPtr = static_cast<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(dictionarySPtr.RawPtr());
            STORE_ASSERT(fastSkipListSPtr != nullptr, "fast skip list is null");

            // Check if an entry is present for this key
            KSharedPtr<DifferentialStateVersions<TValue>> differentialStateVersionsSPtr = nullptr;
            bool keyExists = fastSkipListSPtr->TryGetValue(key, differentialStateVersionsSPtr);

            if (keyExists == false)
            {
               NTSTATUS status = DifferentialStateVersions<TValue>::Create(this->GetThisAllocator(), differentialStateVersionsSPtr);
               Diagnostics::Validate(status);

               bool added = fastSkipListSPtr->TryAdd(key, differentialStateVersionsSPtr);
               STORE_ASSERT(added == true, "Unable to add key to differential store component dictionary");
            }

            if (differentialStateVersionsSPtr->get_CurrentVersion() == nullptr)
            {
               STORE_ASSERT(differentialStateVersionsSPtr->get_PreviousVersion() == nullptr, "Previous version should be null");
               InterlockedAdd64(&size_, value.GetValueSize());
               differentialStateVersionsSPtr->SetCurrentVersion(value);
            }
            else
            {
               LONG64 currentVersionSequenceNumber = differentialStateVersionsSPtr->CurrentVersionSPtr->GetVersionSequenceNumber();
               LONG64 nextVersionSequenceNumber = value.GetVersionSequenceNumber();
               STORE_ASSERT(currentVersionSequenceNumber <= nextVersionSequenceNumber, "Expected current={1} <= next={2}", currentVersionSequenceNumber, nextVersionSequenceNumber);

               if (currentVersionSequenceNumber == nextVersionSequenceNumber)
               {
                  // Update the size with the difference with the existing current item
                  InterlockedAdd64(&size_, value.GetValueSize() - differentialStateVersionsSPtr->CurrentVersionSPtr->GetValueSize());

                  STORE_ASSERT(size_ >= 0, "Size {1} should not be negative", size_);

                  // Set the latest value in case there are multiple updates in the same transaction(same lsn).
                  differentialStateVersionsSPtr->SetCurrentVersion(value);

                  return;
               }

               // Check if the previous version is null
               if (differentialStateVersionsSPtr->get_PreviousVersion() == nullptr)
               {
                  // Increase by size of new current
                  InterlockedAdd64(&size_, value.GetValueSize());

                  differentialStateVersionsSPtr->SetPreviousVersion(differentialStateVersionsSPtr->CurrentVersionSPtr);
                  differentialStateVersionsSPtr->SetCurrentVersion(value);

               }
               else
               {
                  // Move to snapshot container, if needed
                   TxnReplicator::TryRemoveVersionResult::SPtr removeVersionResult;
                   NTSTATUS status = transactionalRepliactorSPtr_->TryRemoveVersion(
                       stateProviderId_,
                       differentialStateVersionsSPtr->PreviousVersionSPtr->GetVersionSequenceNumber(),
                       differentialStateVersionsSPtr->CurrentVersionSPtr->GetVersionSequenceNumber(),
                       removeVersionResult);

                  Diagnostics::Validate(status);

                  if (!removeVersionResult->CanBeRemoved)
                  {
                     auto set = removeVersionResult->EnumerationSet;
                     auto enumerator = set->GetEnumerator();

                     STORE_ASSERT(enumerator != nullptr, "enumerator != nullptr");
                     while (enumerator->MoveNext())
                     {
                        LONG64 snapshotVisibilityLsn = enumerator->Current();

                        KSharedPtr<SnapshotComponent<TKey, TValue>> snapshotComponentSPtr = snapshotContainerSPtr_->Read(snapshotVisibilityLsn);
                        if (snapshotComponentSPtr == nullptr)
                        {
                           status = SnapshotComponent<TKey, TValue>::Create(
                               *snapshotContainerSPtr_, 
                               isValueAReferenceType_, 
                               *keyComparerSPtr_, 
                               *traceComponent_,
                               this->GetThisAllocator(), 
                               snapshotComponentSPtr);
                           Diagnostics::Validate(status);

                           snapshotComponentSPtr = snapshotContainerSPtr_->GetOrAdd(snapshotVisibilityLsn, *snapshotComponentSPtr);
                        }

                        STORE_ASSERT(snapshotComponentSPtr != nullptr, "snapshotComponentSPtr != nullptr");
                        snapshotComponentSPtr->Add(key, *differentialStateVersionsSPtr->PreviousVersionSPtr);
                     }

                     // Wait on notifications and remove the entry from the container.
                     if (removeVersionResult->EnumerationCompletionNotifications != nullptr)
                     {
                        auto lEnumerator = removeVersionResult->EnumerationCompletionNotifications->GetEnumerator();

                        while (lEnumerator->MoveNext())
                        {
                           auto enumerationResult = lEnumerator->Current();
                           STORE_ASSERT(enumerationResult != nullptr, "(enumerationResult != nullptr");
                           ktl::Task task = RemoveFromContainer(*enumerationResult);
                           STORE_ASSERT(task.IsTaskStarted(), "Remove from snapshot container task should have started");
                        }
                     }
                  }

                  // Remove from differential state

                  // Increase by size of new current, decrease by size of old previous
                  InterlockedAdd64(&size_, value.GetValueSize() - differentialStateVersionsSPtr->PreviousVersionSPtr->GetValueSize());

                  STORE_ASSERT(size_ >= 0, "Size {1} should not be negative", size_);

                  differentialStateVersionsSPtr->SetPreviousVersion(differentialStateVersionsSPtr->CurrentVersionSPtr);
                  differentialStateVersionsSPtr->SetCurrentVersion(value);
               }
            }

         }

         bool UndoFalseProgress(__in TKey & key, __in LONG64 sequenceNumber, __in StoreModificationType::Enum storeModificationType)
         {
            STORE_ASSERT(isReadOnly_ == false, "Write operation cannot come after the differential has become readonly.");

            // Find the item sequence number
            KSharedPtr<DifferentialStateVersions<TValue>> differentialStateVersions = ReadVersions(key);

            // Remove item
            KSharedPtr<VersionedItem<TValue>> undoItem = nullptr;
            if (differentialStateVersions != nullptr)
            {
               KSharedPtr<VersionedItem<TValue>> previousVersion = differentialStateVersions->PreviousVersionSPtr;
               if (previousVersion != nullptr && previousVersion->GetVersionSequenceNumber() == sequenceNumber)
               {
                  undoItem = previousVersion;
                  differentialStateVersions->SetPreviousVersion(nullptr);
               }
               else
               {
                  KSharedPtr<VersionedItem<TValue>> currentVersion = differentialStateVersions->CurrentVersionSPtr;
                  STORE_ASSERT(currentVersion != nullptr, "current version cannot be null");
                  if (currentVersion->GetVersionSequenceNumber() == sequenceNumber)
                  {
                     undoItem = currentVersion;

                     // Update current version and make prev version null
                     if (previousVersion != nullptr)
                     {
                        differentialStateVersions->SetCurrentVersion(*previousVersion);
                        differentialStateVersions->SetPreviousVersion(nullptr);
                     }
                     else
                     {
                        RemoveKey(key);
                     }
                  }
               }

               if (undoItem != nullptr)
               {
                  switch (storeModificationType)
                  {
                  case StoreModificationType::Enum::Add:
                  {
                     // Remove implies undoing insert
                     KSharedPtr<InsertedVersionedItem<TValue>> insertedVersion = dynamic_cast<InsertedVersionedItem<TValue> *>(undoItem.RawPtr());
                     STORE_ASSERT(insertedVersion != nullptr, "unexpected deleted version");
                     break;
                  }
                  case StoreModificationType::Enum::Remove:
                  {
                     // Add implies undoing delete
                     KSharedPtr<DeletedVersionedItem<TValue>> deletedVersion = dynamic_cast<DeletedVersionedItem<TValue> *>(undoItem.RawPtr());
                     STORE_ASSERT(deletedVersion != nullptr, "unexpected inserted version");
                     break;
                  }
                  case StoreModificationType::Enum::Update:
                  {
                     // Try casting to an UpdatedVersionedItem first
                     KSharedPtr<UpdatedVersionedItem<TValue>> updatedVersion = dynamic_cast<UpdatedVersionedItem<TValue> *>(undoItem.RawPtr());
                     if (updatedVersion == nullptr)
                     {
                        // Since that didn't work, try casting to an inserted version next
                        KSharedPtr<InsertedVersionedItem<TValue>> insertedVersion = dynamic_cast<InsertedVersionedItem<TValue> *>(undoItem.RawPtr());
                        STORE_ASSERT(insertedVersion != nullptr, "unexpected update version");
                     }
                     break;
                  }
                  }
                  return false;
               }
            }

            return true;
         }

         ktl::Task RemoveFromContainer(__in TxnReplicator::EnumerationCompletionResult & completionResult)
         {
            KShared$ApiEntry();
            TxnReplicator::EnumerationCompletionResult::SPtr completionSPtr = &completionResult;

            try
            {
                ktl::Awaitable<LONG64> notification = completionSPtr->Notification;
                LONG64 visibilitySequenceNumber = co_await notification;
                co_await snapshotContainerSPtr_->RemoveAsync(visibilitySequenceNumber);

                co_return;
            }
            catch (ktl::Exception const & e)
            {
                AssertException(L"RemoveFromContainer", e);
            }
         }

         KSharedPtr<DifferentialStateVersions<TValue>> ReadVersions(__in TKey& key) const
         {
            KSharedPtr<DifferentialStateVersions<TValue>> differentialStateVersionSPtr;

            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            cachedComponentSPtr->TryGetValue(key, differentialStateVersionSPtr);
            return differentialStateVersionSPtr;
         }

         KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key) const
         {
            KSharedPtr<DifferentialStateVersions<TValue>> differentialStateVersionsPtr;
            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            cachedComponentSPtr->TryGetValue(key, differentialStateVersionsPtr);

            if (differentialStateVersionsPtr != nullptr)
            {
               return differentialStateVersionsPtr->CurrentVersionSPtr;
            }

            return nullptr;
         }

         KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key, __in LONG64 visibilityLsn) const
         {
            KSharedPtr<VersionedItem<TValue>> versionedItem = nullptr;
            KSharedPtr<VersionedItem<TValue>> potentialVersionedItem = nullptr;

            auto differentialStateVersionsSPtr = ReadVersions(key);

            if (differentialStateVersionsSPtr != nullptr)
            {
               potentialVersionedItem = differentialStateVersionsSPtr->CurrentVersionSPtr;

               if (potentialVersionedItem != nullptr && potentialVersionedItem->GetVersionSequenceNumber() <= visibilityLsn)
               {
                  versionedItem = potentialVersionedItem;
               }
               else
               {
                  potentialVersionedItem = differentialStateVersionsSPtr->PreviousVersionSPtr;

                  if (potentialVersionedItem != nullptr && potentialVersionedItem->GetVersionSequenceNumber() <= visibilityLsn)
                  {
                     versionedItem = potentialVersionedItem;
                  }
               }
            }

            return versionedItem;
         }

         bool ContainsKey(__in TKey& key) const
         {
            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            return cachedComponentSPtr->ContainsKey(key);
         }

         void RemoveKey(__in TKey& key)
         {
            STORE_ASSERT(isReadOnly_ == false, "isReadOnly_ == false");

            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            KSharedPtr<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> concurrentDictionarySPtr = nullptr;
            concurrentDictionarySPtr = static_cast<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(cachedComponentSPtr.RawPtr());
            STORE_ASSERT(concurrentDictionarySPtr != nullptr, "concurrent dictionary is null");

            KSharedPtr<DifferentialStateVersions<TValue>> differentialStateVersionsPtr;
            bool result = concurrentDictionarySPtr->TryRemove(key, differentialStateVersionsPtr);

            STORE_ASSERT(result, "remove key failed from differential state");
         }

         KSharedPtr<DifferentialKeysEnumerator<TKey, TValue>> GetKeys() const
         {
            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            KSharedPtr<DifferentialKeysEnumerator<TKey, TValue>> differentialStateEnumeratorSPtr = nullptr;
            NTSTATUS status = DifferentialKeysEnumerator<TKey, TValue>::Create(*cachedComponentSPtr, this->GetThisAllocator(), differentialStateEnumeratorSPtr);
            Diagnostics::Validate(status);
            return differentialStateEnumeratorSPtr;
         }

         KSharedPtr<DifferentialKeyValueEnumerator<TKey, TValue>> GetKeyAndValues() const
         {
            auto cachedComponentSPtr = componentSPtr_.Get();
            STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

            KSharedPtr<DifferentialKeyValueEnumerator<TKey, TValue>> differentialKeyValueEnumeratorSPtr = nullptr;
            NTSTATUS status = DifferentialKeyValueEnumerator<TKey, TValue>::Create(*cachedComponentSPtr, this->GetThisAllocator(), differentialKeyValueEnumeratorSPtr);
            Diagnostics::Validate(status);
            return differentialKeyValueEnumeratorSPtr;
         }

         KSharedPtr<IFilterableEnumerator<TKey>> GetEnumerableNewKeys() const
         {
             auto cachedComponentSPtr = componentSPtr_.Get();
             STORE_ASSERT(cachedComponentSPtr != nullptr, "cachedComponentSPtr != nullptr");

             if (!isReadOnly_)
             {
                 KSharedPtr<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> fastSkipListSPtr = nullptr;
                 fastSkipListSPtr = static_cast<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(cachedComponentSPtr.RawPtr());
                 STORE_ASSERT(fastSkipListSPtr != nullptr, "sorted list is null");

                 return fastSkipListSPtr->GetKeys();
             }

             KSharedPtr<ReadOnlySortedList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> sortedListSPtr = nullptr;
             sortedListSPtr = static_cast<ReadOnlySortedList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(cachedComponentSPtr.RawPtr());
             STORE_ASSERT(sortedListSPtr != nullptr, "sorted list is null");

             return sortedListSPtr->GetKeys();
         }

         bool isReadOnlyListNonEmpty()
         {
            STORE_ASSERT(isReadOnly_ == true, "isReadOnly_ == true");

            auto componentSPtr = componentSPtr_.Get();
            STORE_ASSERT(componentSPtr != nullptr, "componentSPtr != nullptr");

            return componentSPtr->Count > 0;
         }

         void Close()
         {
            transactionalRepliactorSPtr_ = nullptr;
         }

         LONG64 Count()
         {
            auto componentSPtr = componentSPtr_.Get();
            STORE_ASSERT(componentSPtr != nullptr, "componentSPtr != nullptr");

            return static_cast<LONG64>(componentSPtr->Count);
         }

         void TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception)
         {
             KDynStringA stackString(this->GetThisAllocator());
             Diagnostics::GetExceptionStackTrace(exception, stackString);
             StoreEventSource::Events->DifferentialStoreComponentException(
                 traceComponent_->PartitionId,
                 traceComponent_->TraceTag,
                 ToStringLiteral(methodName),
                 ToStringLiteral(stackString),
                 exception.GetStatus());
         }

         void AssertException(__in KStringView const & methodName, __in ktl::Exception const & exception)
         {
             KDynStringA stackString(this->GetThisAllocator());
             Diagnostics::GetExceptionStackTrace(exception, stackString);
             STORE_ASSERT(
                 false,
                 "UnexpectedException: Message: {2} Code:{4}\nStack: {3}",
                 ToStringLiteral(methodName),
                 ToStringLiteral(stackString),
                 exception.GetStatus());
         }


      private:
         DifferentialStoreComponent(
            __in HashFunctionType func,
            __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
            __in SnapshotContainer<TKey, TValue> & snapshotContainer,
            __in LONG64 stateProviderId,
            __in IComparer<TKey> & keyComparer,
            __in StoreTraceComponent & traceComponent);

         TxnReplicator::ITransactionalReplicator::SPtr transactionalRepliactorSPtr_;
         KSharedPtr<SnapshotContainer<TKey, TValue>> snapshotContainerSPtr_;
         bool isReadOnly_;
         ThreadSafeSPtrCache<IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> componentSPtr_;

         KSharedPtr<IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> sortedItemDataSPtr_;
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
         LONG64 stateProviderId_;
        
         LONG64 size_;

         // todo: hard-coded for now
         bool isValueAReferenceType_ = false;

         StoreTraceComponent::SPtr traceComponent_;
      };

      template <typename TKey, typename TValue>
      DifferentialStoreComponent<TKey, TValue>::DifferentialStoreComponent(
         __in HashFunctionType func,
         __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
         __in SnapshotContainer<TKey, TValue> & snapshotContainer,
         __in LONG64 stateProviderId,
         __in IComparer<TKey> & keyComparer,
         __in StoreTraceComponent & traceComponent)
         :transactionalRepliactorSPtr_(&transactionalReplicator),
         snapshotContainerSPtr_(&snapshotContainer),
         isReadOnly_(false),
         componentSPtr_(nullptr),
         keyComparerSPtr_(&keyComparer),
         stateProviderId_(stateProviderId),
         size_(0),
         traceComponent_(&traceComponent)
      {
         UNREFERENCED_PARAMETER(func);
         KSharedPtr<FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> fastSkipListSPtr = nullptr;
         NTSTATUS status = FastSkipList<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>::Create(
            keyComparerSPtr_,
            this->GetThisAllocator(),
            fastSkipListSPtr);

         if (!NT_SUCCESS(status))
         {
            this->SetConstructorStatus(status);
            return;
         }

         KSharedPtr<IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>> componentSPtr;
         componentSPtr = static_cast<IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> *>(fastSkipListSPtr.RawPtr());
         componentSPtr_.Put(Ktl::Move(componentSPtr));

         this->SetConstructorStatus(status);
      }

      template <typename TKey, typename TValue>
      DifferentialStoreComponent<TKey, TValue>::~DifferentialStoreComponent()
      {
      }
   }
}
