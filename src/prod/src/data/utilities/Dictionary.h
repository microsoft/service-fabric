// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DICTIONARY_TAG 'stDC'

using namespace Data::Utilities;

namespace Data
{
   namespace Utilities
   {
      template<typename TKey, typename TValue>
      class Dictionary sealed : public IDictionary<TKey, TValue>
      {
         K_FORCE_SHARED(Dictionary)

      public:
         typedef KSharedPtr<Dictionary<TKey, TValue>> SPtr;

         typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;

         static NTSTATUS Create(
            __in ULONG32 size,
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DICTIONARY_TAG, allocator) Dictionary(size, func, keyComparer, allocator);

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

         bool ContainsKey(__in TKey const & key) const override
         {
            BOOLEAN result = hashTable_.ContainsKey(const_cast<TKey&>(key));

            return result == TRUE;
         }

         __declspec(property(get = get_Count)) ULONG Count;
         ULONG get_Count() const override
         {
            ULONG result = hashTable_.Count();

            return result;
         }

         void Add(__in TKey const & key, __in TValue const & value) override
         {
            NTSTATUS status = hashTable_.Put(key, value, false);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to add key");
            THROW_ON_FAILURE(status);

            if (hashTable_.Size() * 3 < hashTable_.Count())
            {
               size_ = (size_ * 2) + 1;
               hashTable_.Resize(size_);
            }
         }

         void AddOrUpdate(__in TKey& key, __in TValue& value)
         {
            NTSTATUS status = hashTable_.Put(key, value, true);
            THROW_ON_FAILURE(status);

            if (hashTable_.Size() * 3 < hashTable_.Count())
            {
               size_ = (size_ * 2) + 1;
               hashTable_.Resize(size_);
            }
         }

         bool TryGetValue(__in TKey const & key, __out TValue& value) const override
         {
            NTSTATUS status = hashTable_.Get(key, value);

            if (status == STATUS_SUCCESS)
            {
               return true;
            }
            else
            {
               return false;
            }
         }

         bool Remove(__in TKey const & key) override
         {
            NTSTATUS status = hashTable_.Remove(key);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to remove key");
            return NT_SUCCESS(status);
         }

         void Remove(__in TKey const & key, __out TValue & value)
         {
             NTSTATUS status = hashTable_.Remove(key, &value);
             ASSERT_IFNOT(NT_SUCCESS(status), "Failed to remove key");
             KInvariant(NT_SUCCESS(status));
         }

         void Clear()
         {
            hashTable_.Clear();
         }

         void Reset()
         {
            hashTable_.Reset();
         }

         NTSTATUS Next(__out TKey& key, __out TValue& value)
         {
            return hashTable_.Next(key, value);
         }

         KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const override
         {
             NTSTATUS status;
             KSharedPtr<DictionaryEnumerator<TKey, TValue>> output;
             status = DictionaryEnumerator<TKey, TValue>::Create(*this, this->GetThisAllocator(), output);
             if (!NT_SUCCESS(status))
             {
                 return nullptr;
             }

             return output.RawPtr();
         }

      private:
          BOOLEAN KeysEqual(__in const TKey& KeyOne, __in const TKey& KeyTwo)
          {
              return keyComparerSPtr_->Compare(KeyOne, KeyTwo) == 0;
          }

         FAILABLE Dictionary(
            __in ULONG size,
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer,
            __in KAllocator & allocator);

         ULONG size_;
         mutable KAutoHashTable<TKey, TValue> hashTable_;
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
      };

      template<typename TKey, typename TValue>
      Dictionary<TKey, TValue>::Dictionary(
         __in ULONG size,
         __in HashFunctionType func,
         __in IComparer<TKey> & keyComparer,
         __in KAllocator & allocator)
         : size_(size),
          hashTable_(allocator),
          keyComparerSPtr_(&keyComparer)
      {
          typename KHashTable<TKey, TValue>::EqualityComparisonFunctionType equalityFunc;
          equalityFunc.Bind(this, &Dictionary::KeysEqual);

          NTSTATUS status = hashTable_.Initialize(size, func, equalityFunc);
          this->SetConstructorStatus(status);
      }

      template<typename TKey, typename TValue>
      Dictionary<TKey, TValue>::~Dictionary()
      {
      }
   }
}

