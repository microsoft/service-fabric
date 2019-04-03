// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <windows.h>
#include "detoured.h"

static HMODULE s_hDll;

HMODULE WINAPI Detoured()
{
    return s_hDll;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        s_hDll = hinst;
        DisableThreadLibraryCalls(hinst);
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.
