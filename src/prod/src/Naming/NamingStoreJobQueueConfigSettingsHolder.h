// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingStoreJobQueueConfigSettingsHolder : public Common::JobQueueConfigSettingsHolder
    {
        DENY_COPY(NamingStoreJobQueueConfigSettingsHolder)
    public:
        NamingStoreJobQueueConfigSettingsHolder() {}
        ~NamingStoreJobQueueConfigSettingsHolder() {}

        int get_MaxQueueSize() const override
        {
            return NamingConfig::GetConfig().RequestQueueSize;
        }

        int get_MaxParallelPendingWorkCount() const override
        {
            return NamingConfig::GetConfig().MaxPendingRequestCount;
        }

    protected:
        int get_MaxThreadCountValue() const override
        {
            return NamingConfig::GetConfig().RequestQueueThreadCount;
        }
    };
}
