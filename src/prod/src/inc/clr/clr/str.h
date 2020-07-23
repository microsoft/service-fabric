// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef _clr_str_h_
#define _clr_str_h_

namespace clr
{
    namespace str
    {
        //-----------------------------------------------------------------------------------------
        // Returns true if the provided string is a null pointer or the empty string.
        static inline bool
        IsNullOrEmpty(LPCWSTR wzStr)
        {
            return wzStr == nullptr || *wzStr == W('\0');
        }
    }
}

#endif // _clr_str_h_

