// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//
// Defina all fabric signal # in one place to avoid conflicts
//

#pragma once

#define FABRIC_SIGNO_TIMER       (SIGRTMAX - 5)
#define FABRIC_SIGNO_LEASE_TIMER (SIGRTMAX - 6)

namespace Common
{
    class SigUtil
    {
    public:
        static void BlockSignalOnCallingThread(int sig);
        static void BlockAllFabricSignalsOnCallingThread();
    };
}
