// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONCURRENTDICTIONARY_TAG 'stDC'

namespace Data
{
    namespace Utilities
    {
        // This is a mock concurrent dictionary.
        // TODO: Actual implementation will need to support key_equal function / equalscomparer to be passed in.
        // TODO: Const correctness.
        template<typename TKey,
                 typename TValue,
                 typename Hash = KDelegate<ULONG(const TKey&)>,
                 typename KeyComparer = KSharedPtr<IComparer<TKey>>>
        class ConcurrentDictionary sealed : public IDictionary<TKey, TValue>
        {
            K_FORCE_SHARED(ConcurrentDictionary)

        private:
            class Enumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
            {
                K_FORCE_SHARED(Enumerator)

            public:
                static NTSTATUS Create(
                    __in KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> & enumerator,
                    __in KAllocator & allocator,
                    __out KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> & result)
                {
                    if (enumerator == nullptr)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }

                    result = _new(CONCURRENTDICTIONARY_TAG, allocator) Enumerator(enumerator);
                    if (!result)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    return STATUS_SUCCESS;
                }

            private:
                Enumerator(__in KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> & enumerator)
                    : enumerator_(enumerator)
                {
                }

            public:
                KeyValuePair<TKey, TValue> Current()
                {
                    return enumerator_->Current();
                }

                bool MoveNext()
                {
                    return enumerator_->MoveNext();
                }

            private:
                KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> enumerator_;
            };

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                KSharedPtr<IComparer<TKey>> keyComparer;
                status = DefaultKeyComparer::Create(allocator, keyComparer);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                return Create(DefaultConcurrencyLevel,
                              DefaultInitialCapacity,
                              DefaultHash,
                              keyComparer,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in KeyComparer const & keyComparer,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                return Create(DefaultConcurrencyLevel,
                              DefaultInitialCapacity,
                              DefaultHash,
                              keyComparer,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in int concurrencyLevel,
                __in int capacity,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                KSharedPtr<IComparer<TKey>> keyComparer;
                status = DefaultKeyComparer::Create(allocator, keyComparer);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                return Create(concurrencyLevel,
                              capacity,
                              DefaultHash,
                              keyComparer,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in int concurrencyLevel,
                __in int capacity,
                __in KeyComparer const & keyComparer,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                return Create(concurrencyLevel,
                              capacity,
                              DefaultHash,
                              keyComparer,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in int concurrencyLevel,
                __in int capacity,
                __in Hash const & hashFunc,
                __in KeyComparer const & keyComparerFunc,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(CONCURRENTDICTIONARY_TAG, allocator) ConcurrentDictionary(concurrencyLevel,
                                                                                             capacity,
                                                                                             hashFunc,
                                                                                             keyComparerFunc,
                                                                                             allocator);

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

            __declspec(property(get = get_Count)) ULONG Count;
            ULONG get_Count() const override
            {
                return skipList_->Count;
            }

            bool ContainsKey(__in TKey const & key) const override
            {
                return skipList_->ContainsKey(key);
            }

            void Add(
                __in TKey const & key,
                __in TValue const & value) override
            {
                bool succeeded = TryAdd(key, value);
                ASSERT_IFNOT(succeeded, "Failed to add key and value. Key already exists.");
            }

            void Clear() override
            {
                skipList_->Clear();
            }

            KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const override
            {
                NTSTATUS status;
                KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> skiplistEnumerator;
                status = ConcurrentSkipList<TKey, TValue>::Enumerator::Create(skipList_, skiplistEnumerator);
                if (!NT_SUCCESS(status))
                {
                    return nullptr;
                }

                KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> output;
                status = Enumerator::Create(skiplistEnumerator, this->GetThisAllocator(), output);
                if (!NT_SUCCESS(status))
                {
                    return nullptr;
                }

                return output;
            }

            bool Remove(__in TKey const & key) override
            {
                TValue value;
                return TryRemove(key, value);
            }

            bool TryGetValue(
                __in TKey const & key,
                __out TValue & value) const override
            {
                return skipList_->TryGetValue(key, value);
            }

            __declspec(property(get = get_IsEmpty)) bool IsEmpty;
            bool ConcurrentDictionary::get_IsEmpty() const
            {
                return skipList_->IsEmpty;
            }

            bool TryAdd(
                __in TKey const & key,
                __in TValue const & value)
            {
                return skipList_->TryAdd(key, value);
            }

            TValue AddOrUpdate(
                __in TKey const & key,
                __in TValue const & value,
                __in KDelegate<TValue(TKey const &, TValue const &)> updateValueFactory)
            {
                return skipList_->AddOrUpdate(key, value, updateValueFactory);
            }

            TValue GetOrAdd(
                __in TKey const & key,
                __in TValue const & value)
            {
                return skipList_->GetOrAdd(key, value);
            }

            TValue GetOrAdd(
                __in TKey const & key,
                __in KDelegate<TValue(TKey const &)> valueFactory)
            {
                auto factory = [&] (__in TKey const &)
                {
                    return valueFactory(key);
                };
                return skipList_->GetOrAdd(key, factory);
            }

            bool TryRemove(
                __in TKey const & key,
                __out TValue & value)
            {
                return skipList_->TryRemove(key, value);
            }

            bool TryUpdate(
                __in TKey const & key,
                __in TValue const & newValue,
                __in TValue const & comparisonValue)
            {
                auto valueEqualFunc = [&] (
                    __in TValue const & lhs,
                    __in TValue const & rhs)
                {
                    return lhs == rhs;
                };
                return skipList_->TryUpdate(key,
                                            newValue,
                                            comparisonValue,
                                            valueEqualFunc);
            }

        private:
            template <typename KeyType>
            static ULONG DefaultHash(__in KeyType const & key)
            {
                return K_DefaultHashFunction(key);
            }

            template <>
            static ULONG DefaultHash(__in int const & key)
            {
                return K_DefaultHashFunction(LONGLONG(key));
            }

            template <typename KeyType>
            static int DefaultComparer(
                __in KeyType const & lhs,
                __in KeyType const & rhs)
            {
                return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
            }

            template <>
            static int DefaultComparer(
                __in KUriView const & lhs,
                __in KUriView const & rhs)
            {
                const KStringView& v1 = lhs.Get(KUriView::eRaw);
                const KStringView& v2 = rhs.Get(KUriView::eRaw);
                return v1.Compare(v2);
            }

            template <>
            static int DefaultComparer(
                __in KUri::SPtr const & lhs,
                __in KUri::SPtr const & rhs)
            {
                return *lhs < *rhs ? -1 : *lhs > *rhs ? 1 : 0;
            }

            template <>
            static int DefaultComparer(
                __in KUri::CSPtr const & lhs,
                __in KUri::CSPtr const & rhs)
            {
                return *lhs < *rhs ? -1 : *lhs > *rhs ? 1 : 0;
            }

            template <>
            static int DefaultComparer(
                __in KString::SPtr const & lhs,
                __in KString::SPtr const & rhs)
            {
                return lhs->Compare(*rhs);
            }

            template <>
            static int DefaultComparer(
                __in KString::CSPtr const & lhs,
                __in KString::CSPtr const & rhs)
            {
                return lhs->Compare(*rhs);
            }

            template <>
            static int DefaultComparer(
                __in KStringView const & lhs,
                __in KStringView const & rhs)
            {
                return lhs.Compare(rhs);
            }

            class DefaultKeyComparer
                : public KObject<DefaultKeyComparer>
                , public KShared<DefaultKeyComparer>
                , public IComparer<TKey>
            {
                K_FORCE_SHARED(DefaultKeyComparer)
                K_SHARED_INTERFACE_IMP(IComparer)

            public:
                static NTSTATUS Create(
                    __in KAllocator & allocator,
                    __out KSharedPtr<IComparer<TKey>> & result)
                {
                    result = _new(CONCURRENTDICTIONARY_TAG, allocator) DefaultKeyComparer();
                    if (!result)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    return STATUS_SUCCESS;
                }

                int Compare(
                    __in TKey const & lhs,
                    __in TKey const & rhs) const override
                {
                    return DefaultComparer(lhs, rhs);
                }
            };

        private:
            FAILABLE ConcurrentDictionary(
                __in ULONG concurrencyLevel,
                __in ULONG capacity,
                __in Hash const & hashFunc,
                __in KeyComparer const & keyComparerFunc,
                __in KAllocator & allocator);

        public:
            static const ULONG DefaultConcurrencyLevel = 16;
            static const ULONG DefaultInitialCapacity = 16;

        private:
            mutable Hash hashFunc_;
            KeyComparer keyComparerFunc_;
            KSharedPtr<ConcurrentSkipList<TKey, TValue>> skipList_;
        };

        class Comparer
        {
        public:
            static BOOLEAN Compare(__in KUri::CSPtr const & itemOne, __in KUri::CSPtr const & itemTwo)
            {
                return itemOne->Compare(*itemTwo);
            }
        };

        template<typename TKey,
                 typename TValue,
                 typename Hash,
                 typename KeyComparer>
        ConcurrentDictionary<TKey, TValue, Hash, KeyComparer>::Enumerator::~Enumerator()
        {
        }

        template<typename TKey,
                 typename TValue,
                 typename Hash,
                 typename KeyComparer>
        ConcurrentDictionary<TKey, TValue, Hash, KeyComparer>::DefaultKeyComparer::DefaultKeyComparer()
        {
        }

        template<typename TKey,
                 typename TValue,
                 typename Hash,
                 typename KeyComparer>
        ConcurrentDictionary<TKey, TValue, Hash, KeyComparer>::DefaultKeyComparer::~DefaultKeyComparer()
        {
        }

        template<typename TKey,
                 typename TValue,
                 typename Hash,
                 typename KeyComparer>
        ConcurrentDictionary<TKey, TValue, Hash, KeyComparer>::ConcurrentDictionary(
            __in ULONG concurrencyLevel,
            __in ULONG capacity,
            __in Hash const & hashFunc,
            __in KeyComparer const & keyComparerFunc,
            __in KAllocator & allocator)
            : hashFunc_(hashFunc),
              keyComparerFunc_(keyComparerFunc)
        {
            UNREFERENCED_PARAMETER(concurrencyLevel);
            UNREFERENCED_PARAMETER(capacity);

            NTSTATUS status;
            status = ConcurrentSkipList<TKey, TValue>::Create(keyComparerFunc, allocator, skipList_);
            this->SetConstructorStatus(status);
        }

        template<typename TKey,
                 typename TValue,
                 typename Hash,
                 typename KeyComparer>
        ConcurrentDictionary<TKey, TValue, Hash, KeyComparer>::~ConcurrentDictionary()
        {
        }
    }
}
