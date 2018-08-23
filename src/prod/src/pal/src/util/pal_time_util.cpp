// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "pal_time_util.h"

FILETIME UnixTimeToFILETIME(time_t sec, long nsec)
{
    int64_t elapsed;
    FILETIME ft;
    elapsed = ((int64_t)sec) * 10000000 + WINDOWS_LINUX_EPOCH_BIAS + (nsec / 100);
    ft.dwLowDateTime = (DWORD)elapsed;
    ft.dwHighDateTime = (DWORD)(elapsed >> 32);
    return ft;
}
