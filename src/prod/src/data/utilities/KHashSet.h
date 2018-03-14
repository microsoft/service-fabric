// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define HASHSET_TAG 'teSH'

namespace Data
{
    namespace Utilities
    {
        // This is a mock hash set.
        template<typename T>
        class KHashSet sealed : 
            public KObject<KHashSet<T>>, 
            public KShared<KHashSet<T>>
        {
            K_FORCE_SHARED(KHashSet);

        public:
            typedef KDelegate<ULONG(const T & Value)> HashFunctionType;
            typedef KDelegate<BOOLEAN(__in const T& KeyOne, __in const T& KeyTwo)> EqualityComparisonFunctionType;

            static NTSTATUS Create(
                __in ULONG size,
                __in HashFunctionType func,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                result = _new(HASHSET_TAG, allocator) KHashSet(size, func);
                if (result == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(result->Status()))
                {
                    return (KHashSet::SPtr(Ktl::Move(result)))->Status();
                }

                return STATUS_SUCCESS;
            }

            static NTSTATUS Create(
                __in ULONG size,
                __in HashFunctionType func,
                __in EqualityComparisonFunctionType equalityComparisonFunction,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                result = _new(HASHSET_TAG, allocator) KHashSet(size, func, equalityComparisonFunction);
                if (result == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(result->Status()))
                {
                    return (KHashSet::SPtr(Ktl::Move(result)))->Status();
                }

                return STATUS_SUCCESS;
            }

            bool ContainsKey(__in T const & value) const
            {
                // TODO: #9000025: KHashTable needs to be const correct.
                T& tmpValue = const_cast<T&>(value);

                BOOLEAN result = hashTable_.ContainsKey(tmpValue);

                return result == TRUE;
            }

            bool TryAdd(__in T const & value)
            {
                bool fakeValue = true;
                NTSTATUS status = hashTable_.Put(value, fakeValue, false);

                if (NT_SUCCESS(status))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            bool TryRemove(__in T const & value)
            {
                bool outValue = false;
                NTSTATUS status = hashTable_.Remove(value, &outValue);

                if (status == STATUS_NOT_FOUND)
                {
                    return false;
                }

                ASSERT_IFNOT(outValue == true, "KHashSet: Failed to remove from table");
                return true;
            }

            ULONG Count() const
            {
                return hashTable_.Count();
            }

            KSharedPtr<IEnumerator<T>> GetEnumerator() const
            {
                KSharedPtr<IEnumerator<T>> enumeratorSPtr = nullptr;
                NTSTATUS status = Enumerator::Create(*this, enumeratorSPtr);
                THROW_ON_FAILURE(status);

                return enumeratorSPtr;
            }

            bool Equals(KHashSet<T> const & other) const
            {
                if (Count() != other.Count())
                {
                    return false;
                }

                if (Count() == 0)
                {
                    return true;
                }

                KSharedPtr<IEnumerator<T>> enumerator = nullptr;
                NTSTATUS status = Enumerator::Create(*this, enumerator);
                THROW_ON_FAILURE(status);

                while (enumerator->MoveNext())
                {
                    if (other.ContainsKey(enumerator->Current()) == false)
                    {
                        return false;
                    }
                }

                return true;
            }

        private:
            class Enumerator;

        private:
            // #9000025: KHashTable needs to be const correct.
            mutable KHashTable<T, bool> hashTable_;

            FAILABLE KHashSet(
                __in ULONG size,
                __in HashFunctionType func);

            FAILABLE KHashSet(
                __in ULONG size,
                __in HashFunctionType func,
                __in EqualityComparisonFunctionType equalityComparisonFunction);
        };

        template<typename T>
        KHashSet<T>::KHashSet(
            __in ULONG size,
            __in HashFunctionType func)
            : KObject<KHashSet<T>>()
            , KShared<KHashSet<T>>()
            , hashTable_(size, func, this->GetThisAllocator())
        {
            this->SetConstructorStatus(hashTable_.Status());
        }

        template<typename T>
        KHashSet<T>::KHashSet(
            __in ULONG size,
            __in HashFunctionType func,
            __in EqualityComparisonFunctionType equalityComparisonFunction)
            : KObject<KHashSet<T>>()
            , KShared<KHashSet<T>>()
            , hashTable_(size, func, equalityComparisonFunction, this->GetThisAllocator())
        {
            this->SetConstructorStatus(hashTable_.Status());
        }

        template<typename T>
        KHashSet<T>::~KHashSet()
        {
        }
    }
}
