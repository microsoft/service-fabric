// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        template <class T>
        class CacheEntry;

        template <class T>
        class LockedCacheEntry
        {
            DENY_COPY(LockedCacheEntry);

        public:
            LockedCacheEntry();

            LockedCacheEntry(std::shared_ptr<CacheEntry<T>> const & entry);

            LockedCacheEntry(LockedCacheEntry && other);

            ~LockedCacheEntry();

            LockedCacheEntry & operator=(LockedCacheEntry && other);

            __declspec(property(get = get_CacheEntry)) std::shared_ptr<CacheEntry<T>> Entry;
            std::shared_ptr<CacheEntry<T>> get_CacheEntry() const { return entry_; }

            explicit operator bool() const;

            std::shared_ptr<T> Get();

            T const* operator->() const;

            T const& operator*() const;

            T * operator->();

            T & operator*();

            void Release();

            void Commit(std::shared_ptr<T> && entry);

            __declspec(property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return entry_->IsDeleted; }
            void set_IsDeleted(bool value) { entry_->IsDeleted = value; }

        private:
            std::shared_ptr<CacheEntry<T>> entry_;
            bool isLocked_;
        };

        template <class T>
        class CacheEntry
        {
            DENY_COPY(CacheEntry);

            friend class LockedCacheEntry<T>;

        public:

            CacheEntry(std::shared_ptr<T> && entry);

            std::shared_ptr<T> Get();

            Common::ErrorCode Lock(
                std::shared_ptr<CacheEntry<T>> const & thisSPtr,
                LockedCacheEntry<T> & lockedEntry,
                Common::TimeSpan timeout = FailoverConfig::GetConfig().LockAcquireTimeout);

            __declspec(property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return isDeleted_; }
            void set_IsDeleted(bool value) { isDeleted_ = value; }

        private:
            void Commit(std::shared_ptr<T> && entry);
            void Unlock();

            std::shared_ptr<T> entry_;
            bool isLocked_;
#if !defined(PLATFORM_UNIX)
            mutable Common::ExclusiveLock lockObject_;
#else
            mutable Common::Mutex lockObject_;
#endif
            Common::ConditionVariable wait_;
            int waitCount_;
            bool isDeleted_;
        };

        typedef LockedCacheEntry<NodeInfo> LockedNodeInfo;
        typedef LockedCacheEntry<ServiceInfo> LockedServiceInfo;
        typedef LockedCacheEntry<ServiceType> LockedServiceType;
        typedef LockedCacheEntry<ApplicationInfo> LockedApplicationInfo;

        template <class T>
        struct NotFoundErrorCodeTraits
        {
            static Common::ErrorCode GetErrorCode();
        };

        template <>
        struct NotFoundErrorCodeTraits<NodeInfo>
        {
            static Common::ErrorCode GetErrorCode()
            {
                return Common::ErrorCodeValue::NodeNotFound;
            }
        };

        template <>
        struct NotFoundErrorCodeTraits<ServiceInfo>
        {
            static Common::ErrorCode GetErrorCode()
            {
                return Common::ErrorCodeValue::FMServiceDoesNotExist;
            }
        };

        template <>
        struct NotFoundErrorCodeTraits<ServiceType>
        {
            static Common::ErrorCode GetErrorCode()
            {
                return Common::ErrorCodeValue::ServiceTypeNotFound;
            }
        };

        template <>
        struct NotFoundErrorCodeTraits<ApplicationInfo>
        {
            static Common::ErrorCode GetErrorCode()
            {
                return Common::ErrorCodeValue::ApplicationNotFound;
            }
        };
    }
}
