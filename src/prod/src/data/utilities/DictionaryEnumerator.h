// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DICTIONARYENUM_TAG 'meID'

namespace Data
{
   namespace Utilities
   {
      template<typename TKey, typename TValue>
      class DictionaryEnumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
      {
         K_FORCE_SHARED(DictionaryEnumerator)

      public:
         static NTSTATUS Create(
            __in Dictionary<TKey, TValue> const & dictionary,
            __in KAllocator & allocator,
            __out KSharedPtr<DictionaryEnumerator> & result)
         {
            NTSTATUS status;
            SPtr output = _new(DICTIONARYENUM_TAG, allocator) DictionaryEnumerator(dictionary);

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

         // Returning TKey for now so as to not require values to have default constructors.
         KeyValuePair<TKey, TValue> Current()
         {
            KeyValuePair<TKey, TValue> ret;
            ret.Key = currentKey_;
            ret.Value = currentValue_;
            return ret;
         }

         bool MoveNext()
         {
            NTSTATUS status = componentSPtr_->Next(currentKey_, currentValue_);
            return NT_SUCCESS(status);
         }

      private:
         KSharedPtr<Dictionary<TKey, TValue>> componentSPtr_;

         // Note: TKey, TValue should have default constructor.
         TKey currentKey_;
         TValue currentValue_;
         NOFAIL DictionaryEnumerator(__in Dictionary<TKey, TValue> const & dictionary);
      };

      template <typename TKey, typename TValue>
      DictionaryEnumerator<TKey, TValue>::DictionaryEnumerator(__in Dictionary<TKey, TValue> const & dictionary)
         : componentSPtr_(const_cast<Dictionary<TKey, TValue>*>(&dictionary))
      {
         componentSPtr_->Reset();
      }

      template <typename TKey, typename TValue>
      DictionaryEnumerator<TKey, TValue>::~DictionaryEnumerator()
      {
      }
   }
}
