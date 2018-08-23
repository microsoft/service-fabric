// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        interface IStore
        {
            K_SHARED_INTERFACE(IStore)
            
        public:

            virtual bool CreateOrFindTransaction(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __out KSharedPtr<IStoreTransaction<TKey, TValue>>& result) = 0;

            virtual ktl::Awaitable<void> AddAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in TValue value,
                __in Common::TimeSpan timeout, // If less than 0, then assumes infinite timeout
                __in ktl::CancellationToken const & cancellationToken) = 0;
            
            virtual ktl::Awaitable<bool> ConditionalUpdateAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in TValue value,
                __in Common::TimeSpan timeout, // If less than 0, then assumes infinite timeout
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt LONG64 conditionalVersion = -1) = 0;
            
            virtual ktl::Awaitable<bool> ConditionalRemoveAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout, // If less than 0, then assumes infinite timeout
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt LONG64 conditionalVersion = -1) = 0;
            
            virtual ktl::Awaitable<bool> ConditionalGetAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout,
                __out KeyValuePair<LONG64, TValue>& value,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            virtual ktl::Awaitable<bool> ContainsKeyAsync(
                __in IStoreTransaction<TKey, TValue>& storeTransaction,
                __in TKey key,
                __in Common::TimeSpan timeout, // If less than 0, then assumes infinite timeout
                __in ktl::CancellationToken const & cancellationToken) noexcept = 0;

            virtual ktl::Awaitable<KSharedPtr<Utilities::IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in ReadMode readMode=ReadMode::ReadValue) = 0;

            virtual ktl::Awaitable<KSharedPtr<Utilities::IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>>> CreateEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in TKey firstKey,
                __in bool useFirstKey,
                __in TKey lastKey,
                __in bool useLastKey,
                __in ReadMode readMode=ReadMode::ReadValue) = 0;

            virtual ktl::Awaitable<KSharedPtr<Data::IEnumerator<TKey>>> CreateKeyEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction) = 0;

            virtual ktl::Awaitable<KSharedPtr<Data::IEnumerator<TKey>>> CreateKeyEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in TKey firstKey) = 0;

            virtual ktl::Awaitable<KSharedPtr<Data::IEnumerator<TKey>>> CreateKeyEnumeratorAsync(
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in TKey firstKey,
                __in TKey lastKey) = 0;
            
            // Gets the BEST EFFORT number of items in the store at the moment the property was accessed
            // This property does not have transactional semantics
            __declspec(property(get = get_Count)) LONG64 Count;
            virtual LONG64 get_Count() const = 0;

            __declspec(property(get = get_DictionaryChangeHandler, put = set_DictionaryChangeHandler)) KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> DictionaryChangeHandlerSPtr;
            virtual KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> get_DictionaryChangeHandler() const = 0;
            virtual void set_DictionaryChangeHandler(KSharedPtr<IDictionaryChangeHandler<TKey, TValue>> dictionaryChangeHandlerSPtr) = 0;

            __declspec(property(get = get_DictionaryChangeHandlerMask, put = set_DictionaryChangeHandlerMask)) DictionaryChangeEventMask::Enum DictionaryChangeHandlerMask;
            virtual DictionaryChangeEventMask::Enum get_DictionaryChangeHandlerMask() const = 0;
            virtual void set_DictionaryChangeHandlerMask(DictionaryChangeEventMask::Enum mask) = 0;

            __declspec(property(get = get_CacheThresholdInMB, put = set_CacheThresholdInMB)) LONG64 CacheThresholdInMB;
            virtual LONG64 get_CacheThresholdInMB() const = 0;
            virtual void set_CacheThresholdInMB(__in LONG64 sizeInMegabytes) = 0;

            __declspec(property(get = get_Allocator)) KAllocator Allocator;
            virtual KAllocator& get_Allocator() const = 0;
        };
    }
}
