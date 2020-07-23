// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef clr_win32_h
#define clr_win32_h

#include "winwrap.h"

namespace clr
{
    namespace win32
    {
        // Prevents an HMODULE from being unloaded until process termination.
        inline
        HRESULT PreventModuleUnload(HMODULE hMod)
        {
            if (!WszGetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCTSTR>(hMod),
                &hMod))
            {
                return HRESULT_FROM_GetLastError();
            }

            return S_OK;
        }
    } // namespace win
} // namespace clr

#endif // clr_win32_h
