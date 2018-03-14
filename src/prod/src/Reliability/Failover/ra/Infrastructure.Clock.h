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
            class Clock : public IClock
            {
            public:
                Clock() {}

                Common::StopwatchTime Now() const override
                {
                    return Common::Stopwatch::Now();
                }

                Common::DateTime DateTimeNow() const
                {
                    return Common::DateTime::Now();
                }
            };
        }
    }
}



