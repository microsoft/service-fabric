// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class AcquireCriticalSection : private DenyCopy
    {
    public:
        AcquireCriticalSection(CriticalSection & criticalSection)
            : criticalSection_(criticalSection)
        {
            criticalSection_.Enter();
        }

        ~AcquireCriticalSection()
        {
            criticalSection_.Leave();
        }

    private:
        CriticalSection & criticalSection_;
    };
}
