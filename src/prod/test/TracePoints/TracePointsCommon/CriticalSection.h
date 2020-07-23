// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class CriticalSection : private DenyCopy
    {
    public:
        CriticalSection()
            : criticalSection_()
        {
            InitializeCriticalSection(&criticalSection_);
        }

        ~CriticalSection()
        {
            DeleteCriticalSection(&criticalSection_);
        }

        void Enter()
        {
            EnterCriticalSection(&criticalSection_);
        }

        void Leave()
        {
            LeaveCriticalSection(&criticalSection_);
        }

    private:
        CRITICAL_SECTION criticalSection_;
    };
}
