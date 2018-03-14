// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DIFFERENTIALSTATEENUM_TAG 'mnED'

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>
      class ConsolidationManager;
      
      //
      // Enuemrates differential data (refer to differential data class)
      //
      template<typename TKey, typename TValue>
      class DifferentialDataEnumerator : public IEnumerator<KSharedPtr<DifferentialData<TKey, TValue>>>
      {
         K_FORCE_SHARED(DifferentialDataEnumerator)

      public:
         static NTSTATUS Create(
            __in SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>& priorityQueue,
            __in IComparer<TKey>& comparer,
            __in ConsolidationManager<TKey, TValue>& consolidationManager,
            __in MetadataTable& metadataTable,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTATEENUM_TAG, allocator) DifferentialDataEnumerator(priorityQueue, comparer, consolidationManager, metadataTable);

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

      public:
         KSharedPtr<DifferentialData<TKey, TValue>> Current()
         {
            return currentDifferentialDataSPtr_;
         }

         bool MoveNext()
         {
            if (priorityQueueSPtr_->IsEmpty())
            {
               return false;
            }

            KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> enumerator = nullptr;
            bool result  = priorityQueueSPtr_->Pop(enumerator);
            ASSERT_IFNOT(result, "Priority queue cannot be empty");
            ASSERT_IFNOT(enumerator != nullptr, "Enumerator cannot be null");

            auto key = enumerator->Current().Key;
            auto value = enumerator->Current().Value;
            auto differentialState = enumerator->ComponentSPtr;

            auto nextValue = value;

            // Ignore all duplicate keys with smaller LSNs.
            while (!priorityQueueSPtr_->IsEmpty())
            {
               KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> peekedEnumerator = nullptr;
               result = priorityQueueSPtr_->Peek(peekedEnumerator);
               ASSERT_IFNOT(result, "Priority queue cannot be empty");
               ASSERT_IFNOT(peekedEnumerator != nullptr, "Enumerator cannot be null");

               auto currentKey = peekedEnumerator->Current().Key;

               if (keyComparerSPtr_->Compare(currentKey, key) == 0)
               {
                  KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> poppedEnumerator = nullptr;
                  result = priorityQueueSPtr_->Pop(poppedEnumerator);

                  ASSERT_IFNOT(result, "Priority queue pop unsuccessful");
                  ASSERT_IFNOT(enumerator != nullptr, "PoppedEnumerator cannot be null");

                  // Verify the item being skipped has an earlier LSN than the one being returned.
                  auto skipLSN = poppedEnumerator->Current().Value->GetVersionSequenceNumber();
                  ASSERT_IFNOT(skipLSN < value->GetVersionSequenceNumber(), "Enumeration should not return a key with an earlier lsn skip lsn {0}, value lsn {1}", skipLSN, value->GetVersionSequenceNumber());

                  // Check if it needs to be moved into snapshot contianer.
                  auto valueToBeDeleted = poppedEnumerator->Current().Value;

                  consolidationManagerSPtr_->ProcessToBeRemovedVersions(key, *nextValue, *valueToBeDeleted, *metadataTableSPtr_);

                  // This value becomes the next value
                  nextValue = valueToBeDeleted;

                  if (poppedEnumerator->MoveNext())
                  {
                     priorityQueueSPtr_->Push(poppedEnumerator);
                  }
                  else
                  {
                     //poppedEnumerator->Dispose();
                  }
               }
               else
               {
                  break;
               }
            }

            // Move the enumerator to the next key and add it back to the heap
            if (enumerator->MoveNext())
            {
               priorityQueueSPtr_->Push(enumerator);
            }
            else
            {
               //enumerator->Dispose();
            }

            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>  kvp(key, value);

            if (currentDifferentialDataSPtr_ == nullptr)
            {
               NTSTATUS status = DifferentialData<TKey, TValue>::Create(*differentialState, kvp, this->GetThisAllocator(), currentDifferentialDataSPtr_);
               Diagnostics::Validate(status);
            }
            else
            {
               currentDifferentialDataSPtr_->DifferentialStoreComponentSPtr = *differentialState;
               currentDifferentialDataSPtr_->KeyAndVersionedItemPair = kvp;
            }

            return true;
         }

         void Close()
         {
            consolidationManagerSPtr_ = nullptr;
         }

      private:

         DifferentialDataEnumerator(
            __in SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>& priorityQueue,
            __in IComparer<TKey>& comparer,
            __in ConsolidationManager<TKey, TValue>& consolidationManager,
            __in MetadataTable& metadataTable);

         KSharedPtr<SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>> priorityQueueSPtr_;
         KSharedPtr<DifferentialData<TKey, TValue>> currentDifferentialDataSPtr_ = nullptr;
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
         MetadataTable::SPtr metadataTableSPtr_;
         KSharedPtr<ConsolidationManager<TKey, TValue>> consolidationManagerSPtr_;
      };

      template <typename TKey, typename TValue>
      DifferentialDataEnumerator<TKey, TValue>::DifferentialDataEnumerator(
         __in SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>& priorityQueue,
         __in IComparer<TKey>& comparer,
         __in ConsolidationManager<TKey, TValue>& consolidationManager,
         __in MetadataTable& metadataTable)
         : priorityQueueSPtr_(&priorityQueue),
         keyComparerSPtr_(&comparer),
         consolidationManagerSPtr_(&consolidationManager),
         metadataTableSPtr_(&metadataTable)
      {
      }

      template <typename TKey, typename TValue>
      DifferentialDataEnumerator<TKey, TValue>::~DifferentialDataEnumerator()
      {
      }
   }
}
