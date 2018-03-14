// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        /// <summary>
        /// Wraps an FailoverUnit that is checked out from the cache.
        /// </summary>
        class LockedFailoverUnitPtr
        {
        public:
            LockedFailoverUnitPtr();

            LockedFailoverUnitPtr(FailoverUnitCacheEntrySPtr const & entry);

            LockedFailoverUnitPtr(LockedFailoverUnitPtr && other);

            LockedFailoverUnitPtr(LockedFailoverUnitPtr const & other);

            LockedFailoverUnitPtr & operator=(LockedFailoverUnitPtr && other);

            ~LockedFailoverUnitPtr();

            __declspec(property(get=get_Old)) FailoverUnitUPtr const& Old;
            FailoverUnitUPtr const& get_Old() const { return old_; }

            __declspec(property(get=get_Current)) FailoverUnitUPtr& Current;
            FailoverUnitUPtr& get_Current() const { return entry_->FailoverUnit; }

            __declspec(property(get=get_IsUpdating)) bool IsUpdating;
            bool get_IsUpdating() const { return isUpdating_; }

            __declspec(property(get = get_SkipPersistence)) bool SkipPersistence;
            bool get_SkipPersistence() const { return skipPersistence_; }

            void EnableUpdate(bool skipPersistence = false);

            // Revert the modified FailoverUnit to the old one
            void Revert();

            // Clear the old FailoverUnit
            void Submit();

            FailoverUnit const* operator->() const;

            FailoverUnit const& operator*() const;

            FailoverUnit * operator->();

            FailoverUnit & operator*();

            explicit operator bool() const;

            std::vector<DynamicStateMachineTaskUPtr> const & GetExecutingTasks() const
            {
                return entry_->GetExecutingTasks();
            }

            bool Release(bool restoreExecutingTask, bool processPendingTask);

        private:
            FailoverUnit* get() const;

            mutable FailoverUnitUPtr old_;
            mutable FailoverUnitCacheEntrySPtr entry_;
            bool isUpdating_;
            bool skipPersistence_;
        };
    }
}
