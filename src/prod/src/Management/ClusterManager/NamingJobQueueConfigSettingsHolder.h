// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class NamingJobQueueConfigSettingsHolder : public Common::JobQueueConfigSettingsHolder
        {
            DENY_COPY(NamingJobQueueConfigSettingsHolder)
        public:
            NamingJobQueueConfigSettingsHolder() {}
            ~NamingJobQueueConfigSettingsHolder() {}

            int get_MaxQueueSize() const override
            {
                return ManagementConfig::GetConfig().NamingJobQueueSize;
            }

            int get_MaxParallelPendingWorkCount() const override
            {
                return ManagementConfig::GetConfig().NamingJobQueueMaxPendingWorkCount;
            }

        protected:
            int get_MaxThreadCountValue() const override
            {
                return ManagementConfig::GetConfig().NamingJobQueueThreadCount;
            }
        };
    }
}
