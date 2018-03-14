// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class PendingItemManager
    {
        DENY_COPY(PendingItemManager);
    public:
        PendingItemManager();

        void AddPendingItem(std::wstring const & name, Common::ComponentRootWPtr const &);
        bool ItemExists(std::wstring const & name);
        void RemovePendingItem(std::wstring const & name, bool skipExistenceCheck = false);
        void ClosePendingItem(std::wstring const & name);
        std::map<std::wstring, Common::TimeSpan> PendingItemManager::GetPendingItems(std::wstring const filter) const;
        std::map<std::wstring, Common::TimeSpan> PendingItemManager::GetPendingItems(std::wstring const filter, int64 seconds) const;

    private:
        void CheckPendingItems() const;

        std::map<std::wstring, Common::ComponentRootWPtr> pendingList_;
        std::map<std::wstring, Common::DateTime> pendingCleanup_;

        mutable Common::ExclusiveLock pendingLock_;
    };

}
