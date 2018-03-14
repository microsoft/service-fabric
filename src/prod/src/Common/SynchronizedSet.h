// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TKey>
    class SynchronizedSet
    {
        DENY_COPY(SynchronizedSet);
    public:

        explicit SynchronizedSet()
            : closed_(false), set_(), setLock_()
        { 
        }

        ~SynchronizedSet() 
        { 
        }

        ErrorCode TryAdd(TKey const & key);        

        ErrorCode Contains(TKey const & key, __out bool & contains);

        bool Remove(TKey const & key);                

        void Clear();

        size_t Count() const;

        // closes the set, so that no more additions are allowed
        std::set<TKey> Close();

    private:
        std::set<TKey> set_;
        Common::RwLock setLock_;
        bool closed_;
    };

    template <class TKey>
    ErrorCode SynchronizedSet<TKey>::TryAdd(TKey const & key)
    {
        Common::AcquireWriteLock lock(this->setLock_);
        if (closed_) return ErrorCodeValue::ObjectClosed;

        auto iter = this->set_.find(key);
        if (iter != this->set_.end())
        {
            return ErrorCodeValue::AlreadyExists;
        }
        
        this->set_.insert(key);

        return ErrorCodeValue::Success;
    }   

    template <class TKey>
    ErrorCode SynchronizedSet<TKey>::Contains(TKey const & key, __out bool & contains)
    {
        Common::AcquireReadLock lock(this->setLock_);
        if (closed_) return ErrorCodeValue::ObjectClosed;

        auto iter = this->set_.find(key);

        contains = (iter != this->set_.end());

        return ErrorCodeValue::Success;
    }

    template <class TKey>
    bool SynchronizedSet<TKey>::Remove(TKey const & key)
    {
        Common::AcquireWriteLock lock(this->setLock_);
        if (closed_) { return false; }

        auto iter = this->set_.find(key);
        if (iter != this->set_.end())
        {
            this->set_.erase(iter);

            return true;
        }

        return false;
    }    

    template <class TKey>
    size_t SynchronizedSet<TKey>::Count() const
    {
        Common::AcquireReadLock lock(this->setLock_);

        return this->set_.size();
    }

    template <class TKey>
    void SynchronizedSet<TKey>::Clear()
    {
        Common::AcquireWriteLock lock(this->setLock_);

        this->set_.clear();
    }

    template <class TKey>
    std::set<TKey> SynchronizedSet<TKey>::Close()
    {
        Common::AcquireWriteLock lock(this->setLock_);

        closed_ = true;

        auto removedSet(std::move(this->set_));

        return removedSet;
    }
}
