// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"
#include "winbase.h"

#ifdef __cplusplus
extern "C" {
#endif

// 153
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.

// 243
typedef
BOOL
(WINAPI *PHANDLER_ROUTINE)(
    DWORD CtrlType
    );

// 249
#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2

// 727
WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler(
    __in_opt PHANDLER_ROUTINE HandlerRoutine,
    __in BOOL Add);


#ifdef __cplusplus
}
#endif
