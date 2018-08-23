// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "wincon.h"
#include "winerror.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

static PHANDLER_ROUTINE gHandlerRoutine = NULL;

static void sighandler_wrapper(int code, siginfo_t *siginfo, void *context)
{
    if(gHandlerRoutine)
        (*gHandlerRoutine)(CTRL_C_EVENT);
}

BOOL DefaultCtrlHandler(DWORD fdwCtrlType)
{
    // See http://www.tldp.org/LDP/abs/html/exitcodes.html
    // Standard exit code for Ctrl-C on Linux is 130, and the previous code used here by PAL was ERROR_CONTROL_C_EXIT == 572, which is out of bounds for the Linux single-byte-width exit code. 
    // This was causing error code truncation and improper termination handling due to error code modulo 256 evaluation. As well, Fabric exit codes within the single byte range are all occupied
    // so by using the Linux Ctrl-C code or any other code within 256 we would have a collision against existing codes. For these reasons we return ERROR_SUCCESS here since Ctrl-C calls are
    // invoked by Fabric as part of typical operation to manage the application lifecycle.
    ExitProcess(ERROR_SUCCESS);
}

WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler(
    __in_opt PHANDLER_ROUTINE HandlerRoutine,
    __in BOOL Add)
{
    // set default sigint handler
    struct sigaction action;
    action.sa_flags = SA_RESTART;
    action.sa_handler = NULL;
    action.sa_sigaction = sighandler_wrapper;
    action.sa_flags |= SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);

    if(gHandlerRoutine && (gHandlerRoutine != DefaultCtrlHandler))
    {
        ASSERT("SetConsoleCtrlHandler only support one handler per process.");
    }
    else
    {
        gHandlerRoutine = HandlerRoutine;
    }

    return true;
}
