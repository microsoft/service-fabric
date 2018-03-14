// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            namespace StalenessCheckType
            {
                enum Enum
                {
                    // No staleness check
                    None,

                    // Standard staleness checks for partition related failover messages
                    FTFailover,

                    // Standard staleness checks for partition related rap messages
                    FTProxy,
                };
            }
        }
    }
}
