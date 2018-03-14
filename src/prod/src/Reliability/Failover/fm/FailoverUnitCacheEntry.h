// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverUnitCacheEntry
        {
            DENY_COPY(FailoverUnitCacheEntry);

        public:
            FailoverUnitCacheEntry(FailoverManager& fm, FailoverUnitUPtr && failoverUnit);

            __declspec(property(get=get_FailoverUnit, put=set_FailoverUnit)) FailoverUnitUPtr & FailoverUnit;
            FailoverUnitUPtr & get_FailoverUnit() { return failoverUnit_; }
            void set_FailoverUnit(FailoverUnitUPtr && value) { failoverUnit_ = move(value); }

            __declspec(property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return isDeleted_; }
            void set_IsDeleted(bool value) { isDeleted_ = value; }

            void ProcessTaskAsync(DynamicStateMachineTaskUPtr && task, Federation::NodeInstance from, bool isFromPLB = false);

            std::vector<DynamicStateMachineTaskUPtr> const & GetExecutingTasks();

            bool Lock(Common::TimeSpan timeout, bool executeStateMachine, bool & isDeleted);
            bool Release(bool restoreExecutingTask, bool processPendingTask);

        private:
            FailoverManager& fm_;
            FailoverUnitUPtr failoverUnit_;
#if !defined(PLATFORM_UNIX)
            MUTABLE_RWLOCK(FM.FailoverUnitCacheEntry, lock_);
#else
            MUTABLE_MUTEX(FM.FailoverUnitCacheEntry, lock_);
#endif
            Common::ConditionVariable wait_;
            std::vector<DynamicStateMachineTaskUPtr> pendingTasks_;
            std::vector<DynamicStateMachineTaskUPtr> executingTasks_; // This is owned by the LockedFailoverUnit, not the entry itself.
            int waitCount_;
            bool isFree_;
            bool isDeleted_;
        };
    }
}
