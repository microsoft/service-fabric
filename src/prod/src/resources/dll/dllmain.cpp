// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <windows.h>

#if defined(PLATFORM_UNIX)
#include <map>
#include <string>
#include <sal.h>
#endif

#if !defined(PLATFORM_UNIX)
#include "targetver.h"
#include "resources/Resources.h"


BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID reserved)
{
    UNREFERENCED_PARAMETER(module);
    UNREFERENCED_PARAMETER(reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif
