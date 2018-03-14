// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "WinErrorCategory.h"

namespace microsoft
{
    namespace detail
    {
        #define NULL 0

        class winerror_category_impl : public std::error_category
        {
        public:
            winerror_category_impl() {}

            virtual std::string message(int value) const  
            {
                std::string result;
                DWORD size = 255;
                result.resize(size);
                DWORD ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM + 255,
                    NULL, value, 0, &result[0], 255,  0);

                if (ret == 0)
                {
                    return "unknown error";
                }
                else
                {
                    result.resize(ret);

                    return result;
                }
            }

            virtual const char* name() const noexcept { return "windows_error"; }
        };

        winerror_category_impl winerror_category_var;
    }

    std::error_category& windows_error_category = detail::winerror_category_var;
}

