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
            // Interface that wraps the system clock for easy testing
            class IClock
            {
                DENY_COPY(IClock);

            public:
                IClock() {}

                virtual ~IClock() {}

                virtual Common::StopwatchTime Now() const = 0;

                virtual Common::DateTime DateTimeNow() const = 0;
            };
        }
    }
}



