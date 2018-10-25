// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "PAL.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

extern BOOL DefaultCtrlHandler(DWORD fdwCtrlType);

void __attribute__((constructor)) common_ctor()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)DefaultCtrlHandler, true);

    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit (RLIMIT_NOFILE, &rl);

    signal(SIGPIPE, SIG_IGN);
}

// For KTL linkage: resolve LttngTraceWrapper into TraceWrapper in ServiceFabric
extern "C" extern void TraceWrapper(
        const char * taskName,
        const char * eventName,
        int level,
        const char * id,
        const char * data);

extern "C" void LttngTraceWrapper(
        const char * taskName,
        const char * eventName,
        int level,
        const char * id,
        const char * data)
{
    TraceWrapper(taskName, eventName, level, id, data);
}


