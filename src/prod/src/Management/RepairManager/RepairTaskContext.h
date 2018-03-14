// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairTaskContext : public Store::StoreData
        {
        public:

            RepairTaskContext();

            RepairTaskContext(RepairTask const &);

            RepairTaskContext(RepairTaskContext &&);

            RepairTaskContext const & operator = (RepairTaskContext const &);

            virtual ~RepairTaskContext();

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            __declspec(property(get=get_Task)) RepairTask & Task;
            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_TaskState)) RepairTaskState::Enum TaskState;
            __declspec(property(get=get_GetStatusOnly, put=set_GetStatusOnly)) bool GetStatusOnly;
            __declspec(property(get=get_NeedsRefresh, put=set_NeedsRefresh)) bool NeedsRefresh;

            RepairTask & get_Task() { return task_; }
            std::wstring const & get_TaskId() const { return task_.TaskId; }
            RepairTaskState::Enum get_TaskState() const { return task_.State; }
            bool get_GetStatusOnly() const { return getStatusOnly_; }
            void set_GetStatusOnly(bool value) { getStatusOnly_ = value; }
            bool get_NeedsRefresh() const { Common::AcquireReadLock lock(lock_); return needsRefresh_; }
            void set_NeedsRefresh(bool value) { Common::AcquireWriteLock lock(lock_); needsRefresh_ = value; }

            std::wstring ConstructDeactivationBatchId() const;

            FABRIC_FIELDS_01(task_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            RepairTask task_;

            mutable Common::RwLock lock_;
            bool needsRefresh_;

            // ISSUE: This is specific to node deactivation; refactor
            bool getStatusOnly_;
        };
    }
}
