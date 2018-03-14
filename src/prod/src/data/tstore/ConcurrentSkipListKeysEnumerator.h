// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONCURRENTSKIPLIST_KEYSENUMERATOR_TAG 'csKE'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class ConcurrentSkipListKeysEnumerator: public IEnumerator<TKey>
        {
            K_FORCE_SHARED(ConcurrentSkipListKeysEnumerator)
        public:
            static NTSTATUS Create(
                __in ConcurrentSkipList<TKey, TValue> & skipList,
                __in KAllocator & allocator,
                __out KSharedPtr<IEnumerator<TKey>> & result)
            {
                result = _new(CONCURRENTSKIPLIST_KEYSENUMERATOR_TAG, allocator) ConcurrentSkipListKeysEnumerator(skipList);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }
            TKey Current() override
            {
                KeyValuePair<TKey, TValue> currentItem = skipListEnumeratorSPtr_->Current();
                return currentItem.Key;
            }

            bool MoveNext() override
            {
                return skipListEnumeratorSPtr_->MoveNext();
            }

        private:
            ConcurrentSkipListKeysEnumerator(__in ConcurrentSkipList<TKey, TValue> & skipList) :
                skipListEnumeratorSPtr_(nullptr)
            {
                KSharedPtr<ConcurrentSkipList<TKey, TValue>> skipListSPtr = &skipList;
                NTSTATUS status = ConcurrentSkipList<TKey, TValue>::Enumerator::Create(skipListSPtr, skipListEnumeratorSPtr_);
                Diagnostics::Validate(status);
            }
            
            KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> skipListEnumeratorSPtr_;
        };

        template<typename TKey, typename TValue>
        ConcurrentSkipListKeysEnumerator<TKey, TValue>::~ConcurrentSkipListKeysEnumerator()
        {
        }
    }
}
