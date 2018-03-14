// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define WRITESETSTORECOMPONENT_TAG 'csSW'

namespace Data
{
   namespace TStore
   {
      template <typename TKey, typename TValue>
      class WriteSetStoreComponent :
         public KObject<WriteSetStoreComponent<TKey, TValue>>,
         public KShared<WriteSetStoreComponent<TKey, TValue>>
      {
         K_FORCE_SHARED(WriteSetStoreComponent);

      public:
         typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;
         //typedef KDelegate<bool(const TKey & Key)> FilterFunctionType;

         struct WriteSetItemContext
         {
            WriteSetItemContext()
            {

            }

            WriteSetItemContext(__in  VersionedItem<TValue>& value)
               : CreateSequenceNumber(value.GetVersionSequenceNumber()),
               FirstVersionKind(value.GetRecordKind()),
               LatestValue(&value)
            {
            }

            WriteSetItemContext(
               __in WriteSetItemContext previous,
               __in VersionedItem<TValue>& latestvalue)
               : CreateSequenceNumber(previous.GetVersionSequenceNumber()),
               FirstVersionKind(previous.GetRecordKind()),
               LatestValue(&latestvalue)
            {
            }

            // todo: struct member convention
            KSharedPtr<VersionedItem<TValue>> LatestValue;
            LONG64                            CreateSequenceNumber;
            RecordKind                        FirstVersionKind;
         };

         static NTSTATUS Create(
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer,
            __in KAllocator& allocator,
            __out SPtr& result)
         {
            NTSTATUS status;
            SPtr output = _new(WRITESETSTORECOMPONENT_TAG, allocator) WriteSetStoreComponent(32, func, keyComparer);

            if (!output)
            {
               return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = output->Status();
            if (!NT_SUCCESS(status))
            {
               return status;
            }

            result = Ktl::Move(output);
            return STATUS_SUCCESS;
         }

         bool ContainsKey(__in TKey& key)
         {
            return writeSetSPtr_->ContainsKey(key);
         }

         typename VersionedItem<TValue>::SPtr Read(__in TKey& key)
         {
            WriteSetItemContext outContext;
            bool itemFound = writeSetSPtr_->TryGetValue(key, outContext);
            if (itemFound)
            {
               return outContext.LatestValue;
            }

            return nullptr;
         }

         void Add(__in bool isIdempotent, __in TKey& key, __in VersionedItem<TValue>& item)
         {
            KInvariant(item.GetVersionSequenceNumber() >= 0);

            WriteSetItemContext context;
            bool foundKey = writeSetSPtr_->TryGetValue(key, context);
            if (foundKey)
            {
               if (isIdempotent)
               {
                  // Apply only if the version sequence number is higher.
                  if (item.GetVersionSequenceNumber() > context.LatestValue->GetVersionSequenceNumber())
                  {
                     Update(key, context, item);
                  }
               }
               else
               {
                  // Update must succeed
                  Update(key, context, item);
               }
            }
            else
            {
               // First time this key is modified - add it.
               WriteSetItemContext lContext(item);
               writeSetSPtr_->Add(key, lContext);
            }
         }

         void Update(__in TKey& key, __in WriteSetItemContext &currentContext, __in VersionedItem<TValue>& newItem)
         {
            auto lastVersionedItem = currentContext.LatestValue;

            // todo: Add assert after versioned item access thro derived class is resolved.

            // Update last versioned item.
            WriteSetItemContext context(newItem);
            writeSetSPtr_->AddOrUpdate(key, context);
         }

         KSharedPtr<DictionaryEnumerator<TKey, WriteSetItemContext>> GetEnumerator()
         {
            KSharedPtr<DictionaryEnumerator<TKey, WriteSetItemContext>> enumeratorSPtr = nullptr;
            NTSTATUS status = DictionaryEnumerator<TKey, WriteSetItemContext>::Create(*writeSetSPtr_, this->GetThisAllocator(), enumeratorSPtr);
            Diagnostics::Validate(status);
            return enumeratorSPtr;
         }

         KSharedPtr<IEnumerator<TKey>> GetSortedKeyEnumerable(__in bool useFirstKey, __in TKey & firstKey, __in bool useLastKey, __in TKey & lastKey) //, __in FilterFunctionType keyFilter = SelectAllFilterFunction)
         {
             auto snapWriteSetSPtr = writeSetSPtr_;

             auto numberOfItems = snapWriteSetSPtr->Count;
             if (numberOfItems == 0)
             {
                 return nullptr;
             }

             KSharedPtr<KSharedArray<TKey>> keyListSPtr = _new(WRITESETSTORECOMPONENT_TAG, this->GetThisAllocator()) KSharedArray<TKey>();
             keyListSPtr->Reserve(numberOfItems);

             KSharedPtr<DictionaryEnumerator<TKey, WriteSetItemContext>> enumeratorSPtr = nullptr;
             NTSTATUS status = DictionaryEnumerator<TKey, WriteSetItemContext>::Create(*snapWriteSetSPtr, this->GetThisAllocator(), enumeratorSPtr);
             Diagnostics::Validate(status);

             while (enumeratorSPtr->MoveNext())
             {
                 KeyValuePair<TKey, WriteSetItemContext> row = enumeratorSPtr->Current();

                 // If current item is smaller than "first key", ignore the key
                 if (useFirstKey && keyComparerSPtr_->Compare(row.Key, firstKey) < 0)
                 {
                     continue;
                 }
                
                 // If current item is larger than the "last key", ignore the key
                 if (useLastKey && keyComparerSPtr_->Compare(row.Key, lastKey) > 0)
                 {
                     continue;
                 }

                 //// If current item does not fit the filter, go to next
                 //if (keyFilter(row.Key) == false)
                 //{
                 //    continue;
                 //}

                 keyListSPtr->Append(row.Key);
             }

             Sorter<TKey>::QuickSort(true, *keyComparerSPtr_, keyListSPtr);

             KSharedPtr<IEnumerator<TKey>> outputEnumerationSPtr;
             KSharedArrayEnumerator<TKey>::Create(*keyListSPtr, this->GetThisAllocator(), outputEnumerationSPtr);
             return outputEnumerationSPtr;
         }

      private:
          //static bool SelectAllFilterFunction(__in const TKey & key)
          //{
          //    UNREFERENCED_PARAMETER(key);
          //    return true;
          //}

         FAILABLE WriteSetStoreComponent(
            __in ULONG32 size,
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer);

         KSharedPtr<Dictionary<TKey, WriteSetItemContext>> writeSetSPtr_;
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
      };

      template <typename TKey, typename TValue>
      WriteSetStoreComponent<TKey, TValue>::WriteSetStoreComponent(
         __in ULONG32 size,
         __in HashFunctionType func,
         __in IComparer<TKey> & keyComparer)
         : writeSetSPtr_(nullptr),
          keyComparerSPtr_(&keyComparer)
      {

         NTSTATUS status = Dictionary<TKey, WriteSetItemContext>::Create(size, func, keyComparer, this->GetThisAllocator(), writeSetSPtr_);
         if (!NT_SUCCESS(status))
         {
            this->SetConstructorStatus(status);
            return;
         }
      }

      template <typename TKey, typename TValue>
      WriteSetStoreComponent<TKey, TValue>::~WriteSetStoreComponent()
      {
      }
   }
}
