// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define AGGREGATEDSTATE_TAG 'tsGA'

namespace Data
{
   namespace TStore
   {
      template <typename TKey, typename TValue>
      class AggregatedStoreComponent : public KObject<AggregatedStoreComponent<TKey, TValue>>, public KShared<AggregatedStoreComponent<TKey, TValue>>
      {
         K_FORCE_SHARED(AggregatedStoreComponent)

      public:
         typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;

         static NTSTATUS Create(
            __in IComparer<TKey>& keyComparer,
            __in StoreTraceComponent& traceComponent,
            __in KAllocator& allocator,
            __out SPtr& result)
         {
            NTSTATUS status;
            SPtr output = _new(AGGREGATEDSTATE_TAG, allocator) AggregatedStoreComponent(keyComparer, traceComponent);

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

         static NTSTATUS Create(
            __in ConsolidatedStoreComponent<TKey, TValue>& consolidatedStoreComponent,
            __in IComparer<TKey> & keyComparer,
            __in StoreTraceComponent& traceComponent,
            __in KAllocator& allocator,
            __out SPtr& result)
         {
            NTSTATUS status;
            SPtr output = _new(AGGREGATEDSTATE_TAG, allocator) AggregatedStoreComponent(consolidatedStoreComponent, keyComparer, traceComponent);

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

         __declspec(property(get = get_Index)) ULONG32 Index;
         ULONG32 get_Index() const
         {
            return index_;
         }

         __declspec(property(get = get_DeltaDifferentialStateList)) KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>> DeltaDifferentialStateList;
         KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>> get_DeltaDifferentialStateList() const
         {
            return deltaDifferentialStateListSPtr_;
         }

         KSharedPtr<ConsolidatedStoreComponent<TKey, TValue>> GetConsolidatedState() const
         {
            return consolidatedStoreComponentSPtr_;
         }

         KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key) const
         {
            KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;

            KSharedPtr<DeltaEnumerator<TKey, TValue>> deltaEnumerator = nullptr;
            NTSTATUS status = DeltaEnumerator<TKey, TValue>::Create(*DeltaDifferentialStateList, index_, this->GetThisAllocator(), deltaEnumerator);
            Diagnostics::Validate(status);

            while (deltaEnumerator->MoveNext())
            {
               auto deltaState = deltaEnumerator->Current();
               ASSERT_IFNOT(deltaState != nullptr, "deltaState != nullptr");

               versionedItemSPtr = deltaState->Read(key);

               if (versionedItemSPtr != nullptr)
               {
                  return versionedItemSPtr;
               }
            }

            versionedItemSPtr = consolidatedStoreComponentSPtr_->Read(key);
            return versionedItemSPtr;
         }

         KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key, __in LONG64 visbilityLsn) const
         {
            KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;

            KSharedPtr<DeltaEnumerator<TKey, TValue>> deltaEnumerator = nullptr;
            NTSTATUS status = DeltaEnumerator<TKey, TValue>::Create(*DeltaDifferentialStateList, index_, this->GetThisAllocator(), deltaEnumerator);
            Diagnostics::Validate(status);

            while (deltaEnumerator->MoveNext())
            {
               auto deltaState = deltaEnumerator->Current();
               ASSERT_IFNOT(deltaState != nullptr, "deltaState != nullptr");

               versionedItemSPtr = deltaState->Read(key, visbilityLsn);

               if (versionedItemSPtr != nullptr)
               {
                  return versionedItemSPtr;
               }
            }

            versionedItemSPtr = consolidatedStoreComponentSPtr_->Read(key, visbilityLsn);

            return versionedItemSPtr;
         }

         bool ContainsKey(__in TKey& key) const
         {
            KSharedPtr<DeltaEnumerator<TKey, TValue>> deltaEnumerator = nullptr;
            NTSTATUS status = DeltaEnumerator<TKey, TValue>::Create(*DeltaDifferentialStateList, index_, this->GetThisAllocator(), deltaEnumerator);
            Diagnostics::Validate(status);

            while (deltaEnumerator->MoveNext())
            {
               auto deltaState = deltaEnumerator->Current();
               ASSERT_IFNOT(deltaState != nullptr, "deltaState != nullptr");

               if (deltaState->ContainsKey(key))
               {
                  return true;
               }
            }

            return consolidatedStoreComponentSPtr_->ContainsKey(key);
         }

         LONG64 Count() const
         {
            LONG64 count = 0;
            auto deltaDifferentialStateListEnumerator = deltaDifferentialStateListSPtr_->GetEnumerator();

            while (deltaDifferentialStateListEnumerator->MoveNext())
            {
               auto currentItem = deltaDifferentialStateListEnumerator->Current();
               auto currentComponent = currentItem.Value;
               count = count + currentComponent->Count();
            }

            return count + consolidatedStoreComponentSPtr_->Count();
         }

         LONG64 GetMemorySize(__in LONG64 estimatedKeySize) const
         {
            LONG64 size = 0;
            auto deltaDifferentialStateListEnumerator = deltaDifferentialStateListSPtr_->GetEnumerator();

            while (deltaDifferentialStateListEnumerator->MoveNext())
            {
                auto currentItem = deltaDifferentialStateListEnumerator->Current();
                auto componentSPtr = currentItem.Value;
                size += componentSPtr->Size;
                size += (estimatedKeySize + Constants::ValueMetadataSizeBytes) * componentSPtr->Count();
            }

            return size + consolidatedStoreComponentSPtr_->Size + (estimatedKeySize + Constants::ValueMetadataSizeBytes) * consolidatedStoreComponentSPtr_->Count();
         }

         void AddToMemorySize(__in LONG64 size)
         {
            // Preethas : These elements could be present in one of the delta states but the size of consolidated state is incremented
            // It is okay to be approximate here.
            consolidatedStoreComponentSPtr_->AddToMemorySize(size);
         }

         void AppendDeltaDifferentialState(__in DifferentialStoreComponent<TKey, TValue>& deltaState)
         {
            // Avoid incrementing index before insertion.
            ULONG32 localIndex = index_;
            auto deltaStateSPtr = &deltaState;
            bool add = deltaDifferentialStateListSPtr_->TryAdd(++localIndex, deltaStateSPtr);
            ASSERT_IFNOT(add, "Failed to add index {0} to differential list", localIndex);
            index_ = localIndex;
         }

         KSharedPtr<IEnumerator<TKey>> GetSortedKeyEnumerable(__in bool useFirstKey, __in TKey & firstKey, __in bool useLastKey, __in TKey & lastKey)
         {
             KSharedPtr<KSharedArray<KSharedPtr<IEnumerator<TKey>>>> enumeratorsSPtr = _new(AGGREGATEDSTATE_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<IEnumerator<TKey>>>();
             
             auto deltaDifferentialStateListEnumerator = deltaDifferentialStateListSPtr_->GetEnumerator();

             while (deltaDifferentialStateListEnumerator->MoveNext())
             {
                 auto currentItem = deltaDifferentialStateListEnumerator->Current();
                 auto componentSPtr = currentItem.Value;
                 auto filterableEnumerator = componentSPtr->GetEnumerableNewKeys();

                 if (useFirstKey)
                 {
                     filterableEnumerator->MoveTo(firstKey);
                 }

                 KSharedPtr<IEnumerator<TKey>> enumerator = static_cast<IEnumerator<TKey> *>(filterableEnumerator.RawPtr());
                 enumeratorsSPtr->Append(enumerator);
             }

             auto consolidatedEnumerator = consolidatedStoreComponentSPtr_->EnumerateKeys();
             if (useFirstKey)
             {
                 consolidatedEnumerator->MoveTo(firstKey);
             }
             
             KSharedPtr<IEnumerator<TKey>> enumerator = static_cast<IEnumerator<TKey> *>(consolidatedEnumerator.RawPtr());

             enumeratorsSPtr->Append(enumerator);

             KSharedPtr<IEnumerator<TKey>> resultSPtr;
             NTSTATUS status = SortedSequenceMergeEnumerator<TKey>::Create(*enumeratorsSPtr, *keyComparerSPtr_, useFirstKey, firstKey, useLastKey, lastKey, this->GetThisAllocator(), resultSPtr);
             Diagnostics::Validate(status);
             ASSERT_IFNOT(resultSPtr != nullptr, "result enumerator should not be null");
             return resultSPtr;
         }

         KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> GetKeysAndValuesEnumerator()
         {
             KSharedPtr<KSharedArray<KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>>>> enumeratorsSPtr = 
                 _new(AGGREGATEDSTATE_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>>>();

             auto deltaDifferentialStateListEnumerator = deltaDifferentialStateListSPtr_->GetEnumerator();

             while (deltaDifferentialStateListEnumerator->MoveNext())
             {
                 auto currentItem = deltaDifferentialStateListEnumerator->Current();
                 auto componentSPtr = currentItem.Value;

                 auto differentialEnumerator = componentSPtr->GetKeyAndValues();

                 KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> enumerator = static_cast<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> *>(differentialEnumerator.RawPtr());
                 enumeratorsSPtr->Append(enumerator);
             }

             auto consolidatedStateEnumerator = consolidatedStoreComponentSPtr_->EnumerateEntries();
             KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> enumerator = static_cast<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> *>(consolidatedStateEnumerator.RawPtr());
             enumeratorsSPtr->Append(enumerator);

             KSharedPtr<KeyVersionedItemComparer<TKey, TValue>> comparerSPtr;
             KeyVersionedItemComparer<TKey, TValue>::Create(*keyComparerSPtr_, this->GetThisAllocator(), comparerSPtr);

             KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> defaultKV;
             KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> resultSPtr;
             NTSTATUS status = SortedSequenceMergeEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>::Create(
                 *enumeratorsSPtr, 
                 *comparerSPtr, 
                 false, 
                 defaultKV, 
                 false, 
                 defaultKV, 
                 this->GetThisAllocator(), 
                 resultSPtr);
             Diagnostics::Validate(status);
             ASSERT_IFNOT(resultSPtr != nullptr, "result enumerator should not be null");
             return resultSPtr;
         }

         //KSharedPtr<IEnumerator<KSharedPtr<VersionedItem<TValue>>>> GetValuesForSweep()
         //{
         //   ULONG32 i = 0;
         //   for()

         //}

      private:
         AggregatedStoreComponent(__in IComparer<TKey>& keyComparer, __in StoreTraceComponent& traceComponent);
         AggregatedStoreComponent(
             __in ConsolidatedStoreComponent<TKey, TValue>& consolidatedStoreComponent,
             __in IComparer<TKey> & keyComparer,
             __in StoreTraceComponent & traceComponent);

         KSharedPtr<ConsolidatedStoreComponent<TKey, TValue>> consolidatedStoreComponentSPtr_;
         KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>> deltaDifferentialStateListSPtr_;
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;

         StoreTraceComponent::SPtr traceComponent_;

         // Index starts with 1, zero is not a valid index.
         ULONG32 index_ = 0;
      };

      template <typename TKey, typename TValue>
      AggregatedStoreComponent<TKey, TValue>::AggregatedStoreComponent(
         __in IComparer<TKey>& keyComparer,
          __in StoreTraceComponent & traceComponent):
          consolidatedStoreComponentSPtr_(nullptr),
          keyComparerSPtr_(&keyComparer),
          traceComponent_(&traceComponent)
      {
         NTSTATUS status = ConsolidatedStoreComponent<TKey, TValue>::Create(keyComparer, this->GetThisAllocator(), consolidatedStoreComponentSPtr_);
         if (!NT_SUCCESS(status))
         {
            this->SetConstructorStatus(status);
            return;
         }

         status = ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>::Create(this->GetThisAllocator(), deltaDifferentialStateListSPtr_);
         this->SetConstructorStatus(status);
      }

      template <typename TKey, typename TValue>
      AggregatedStoreComponent<TKey, TValue>::AggregatedStoreComponent(
          __in ConsolidatedStoreComponent<TKey, TValue>& consolidatedStoreComponent,
          __in IComparer<TKey> & keyComparer,
          __in StoreTraceComponent & traceComponent):
          consolidatedStoreComponentSPtr_(&consolidatedStoreComponent),
          keyComparerSPtr_(&keyComparer),
          traceComponent_(&traceComponent)
      {
         NTSTATUS status = ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>::Create(this->GetThisAllocator(), deltaDifferentialStateListSPtr_);
         this->SetConstructorStatus(status);
      }

      template <typename TKey, typename TValue>
      AggregatedStoreComponent<TKey, TValue>::~AggregatedStoreComponent()
      {
      }
   }
}
