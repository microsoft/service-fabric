// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KSHAREDARRAY_ENUMERATOR_TAG 'ksAT'

namespace Data
{
    namespace TStore
    {
        template<typename T>
        class KSharedArrayEnumerator : public IEnumerator<T>
        {
            K_FORCE_SHARED(KSharedArrayEnumerator)
        public:
            static NTSTATUS Create(
                __in KSharedArray<T> & items,
                __in KAllocator & allocator,
                __out KSharedPtr<IEnumerator<T>> & result)
            {
                result = _new(KSHAREDARRAY_ENUMERATOR_TAG, allocator) KSharedArrayEnumerator(items);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }
            T Current() override
            {
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                ASSERT_IFNOT(static_cast<ULONG>(index_) < arraySPtr_->Count(), "Got out of bound index: {0}", index_);
                return (*arraySPtr_)[index_];
            }

            bool MoveNext() override
            {
                index_++;
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                return static_cast<ULONG>(index_) < arraySPtr_->Count();
            }
        private:
            KSharedArrayEnumerator(__in KSharedArray<T> & items) :
                arraySPtr_(&items),
                index_(-1)
            {
            }

            LONG32 index_;
            KSharedPtr<KSharedArray<T>> arraySPtr_;
        };

        template<typename T>
        KSharedArrayEnumerator<T>::~KSharedArrayEnumerator()
        {
        }
    }
}
