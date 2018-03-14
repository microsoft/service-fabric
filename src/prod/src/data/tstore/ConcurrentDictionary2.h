// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DICTIONARY_TAG 'stDC'

namespace Data
{
   namespace TStore
   {
      // TODO: Temporary name, Reconcile with ConcurrentDictionary post Ignite
      template<typename TKey, typename TValue>
      class ConcurrentDictionary2 sealed : public IDictionary<TKey, TValue>
      {
         K_FORCE_SHARED(ConcurrentDictionary2)

      public:
         typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;

         static NTSTATUS Create(
            __in ULONG32 size,
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DICTIONARY_TAG, allocator) ConcurrentDictionary2(size, func, keyComparer, allocator);

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
             lock_.AcquireShared();
             KFinally([&] { lock_.ReleaseShared(); });

             return dictionarySPtr_->ContainsKey(key);
         }

		 __declspec(property(get = get_Count)) ULONG Count;
         ULONG get_Count() const override
         {
             lock_.AcquireShared();
             KFinally([&] { lock_.ReleaseShared(); });

             return dictionarySPtr_->Count;
         }

         void Add(__in TKey const & key, __in TValue const & value) override
         {
             lock_.AcquireExclusive();
             KFinally([&] { lock_.ReleaseExclusive(); });

             dictionarySPtr_->Add(key, value);
         }

         bool TryAdd(__in TKey const & key, __in TValue const & value)
         {
             lock_.AcquireExclusive();
             KFinally([&] { lock_.ReleaseExclusive(); });
            
             if (dictionarySPtr_->ContainsKey(key))
             {
                 return false;
             }

             dictionarySPtr_->Add(key, value);
             return true;
         }

         void AddOrUpdate(__in TKey& key, __in TValue& value)
         {
             lock_.AcquireExclusive();
             KFinally([&] { lock_.ReleaseExclusive(); });

             dictionarySPtr_->AddOrUpdate(key, value);
         }

         bool TryGetValue(__in TKey const & key, __out TValue& value) const override
         {
             lock_.AcquireShared();
             KFinally([&] { lock_.ReleaseShared(); });

             return dictionarySPtr_->TryGetValue(key, value);
         }

         bool Remove(__in TKey const & key) override
         {
             lock_.AcquireExclusive();
             KFinally([&] { lock_.ReleaseExclusive(); });

             if (!dictionarySPtr_->ContainsKey(key))
             {
                 return false;
             }

             return dictionarySPtr_->Remove(key);
         }

         KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const override
         {
             throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
         }

         void Clear() override
         {
             throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
         }

      private:
         FAILABLE ConcurrentDictionary2(
            __in ULONG size,
            __in HashFunctionType func,
            __in IComparer<TKey> & keyComparer,
            __in KAllocator & allocator);
        
         KSharedPtr<Dictionary<TKey, TValue>> dictionarySPtr_;
         mutable KReaderWriterSpinLock lock_;
      };

      template<typename TKey, typename TValue>
      ConcurrentDictionary2<TKey, TValue>::ConcurrentDictionary2(
         __in ULONG size,
         __in HashFunctionType func,
         __in IComparer<TKey> & keyComparer,
         __in KAllocator & allocator)
         : dictionarySPtr_(nullptr)
      {
          NTSTATUS status = Dictionary<TKey, TValue>::Create(size, func, keyComparer, allocator, dictionarySPtr_);
          Diagnostics::Validate(status);
      }

      template<typename TKey, typename TValue>
      ConcurrentDictionary2<TKey, TValue>::~ConcurrentDictionary2()
      {
      }
   }
}

