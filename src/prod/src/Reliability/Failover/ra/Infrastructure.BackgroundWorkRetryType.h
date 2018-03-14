// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            namespace BackgroundWorkRetryType
            {
                enum Enum
                {
                    // No further retry is needed for this background work
                    None,

                    // Retry immediately
                    // This is subject to min interval between work
                    Immediate,

                    // Retry after retry interval
                    Deferred
                };
            }
        }
    }
}



