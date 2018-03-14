// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace TestCommon
{
    using namespace std;
    using namespace Common;

    PendingItemManager::PendingItemManager()
    {
    }

    void PendingItemManager::AddPendingItem(std::wstring const & name, ComponentRootWPtr const & weakRoot)
    {
        AcquireWriteLock acquire(pendingLock_);

        auto it = pendingList_.find(name);
        if (it != pendingList_.end())
        {
            auto existingWeakRoot = it->second;
            auto existingRoot = existingWeakRoot.lock();

            if (existingRoot.get() != nullptr)
            {
                existingRoot->TraceTrackedReferences();
            }

            TestSession::FailTest("{0} already in pending list", name);
        }
        pendingList_[name] = weakRoot;
    }

    bool PendingItemManager::ItemExists(std::wstring const & name)
    {
        AcquireWriteLock acquire(pendingLock_);
        return (pendingList_.find(name) != pendingList_.end());
    }

    void PendingItemManager::RemovePendingItem(std::wstring const & name, bool skipExistenceCheck)
    {
        AcquireWriteLock acquire(pendingLock_);
        pendingCleanup_.erase(name);

        auto it = pendingList_.find(name);
        if (it == pendingList_.end())
        {
            TestSession::FailTestIfNot(skipExistenceCheck, "{0} not in pending list", name);
        }
        else
        {
            pendingList_.erase(it);

            CheckPendingItems();
        }
    }

    void PendingItemManager::ClosePendingItem(std::wstring const & name)
    {
        AcquireWriteLock acquire(pendingLock_);
        pendingCleanup_[name] = DateTime::Now();

        CheckPendingItems();
    }

    std::map<std::wstring, TimeSpan> PendingItemManager::GetPendingItems(std::wstring const filter, int64 seconds) const
    {
        DateTime now = DateTime::Now();
        std::map<std::wstring, TimeSpan> pendingItems;
        AcquireReadLock acquire(pendingLock_);
                
        for (auto const & currentItem : pendingList_)
        {
            auto const & name = currentItem.first;

            if (filter.size() == 0 || StringUtility::Contains<wstring>(name, filter))
            {
                auto pendingCleanupItem = pendingCleanup_.find(name);
                if (pendingCleanupItem != pendingCleanup_.end())
                {
                    TimeSpan t = now - pendingCleanupItem->second;
                    if (t.TotalSeconds() >= seconds)
                    {
                        pendingItems.insert(std::pair<std::wstring, TimeSpan>(name, t));
                    }
                }
            }
        }

        return pendingItems;
    }

    std::map<std::wstring, TimeSpan> PendingItemManager::GetPendingItems(std::wstring const filter) const
    {
        DateTime now = DateTime::Now();
        std::map<std::wstring, TimeSpan> pendingItems;
        AcquireReadLock acquire(pendingLock_);
        for (auto const & currentItem : pendingList_)
        {
            auto const & name = currentItem.first;

            if (filter.size() == 0 || StringUtility::Contains<wstring>(name, filter))
            {
                auto pendingCleanupItem = pendingCleanup_.find(name);
                if (pendingCleanupItem != pendingCleanup_.end())
                {
                    TimeSpan t = now - pendingCleanupItem->second;
                    pendingItems.insert(std::pair<std::wstring, TimeSpan>(name, t));
                }
                else
                {
                    pendingItems.insert(std::pair<std::wstring, TimeSpan>(name, TimeSpan::MinValue));
                }
            }
        }

        return pendingItems;
    }

    void PendingItemManager::CheckPendingItems() const
    {
        DateTime now = DateTime::Now();
        for (auto it = pendingCleanup_.begin(); it != pendingCleanup_.end(); ++it)
        {
            TimeSpan t = now - it->second;
            if (t > TimeSpan::FromSeconds(5))
            {
                TestSession::WriteWarning("Pending", "{0} pending cleanup for {1}", it->first, t);

                TestSession::FailTestIf(t > TimeSpan::FromMinutes(5), "Leaked item - {0}", it->first);
            }
        }
    }

}
