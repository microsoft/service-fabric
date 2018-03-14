// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // LruPrefixCache is a wrapper around LruCache and shares the same
    // underlying entry cache, but maintains its own list of waiters
    // for the purpose of de-duplicating requests. Prefix resolution
    // is a separate request type processed differently from exact
    // resolution by the Naming Store Service.
    //
    // This also implies that prefix resolution requests can complete 
    // exact resolution waiters, but not vice versa.
    //
    // e.g.
    //
    // 1) Prefix: a/b/c/d
    // 2) Exact: a/b
    //
    // 1) will complete waiters on 2) if the request resolves to
    // the a/b prefix. It's possible to implement 2) calling back
    // into the prefix cache in order to search for and complete
    // 1), but that's currently not known to be a useful scenario
    // for optimization.
    //
    template <typename TKey, typename TEntry>
    class LruPrefixCache
    {
    public:
        typedef LruCache<TKey, TEntry> InnerCacheType;
        typedef LruCacheWaiterTable<TKey, TEntry> WaiterTableType;
        typedef std::function<TKey(NamingUri const &)> InnerCacheKeyMapper;

        LruPrefixCache(__in InnerCacheType & innerCache)
            : innerCache_(innerCache)
            , waitersTable_()
            , innerCacheKeyMapper_(DefaultInnerCacheKeyMapper)
        {
        }

        LruPrefixCache(
            __in InnerCacheType & innerCache,
            InnerCacheKeyMapper const & func)
            : innerCache_(innerCache)
            , waitersTable_()
            , innerCacheKeyMapper_(func)
        {
        }

        __declspec(property(get=get_InnerCache)) InnerCacheType & InnerCache;
        InnerCacheType & get_InnerCache() const { return innerCache_; }

        // Inner cache may be backed by a different key type, but the prefix
        // cache itself is always "keyed" by URI.
        //
        AsyncOperationSPtr BeginTryGet(
            NamingUri const & key, 
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        {
            std::shared_ptr<LruCacheWaiterAsyncOperation<TEntry>> waiter;

            for (auto prefix = key;
                !prefix.IsRootNamingUri;
                prefix = prefix.GetParentName())
            {
                std::shared_ptr<TEntry> result;
                if (innerCache_.TryGet(innerCacheKeyMapper_(prefix), result))
                {
                    waiter = LruCacheWaiterAsyncOperation<TEntry>::Create(
                        result,
                        callback,
                        parent);

                    break;
                }
            }

            if (!waiter)
            {
                waiter = waitersTable_.AddWaiter(innerCacheKeyMapper_(key), timeout, callback, parent);
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

        bool TryPutOrGet(NamingUri const & key, __inout std::shared_ptr<TEntry> & item)
        {
            bool putResult = innerCache_.TryPutOrGet(item);

            auto list = waitersTable_.TakeWaiters(innerCacheKeyMapper_(key));

            if (list)
            {
                list->CompleteWaiters(item);
            }

            return putResult;
        }

        void CancelWaiters(NamingUri const & key)
        {
            this->FailWaiters(key, ErrorCodeValue::OperationCanceled);
        }

        void FailWaiters(NamingUri const & key, ErrorCode const & error)
        {
            auto list = waitersTable_.TakeWaiters(innerCacheKeyMapper_(key));

            if (list)
            {
                list->CompleteWaiters(error);
            }
        }

        void CompleteWaitersWithMockEntry(NamingUri const & key, std::shared_ptr<TEntry> const & mockEntry)
        {
            innerCache_.CompleteWaitersWithMockEntry(mockEntry);

            auto list = waitersTable_.TakeWaiters(innerCacheKeyMapper_(key));

            if (list)
            {
                list->CompleteWaiters(mockEntry);
            }
        }

    private:

        static TKey DefaultInnerCacheKeyMapper(NamingUri const & key)
        {
            return key;
        }

        InnerCacheType & innerCache_;
        WaiterTableType waitersTable_;
        InnerCacheKeyMapper innerCacheKeyMapper_;
    };
}
