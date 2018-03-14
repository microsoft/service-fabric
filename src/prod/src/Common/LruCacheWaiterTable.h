// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // This class maintains a table of waiter lists, used by
    // LruCache to track a waiter list for each cache entry key.
    //
    template <typename TKey, typename TEntry>
    class LruCacheWaiterTable
    {
        DENY_COPY(LruCacheWaiterTable)

    private:

        typedef LruCacheWaiterList<TEntry> WaiterList;
        typedef std::shared_ptr<WaiterList> WaiterListSPtr;

    public: 

        LruCacheWaiterTable()
            : waitersHash_()
            , waitersHashLock_()
        {
        }

        explicit LruCacheWaiterTable(size_t bucketCount)
            : waitersHash_(bucketCount)
            , waitersHashLock_()
        {
        }

        size_t GetWaiterCount(TKey const & key)
        {
            AcquireReadLock lock(waitersHashLock_);

            auto it = waitersHash_.find(key);
            if (it != waitersHash_.end())
            {
                return it->second->GetSize();
            }

            return 0;
        }

        std::shared_ptr<LruCacheWaiterAsyncOperation<TEntry>> AddWaiter(
            TKey const & key,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            WaiterListSPtr list;
            {
                AcquireWriteLock lock(waitersHashLock_);

                auto it = waitersHash_.find(key);
                if (it == waitersHash_.end())
                {
                    it = waitersHash_.insert(std::pair<TKey, WaiterListSPtr>(
                        key,
                        std::make_shared<WaiterList>())).first;
                }

                list = it->second;
            }

            return list->AddWaiter(
                timeout,
                callback,
                parent);
        }

        WaiterListSPtr TakeWaiters(TKey const & key)
        {
            WaiterListSPtr list;
            {
                AcquireWriteLock lock(waitersHashLock_);

                auto it = waitersHash_.find(key);
                if (it != waitersHash_.end())
                {
                    list = std::move(it->second);

                    waitersHash_.erase(it);
                }
            }

            return list;
        }

    private:
        template <typename TKey, typename TEntry>
        struct Hasher
        {
            size_t operator() (TKey const & key) const { return TEntry::GetHash(key); }
            bool operator() (TKey const & left, TKey const & right) const { return TEntry::AreEqualKeys(left, right); }
        };

        std::unordered_map<
            TKey, 
            WaiterListSPtr,
            Hasher<TKey, TEntry>, 
            Hasher<TKey, TEntry>> waitersHash_;
        mutable RwLock waitersHashLock_;
    };
}
