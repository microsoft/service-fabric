// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ServiceModel/management/ResourceMonitor/public.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ResourceMonitor
        {
            class ResourceComponent
            {
                DENY_COPY(ResourceComponent);

            public:
                ResourceComponent(ReconfigurationAgent & ra);
                bool ProcessResoureUsageMessage(Management::ResourceMonitor::ResourceUsageReport const & resourceUsageReport);

            private:
                ReconfigurationAgent & ra_;
                Common::StopwatchTime lastTraceTime_;
            };
        }
    }
}

