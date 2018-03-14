// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace microsoft
{
    extern std::error_category& windows_error_category;
    
    inline std::error_code MakeWindowsErrorCode(LONG errc) { return std::error_code( errc, windows_error_category ); }
    inline std::error_code GetLastErrorCode() { return MakeWindowsErrorCode( GetLastError() ); }
    inline DWORD MakeErrorCodeIntoDword(const std::error_code& errorCode)
    {
        if(errorCode.category() == microsoft::windows_error_category)
        {
            return errorCode.value();
        }
        else
        {
            return ERROR_GEN_FAILURE;
        }
    }
}


