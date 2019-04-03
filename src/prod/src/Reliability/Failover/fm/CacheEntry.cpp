// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

template <class T>
CacheEntry<T>::CacheEntry(shared_ptr<T> && entry)
    : entry_(move(entry)),
      isLocked_(false),
      lockObject_(),
      wait_(lockObject_),
      waitCount_(0),
      isDeleted_(false)
{
}

template <class T>
shared_ptr<T> CacheEntry<T>::Get()
{
#if !defined(PLATFORM_UNIX)
    AcquireExclusiveLock lock(lockObject_);
#else
    AcquireMutexLock lock(&lockObject_);
#endif

    return entry_;
}

template <class T>
ErrorCode CacheEntry<T>::Lock(shared_ptr<CacheEntry<T>> const & thisSPtr, LockedCacheEntry<T> & lockedEntry, TimeSpan timeout)
{
#if !defined(PLATFORM_UNIX)
    AcquireExclusiveLock lock(lockObject_);
#else
    AcquireMutexLock lock(&lockObject_);
#endif

    if (isLocked_)
    {
        StopwatchTime endTime = Stopwatch::Now() + timeout;
        do
        {
            TimeSpan waitTime = endTime - Stopwatch::Now();
            if (waitTime <= TimeSpan::Zero)
            {
                return ErrorCodeValue::UpdatePending;
            }

            waitCount_++;
            wait_.Sleep(waitTime);
            waitCount_--;
        } while (isLocked_);
    }

    if (isLocked_)
    {
        return ErrorCodeValue::UpdatePending;
    }
    else if (isDeleted_)
    {
        return NotFoundErrorCodeTraits<T>::GetErrorCode();
    }
    else
    {
        isLocked_ = true;

        lockedEntry = LockedCacheEntry<T>(thisSPtr);
        return ErrorCode::Success();
    }
}

template <class T>
void CacheEntry<T>::Commit(shared_ptr<T> && entry)
{
    bool waiting = false;

    {
#if !defined(PLATFORM_UNIX)
        AcquireExclusiveLock lock(lockObject_);
#else
        AcquireMutexLock lock(&lockObject_);
#endif

        ASSERT_IFNOT(isLocked_, "Update called on an entry that is not getting updated: {0}", *entry_);
        ASSERT_IFNOT(entry.get() != entry_.get(), "Update called on the same entry: {0}", *entry_);

        __if_exists(T::IsStale)
        {
            ASSERT_IF(entry->IsStale, "New entry is stale: {0}", *entry);
            entry_->IsStale = true;
        }

        entry_ = move(entry);

        if (waitCount_ > 0)
        {
            waiting = true;
        } 

        isLocked_ = false;
    }

    if (waiting)
    {
        wait_.Wake();
    }
}

template <class T>
void CacheEntry<T>::Unlock()
{
    bool waiting = false;

    {
#if !defined(PLATFORM_UNIX)
        AcquireExclusiveLock lock(lockObject_);
#else
        AcquireMutexLock lock(&lockObject_);
#endif

        ASSERT_IFNOT(isLocked_, "Unlock called on an entry that is not getting updated: {0}", *entry_);

        if (waitCount_ > 0)
        {
            waiting = true;
        }

        isLocked_ = false;
    }

    if (waiting)
    {
        wait_.Wake();
    }
}

template <class T>
LockedCacheEntry<T>::LockedCacheEntry()
    : entry_(nullptr),
      isLocked_(false)
{
}

template <class T>
LockedCacheEntry<T>::LockedCacheEntry(std::shared_ptr<CacheEntry<T>> const & entry)
    : entry_(entry),
      isLocked_(true)
{
}

template <class T>
LockedCacheEntry<T>::LockedCacheEntry(LockedCacheEntry && other)
    : entry_(std::move(other.entry_)),
      isLocked_(other.isLocked_)
{
}

template <class T>
LockedCacheEntry<T>::~LockedCacheEntry()
{
    Release();
}

template <class T>
LockedCacheEntry<T> & LockedCacheEntry<T>::operator=(LockedCacheEntry<T> && other)
{
    if (this != &other)
    {
        entry_ = std::move(other.entry_);
        isLocked_ = other.isLocked_;
    }

    return *this;
}

template <class T>
LockedCacheEntry<T>::operator bool() const
{
    return (entry_ != nullptr);
}

template <class T>
std::shared_ptr<T> LockedCacheEntry<T>::Get()
{
    return entry_->Get();
}

template <class T>
T const* LockedCacheEntry<T>::operator->() const
{
    return entry_->Get().get();
}

template <class T>
T const& LockedCacheEntry<T>::operator*() const
{
    return *(entry_->Get().get());
}

template <class T>
T* LockedCacheEntry<T>::operator->()
{
    return entry_->Get().get();
}

template <class T>
T& LockedCacheEntry<T>::operator*()
{
    return *(entry_->Get().get());
}

template <class T>
void LockedCacheEntry<T>::Release()
{
    if (entry_ && isLocked_)
    {
        entry_->Unlock();
        isLocked_ = false;
    }
}

template <class T>
void LockedCacheEntry<T>::Commit(std::shared_ptr<T> && entry)
{
    ASSERT_IF(!isLocked_, "Commit called on LockedCacheEntry that is already released");
    entry_->Commit(move(entry));
    isLocked_ = false;
}

template class Reliability::FailoverManagerComponent::CacheEntry<NodeInfo>;
template class Reliability::FailoverManagerComponent::CacheEntry<ServiceInfo>;
template class Reliability::FailoverManagerComponent::CacheEntry<ServiceType>;
template class Reliability::FailoverManagerComponent::CacheEntry<ApplicationInfo>;

template class Reliability::FailoverManagerComponent::LockedCacheEntry<NodeInfo>;
template class Reliability::FailoverManagerComponent::LockedCacheEntry<FailoverManagerComponent::ServiceInfo>;
template class Reliability::FailoverManagerComponent::LockedCacheEntry<ServiceType>;
template class Reliability::FailoverManagerComponent::LockedCacheEntry<ApplicationInfo>;

template class Reliability::FailoverManagerComponent::CacheEntry<Management::NetworkInventoryManager::NetworkManager>;
template class Reliability::FailoverManagerComponent::LockedCacheEntry<Management::NetworkInventoryManager::NetworkManager>;

template class Reliability::FailoverManagerComponent::CacheEntry<Management::NetworkInventoryManager::NIMNetworkMACAddressPoolStoreData>;
template class Reliability::FailoverManagerComponent::LockedCacheEntry<Management::NetworkInventoryManager::NIMNetworkMACAddressPoolStoreData>;

template <>
struct NotFoundErrorCodeTraits<Management::NetworkInventoryManager::NetworkManager>
{
    static Common::ErrorCode GetErrorCode()
    {
        return Common::ErrorCodeValue::NodeNotFound;
    }
};

template <>
struct NotFoundErrorCodeTraits<Management::NetworkInventoryManager::NIMNetworkMACAddressPoolStoreData>
{
    static Common::ErrorCode GetErrorCode()
    {
        return Common::ErrorCodeValue::NodeNotFound;
    }
};