// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCounts
        {
        public:
            ServiceCounts();

            __declspec (property(get = get_ServiceCount, put = set_ServiceCount)) size_t ServiceCount;
            size_t get_ServiceCount() const { return serviceCount_; }
            void set_ServiceCount(size_t serviceCount) { serviceCount_ = serviceCount; }

            __declspec (property(get = get_StatelessCount, put = set_StatelessCount)) size_t StatelessCount;
            size_t get_StatelessCount() const { return statelessCount_; }
            void set_StatelessCount(size_t statelessCount) { statelessCount_ = statelessCount; }

            __declspec (property(get = get_VolatileCount, put = set_VolatileCount)) size_t VolatileCount;
            size_t get_VolatileCount() const { return volatileCount_; }
            void set_VolatileCount(size_t volatileCount) { volatileCount_ = volatileCount; }

            __declspec (property(get = get_PersistedCount, put = set_PersistedCount)) size_t PersistedCount;
            size_t get_PersistedCount() const { return persistedCount_; }
            void set_PersistedCount(size_t persistedCount) { persistedCount_ = persistedCount; }

            __declspec (property(get = get_UpdatingCount, put = set_UpdatingCount)) size_t UpdatingCount;
            size_t get_UpdatingCount() const { return updatingCount_; }
            void set_UpdatingCount(size_t updatingCount) { updatingCount_ = updatingCount; }

            __declspec (property(get = get_DeletingCount, put = set_DeletingCount)) size_t DeletingCount;
            size_t get_DeletingCount() const { return deletingCount_; }
            void set_DeletingCount(size_t deletingCount) { deletingCount_ = deletingCount; }

            __declspec (property(get = get_DeletedCount, put = set_DeletedCount)) size_t DeletedCount;
            size_t get_DeletedCount() const { return deletedCount_; }
            void set_DeletedCount(size_t deletedCount) { deletedCount_ = deletedCount; }

            __declspec (property(get = get_ServiceTypeCount, put = set_ServiceTypeCount)) size_t ServiceTypeCount;
            size_t get_ServiceTypeCount() const { return serviceTypeCount_; }
            void set_ServiceTypeCount(size_t serviceTypeCount) { serviceTypeCount_ = serviceTypeCount; }

            __declspec (property(get = get_ApplicationCount, put = set_ApplicationCount)) size_t ApplicationCount;
            size_t get_ApplicationCount() const { return applicationCount_; }
            void set_ApplicationCount(size_t applicationCount) { applicationCount_ = applicationCount; }

            __declspec (property(get = get_ApplicationUpgradeCount, put = set_ApplicationUpgradeCount)) size_t ApplicationUpgradeCount;
            size_t get_ApplicationUpgradeCount() const { return applicationUpgradeCount_; }
            void set_ApplicationUpgradeCount(size_t applicationUpgradeCount) { applicationUpgradeCount_ = applicationUpgradeCount; }

            __declspec (property(get = get_MonitoredUpgradeCount, put = set_MonitoredUpgradeCount)) size_t MonitoredUpgradeCount;
            size_t get_MonitoredUpgradeCount() const { return monitoredUpgradeCount_; }
            void set_MonitoredUpgradeCount(size_t monitoredUpgradeCount) { monitoredUpgradeCount_ = monitoredUpgradeCount; }

            __declspec (property(get = get_ManualUpgradeCount, put = set_ManualUpgradeCount)) size_t ManualUpgradeCount;
            size_t get_ManualUpgradeCount() const { return manualUpgradeCount_; }
            void set_ManualUpgradeCount(size_t manualUpgradeCount) { manualUpgradeCount_ = manualUpgradeCount; }

            __declspec (property(get = get_ForceRestartUpgradeCount, put = set_ForceRestartUpgradeCount)) size_t ForceRestartUpgradeCount;
            size_t get_ForceRestartUpgradeCount() const { return forceRestartUpgradeCount_; }
            void set_ForceRestartUpgradeCount(size_t forceRestartUpgradeCount) { forceRestartUpgradeCount_ = forceRestartUpgradeCount; }

            __declspec (property(get = get_NotificationOnlyUpgradeCount, put = set_NotificationOnlyUpgradeCount)) size_t NotificationOnlyUpgradeCount;
            size_t get_NotificationOnlyUpgradeCount() const { return notificationOnlyUpgradeCount_; }
            void set_NotificationOnlyUpgradeCount(size_t notificationOnlyUpgradeCount) { notificationOnlyUpgradeCount_ = notificationOnlyUpgradeCount; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            size_t serviceCount_;
            size_t statelessCount_;
            size_t volatileCount_;
            size_t persistedCount_;

            size_t updatingCount_;
            size_t deletingCount_;
            size_t deletedCount_;
            size_t serviceTypeCount_;

            size_t applicationCount_;
            size_t applicationUpgradeCount_;
            size_t monitoredUpgradeCount_;
            size_t manualUpgradeCount_;
            size_t forceRestartUpgradeCount_;
            size_t notificationOnlyUpgradeCount_;
        };
    }
}
