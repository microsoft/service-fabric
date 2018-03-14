// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class LruClientCacheManager::ProcessServiceNotificationAsyncOperation 
        : public CacheAsyncOperationBase
    {
        DENY_COPY(ProcessServiceNotificationAsyncOperation)

    public:
        ProcessServiceNotificationAsyncOperation(
            __in LruClientCacheManager &,
            __in Naming::ServiceNotification &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out std::vector<ServiceNotificationResultSPtr> &);

        void OnStart(Common::AsyncOperationSPtr const &);

    protected:

        virtual void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &)
        {
            // Intentional no-op
        }

        virtual void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &)
        {
            // Intentional no-op
        }

    private:

        void InitializeStes(__in Naming::ServiceNotification &);

        void UpdateCacheEntries(Common::AsyncOperationSPtr const &);

        void OnUpdateCacheEntryComplete(
            Common::NamingUri const &,
            Reliability::ServiceTableEntryNotificationSPtr const &,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        std::multimap<Common::NamingUri, Reliability::ServiceTableEntryNotificationSPtr> steNotifications_;
        Reliability::GenerationNumber generation_;
        Common::atomic_long pendingCount_;

        std::vector<ServiceNotificationResultSPtr> notificationResults_;
        Common::ErrorCode error_;
        Common::ExclusiveLock resultsLock_;
    };
}
