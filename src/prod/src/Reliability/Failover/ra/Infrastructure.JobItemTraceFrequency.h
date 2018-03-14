// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace JobItemTraceFrequency
        {
            enum Enum
            {
                // Job item never needs to be traced
                // Useful for readonly job items etc
                Never = 0,

                // Trace only if the job item was processed
                OnSuccessfulProcess = 1,

                // Trace always
                // Useful for messages etc where we want to trace even if the message was dropped because of staleness etc
                Always = 2,
            };
        }
    }
}

