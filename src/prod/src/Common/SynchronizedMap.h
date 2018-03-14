// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TKey, class TValue>
    class SynchronizedMap
    {
        DENY_COPY(SynchronizedMap);
    public:

        explicit SynchronizedMap()
            : closed_(false), map_(), mapLock_()
        { 
        }

        ~SynchronizedMap() 
        { 
        }

        ErrorCode TryAdd(TKey const & key, TValue const & value);

        ErrorCode TryAddOrUpdate(TKey const & key, TValue const & value);

        bool Contains(TKey const & key);

        bool Remove(TKey const & key);

        std::vector<std::pair<TKey, TValue>> RemoveIf(std::function<bool(std::pair<TKey, TValue> const&)> const & predicate);

        bool TryGet(TKey const & key, __out TValue & value);

        bool TryGetAndRemove(TKey const & key, __out TValue & value);

        void Clear();

        size_t Count() const;

        // closes the map, so that no more additions are allowed
        std::map<TKey, TValue> Close();

    private:
        std::map<TKey, TValue> map_;
        Common::RwLock mapLock_;
        bool closed_;
    };

    template <class TKey, class TValue>
    ErrorCode SynchronizedMap<TKey, TValue>::TryAdd(TKey const & key, TValue const & value)
    {
        Common::AcquireWriteLock lock(this->mapLock_);
        if (closed_) return ErrorCodeValue::ObjectClosed;

        auto iter = this->map_.find(key);
        if (iter != this->map_.end())
        {
            return ErrorCodeValue::AlreadyExists;
        }
        
        this->map_.insert(std::make_pair(key, value));

        return ErrorCodeValue::Success;
    }

    template <class TKey, class TValue>
    ErrorCode SynchronizedMap<TKey, TValue>::TryAddOrUpdate(TKey const & key, TValue const & value)
    {
        AcquireWriteLock lock(this->mapLock_);

        if (closed_) return ErrorCodeValue::ObjectClosed;

        auto iter = this->map_.find(key);
        if (iter != this->map_.end())
        {
            iter->second = value;
        }
        else
        {
            map_.insert(std::make_pair(key, value));
        }

        return ErrorCodeValue::Success;
    }

    template <class TKey, class TValue>
    bool SynchronizedMap<TKey, TValue>::Contains(TKey const & key)
    {
        Common::AcquireReadLock lock(this->mapLock_);

        auto iter = this->map_.find(key);

        return iter != this->map_.end();
    }

    template <class TKey, class TValue>
    bool SynchronizedMap<TKey, TValue>::Remove(TKey const & key)
    {
        Common::AcquireWriteLock lock(this->mapLock_);

        auto iter = this->map_.find(key);
        if (iter != this->map_.end())
        {
            this->map_.erase(iter);

            return true;
        }

        return false;
    }

    template <class TKey, class TValue>
    std::vector<std::pair<TKey, TValue>> SynchronizedMap<TKey, TValue>::RemoveIf(std::function<bool(std::pair<TKey, TValue> const&)> const & predicate)
    {
        std::vector<std::pair<TKey, TValue>> removed;

        Common::AcquireWriteLock lock(this->mapLock_);

        for (auto iter = map_.begin(); iter != map_.end();)
        {
            if (predicate(*iter))
            {
                removed.emplace_back(move(*iter));
                iter = map_.erase(iter);
                continue;
            }

            ++iter;
        }

        return removed;
    }

    template <class TKey, class TValue>
    bool SynchronizedMap<TKey, TValue>::TryGet(TKey const & key, __out TValue & value)
    {
        Common::AcquireReadLock lock(this->mapLock_);

        auto iter = this->map_.find(key);
        if (iter != this->map_.end())
        {
            value = iter->second;

            return true;
        }

        return false;
    }

    template <class TKey, class TValue>
    bool SynchronizedMap<TKey, TValue>::TryGetAndRemove(TKey const & key, __out TValue & value)
    {
        Common::AcquireWriteLock lock(this->mapLock_);

        auto iter = this->map_.find(key);
        if (iter != this->map_.end())
        {
            value = iter->second;

            this->map_.erase(iter);

            return true;
        }

        return false;
    }

    template <class TKey, class TValue>
    size_t SynchronizedMap<TKey, TValue>::Count() const
    {
        Common::AcquireReadLock lock(this->mapLock_);

        return this->map_.size();
    }

    template <class TKey, class TValue>
    void SynchronizedMap<TKey, TValue>::Clear()
    {
        Common::AcquireWriteLock lock(this->mapLock_);

        this->map_.clear();
    }

    template <class TKey, class TValue>
    std::map<TKey, TValue> SynchronizedMap<TKey, TValue>::Close()
    {
        Common::AcquireWriteLock lock(this->mapLock_);

        closed_ = true;

        auto removedMap(std::move(this->map_));

        return removedMap;
    }
}
