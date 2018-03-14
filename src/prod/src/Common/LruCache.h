// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Cache structure that supports LRU eviction in constant time while
    // still allowing writes/reads to the main cache (hash table) in
    // constant time.
    //
    // This cache also supports tracking of async waiters for cache misses
    // on a per-cache entry basis. This allows the first async operation
    // that encounters a cache miss to handle the miss while blocking
    // subsequent waiters and avoiding duplicate cache miss processing. 
    //
    template <typename TKey>
    class LruCacheEntryBase
    {
        DENY_COPY(LruCacheEntryBase)

    public:
        explicit LruCacheEntryBase(TKey const & key) : key_(key) { }
        explicit LruCacheEntryBase(TKey && key) : key_(std::move(key)) { }

        TKey const & GetKey() const { return key_; }

        static size_t GetHash(TKey const &);

        bool operator == (LruCacheEntryBase<TKey> const & other) const { return key_ == other.key_; }

    private:
        TKey key_;
    };

    template <typename TKey, typename TEntry>
    class LruCache : public TextTraceComponent<TraceTaskCodes::Client>
    {
        DENY_COPY(LruCache)

    private:
        typedef EmbeddedListEntry<TEntry> HashEntry;
        typedef std::shared_ptr<EmbeddedListEntry<TEntry>> HashEntrySPtr;

        typedef EmbeddedList<TEntry> EvictionList;
        typedef std::shared_ptr<EvictionList> EvictionListSPtr;

    public:
        LruCache(size_t cacheLimit)
            : cacheLimit_(cacheLimit)
            , hash_()
            , hashLock_()
            , evictionList_(std::make_shared<EvictionList>())
            , waitersTable_()
        {
        }

        LruCache(size_t cacheLimit, size_t bucketCount)
            : cacheLimit_(cacheLimit)
            , hash_(bucketCount)
            , hashLock_()
            , evictionList_(std::make_shared<EvictionList>())
            , waitersTable_(bucketCount)
        {
        }

        virtual ~LruCache()
        {
            evictionList_->Clear();
        }

        __declspec(property(get=get_Size)) size_t Size;
        size_t get_Size() const { return hash_.size(); }

        __declspec(property(get=get_EvictionListSize)) size_t EvictionListSize;
        size_t get_EvictionListSize() const { return evictionList_->GetSize(); }

        __declspec(property(get=get_CacheLimit)) size_t CacheLimit;
        size_t get_CacheLimit() const { return cacheLimit_; }

        __declspec(property(get=get_IsCacheLimitEnabled)) bool IsCacheLimitEnabled;
        bool get_IsCacheLimitEnabled() const { return (cacheLimit_ > 0); }

        size_t GetWaiterCount(TKey const & key)
        {
            return waitersTable_.GetWaiterCount(key);
        }

        bool TryPutOrGet(__inout std::shared_ptr<TEntry> & item)
        {
            if (!item) { return false; }

            bool updated = false;

            auto hashEntry = std::make_shared<HashEntry>(item);

            {
                AcquireWriteLock lock(hashLock_); 

                auto it = hash_.insert(std::pair<TKey, std::shared_ptr<HashEntry>>(
                    hashEntry->GetListEntry()->GetKey(),
                    hashEntry));

                if (it.second)
                {
                    updated = true;

                    if (cacheLimit_ > 0)
                    {
                        UpdateLruHeadAndTrim_WriteHashLock(hashEntry);
                    }
                }
                else
                {
                    auto existing = it.first;

                    if (TEntry::ShouldUpdateUnderLock(
                        *(existing->second->GetListEntry()),
                        *(hashEntry->GetListEntry())))
                    {
                        // Update the actual list entry data, but preserve
                        // the embedded list entry links to keep the
                        // entry count consistent.
                        //
                        existing->second->SetListEntry(hashEntry);

                        updated = true;

                        if (cacheLimit_ > 0)
                        {
                            UpdateLruHeadAndTrim_WriteHashLock(existing->second);
                        }
                    }
                    else
                    {
                        // updated is false
                        //
                        item = existing->second->GetListEntry();
                    }
                }
            }

            if (updated)
            {
                this->UpdateWaiters(hashEntry->GetListEntry());
            }

            return updated;
        }

        bool TryRemove(TKey const & key)
        {
            AcquireWriteLock lock(hashLock_);

            auto it = hash_.find(key);
            if (it != hash_.end())
            {
                if (cacheLimit_ > 0)
                {
                    RemoveLruEntry_WriteHashLock(it->second);
                }

                hash_.erase(it);

                return true;
            }
            else
            {
                return false;
            }
        }

        bool TryGet(TKey const & key, __out std::shared_ptr<TEntry> & result) const
        {
            bool found = false;

            {
                AcquireReadLock lock(hashLock_); 

                auto it = hash_.find(key);
                if (it != hash_.end())
                {
                    auto const & hashEntry = it->second;

                    if (cacheLimit_ > 0)
                    {
                        UpdateLruHead_ReadHashLock(hashEntry);
                    }

                    result = hashEntry->GetListEntry();

                    found = true;
                }
            }

            return found;
        }

        AsyncOperationSPtr BeginTryGet(
            TKey const & key, 
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        {
            std::shared_ptr<LruCacheWaiterAsyncOperation<TEntry>> waiter;
            {
                AcquireReadLock lock(hashLock_); 

                auto it = hash_.find(key);
                if (it != hash_.end())
                {
                    auto const & hashEntry = it->second;

                    if (cacheLimit_ > 0)
                    {
                        UpdateLruHead_ReadHashLock(hashEntry);
                    }

                    waiter = LruCacheWaiterAsyncOperation<TEntry>::Create(
                        hashEntry->GetListEntry(),
                        callback,
                        parent);
                }
                else
                {
                    waiter = waitersTable_.AddWaiter(key, timeout, callback, parent);
                }
            }

            waiter->StartOutsideLock(waiter);

            return waiter;
        }

        ErrorCode EndTryGet(
            AsyncOperationSPtr const & operation, 
            __out bool & isFirstWaiter, 
            __out std::shared_ptr<TEntry> & entry)
        {
            return LruCacheWaiterAsyncOperation<TEntry>::End(operation, isFirstWaiter, entry);
        }

        // Refresh is used to update cache contents (leveraging
        // the waiter queues) without removing the current entries.
        // This allows continued reading of possibly stale cache
        // entries while the refresh is pending.
        //
        AsyncOperationSPtr BeginTryRefresh(
            TKey const & key, 
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        {
            return BeginTryInvalidate(key, nullptr, timeout, callback, parent);
        }

        ErrorCode EndTryRefresh(
            AsyncOperationSPtr const & operation, 
            __out bool & isFirstWaiter, 
            __out std::shared_ptr<TEntry> & entry)
        {
            return EndTryInvalidate(operation, isFirstWaiter, entry);
        }

        // Invalidate is effectively Remove() followed by TryGet()
        // combined under the lock
        //
        AsyncOperationSPtr BeginTryInvalidate(
            std::shared_ptr<TEntry> const & item,
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        {
            return BeginTryInvalidate(item->GetKey(), item, timeout, callback, parent);
        }

        ErrorCode EndTryInvalidate(
            AsyncOperationSPtr const & operation, 
            __out bool & isFirstWaiter, 
            __out std::shared_ptr<TEntry> & entry)
        {
            return LruCacheWaiterAsyncOperation<TEntry>::End(operation, isFirstWaiter, entry);
        }

        void CancelWaiters(TKey const & key)
        {
            this->FailWaiters(key, Common::ErrorCodeValue::OperationCanceled);
        }

        void FailWaiters(TKey const & key, Common::ErrorCode const & error)
        {
            auto list = waitersTable_.TakeWaiters(key);

            if (list)
            {
                list->CompleteWaiters(error);
            }
        }

        void CompleteWaitersWithMockEntry(std::shared_ptr<TEntry> const & mockEntry)
        {
            this->UpdateWaiters(mockEntry);
        }

    private:
        template <typename TKey, typename TEntry>
        struct Hasher
        {
            size_t operator() (TKey const & key) const { return TEntry::GetHash(key); }
            bool operator() (TKey const & left, TKey const & right) const { return TEntry::AreEqualKeys(left, right); }
        };

        AsyncOperationSPtr BeginTryInvalidate(
            TKey const & key,
            std::shared_ptr<TEntry> const & item,
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        {
            std::shared_ptr<LruCacheWaiterAsyncOperation<TEntry>> waiter;
            {
                AcquireWriteLock lock(hashLock_); 

                if (item)
                {
                    auto it = hash_.find(key);
                    if (it != hash_.end())
                    {
                        if (it->second->GetListEntry().get() == item.get())
                        {
                            if (cacheLimit_ > 0)
                            {
                                RemoveLruEntry_WriteHashLock(it->second);
                            }

                            hash_.erase(it);
                        }
                        else
                        {
                            waiter = LruCacheWaiterAsyncOperation<TEntry>::Create(
                                it->second->GetListEntry(),
                                callback,
                                parent);
                        }
                    }
                }

                if (!waiter)
                {
                    waiter = waitersTable_.AddWaiter(key, timeout, callback, parent);
                }
            }

            waiter->StartOutsideLock(waiter);

            return waiter;
        }
        void UpdateLruHeadAndTrim_WriteHashLock(HashEntrySPtr const & hashEntry)
        {
            auto removed = evictionList_->UpdateHeadAndTrim(
                hashEntry, 
                cacheLimit_);

            if (removed)
            {
                hash_.erase(removed->GetListEntry()->GetKey());

                CommonEventSource::Events->TraceLruCacheEviction(
                    wformatString(hashEntry->GetListEntry()->GetKey()),
                    wformatString(removed->GetListEntry()->GetKey()), 
                    cacheLimit_,
                    hash_.size(),
                    evictionList_->GetSize());
            }
        }

        void UpdateLruHead_ReadHashLock(HashEntrySPtr const & hashEntry) const
        {
            // Only need fine-grained locking of the embedded LRU list since
            // the cache entries themselves are not being updated.
            //
            evictionList_->UpdateHead(hashEntry);
        }

        void RemoveLruEntry_WriteHashLock(HashEntrySPtr const & hashEntry) const
        {
            evictionList_->RemoveFromList(hashEntry);
        }

        void UpdateWaiters(std::shared_ptr<TEntry> const & item)
        {
            auto list = waitersTable_.TakeWaiters(item->GetKey());

            if (list)
            {
                list->CompleteWaiters(item);
            }
        }

        size_t cacheLimit_;

        // Cache: use unordered_map instead of unordered_set.
        //
        // Trade-off: will store an extra copy of TKey
        // but reads can use TKey directly without 
        // constructing a TEntry in order to do the hash
        // lookup.
        //
        std::unordered_map<
            TKey, 
            HashEntrySPtr,
            Hasher<TKey, TEntry>, 
            Hasher<TKey, TEntry>> hash_;
        mutable RwLock hashLock_;

        // The eviction list is just a link list embedded
        // into the cache entries themselves. Cache entry
        // access moves the entry to the head of the list. 
        // Eviction occurs by removing the tail entry
        // of the list.
        //
        mutable EvictionListSPtr evictionList_;

        // Support waiting on cache updates (asynchronously blocks waiters)
        //
        LruCacheWaiterTable<TKey, TEntry> waitersTable_;
    };
}
