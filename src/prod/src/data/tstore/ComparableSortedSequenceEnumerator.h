// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define COMPARABLE_SORTED_ENUM_TAG 'csSE'

namespace Data
{
    namespace TStore
    {
        template<typename T>
        class ComparableSortedSequenceEnumerator : public IEnumerator<T>
        {
            K_FORCE_SHARED(ComparableSortedSequenceEnumerator)
        public:
            static NTSTATUS Create(
                __in IEnumerator<T> & keyEnumerable,
                __in IComparer<T> & keyComparer,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                result = _new(COMPARABLE_SORTED_ENUM_TAG, allocator) ComparableSortedSequenceEnumerator(keyEnumerable, keyComparer);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            T Current() override
            {
                return keyEnumeratorSPtr_->Current();
            }

            bool MoveNext() override
            {
                return keyEnumeratorSPtr_->MoveNext();
            }

            int CompareTo(__in ComparableSortedSequenceEnumerator<T> & other)
            {
                return keyComparerSPtr_->Compare(Current(), other.Current());
            }

        private:
            ComparableSortedSequenceEnumerator(
                __in IEnumerator<T> & keyEnumerable,
                __in IComparer<T> & keyComparer) :
                keyComparerSPtr_(&keyComparer),
                keyEnumeratorSPtr_(&keyEnumerable)
            {
            }

            KSharedPtr<IComparer<T>> keyComparerSPtr_;
            KSharedPtr<IEnumerator<T>> keyEnumeratorSPtr_;
        };

        template<typename T>
        ComparableSortedSequenceEnumerator<T>::~ComparableSortedSequenceEnumerator()
        {
        }
    }
}
