// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        template<typename T>
        class KHashSet<T>::Enumerator : public IEnumerator<T>
        {
            K_FORCE_SHARED(Enumerator)

        public:
            static NTSTATUS Create(
                __in KHashSet<T> const & hashSet,
                __out KSharedPtr<IEnumerator<T>> & result)
            {
                result = _new(HASHSET_TAG, hashSet.GetThisAllocator()) Enumerator(hashSet);
                if (result == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(result->Status()))
                {
                    return (KSharedPtr<IEnumerator<T>>(Ktl::Move(result)))->Status();
                }

                return STATUS_SUCCESS;
            }

        public: // IEnumerator<T>
            T Current() override
            {
                return current_;
            }

            bool MoveNext() override
            {
                T key;
                bool value;
                NTSTATUS status = sourceSPtr_->hashTable_.Next(key, value);
                if (NT_SUCCESS(status))
                {
                    current_ = key;
                    return true;
                }

                return false;
            }

        private: // Constructors
            Enumerator(__in KHashSet<T> const & hashSet);

        private:
            const KSharedPtr<const KHashSet<T>> sourceSPtr_;
            T current_;
        };

        template<typename T>
        KHashSet<T>::Enumerator::Enumerator(
            __in KHashSet<T> const & hashSet)
            : sourceSPtr_(&hashSet)
        {
            sourceSPtr_->hashTable_.Reset();
        }

        template<typename T>
        KHashSet<T>::Enumerator::~Enumerator()
        {
        }
    }
}
