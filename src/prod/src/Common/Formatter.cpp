// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace detail
    {
        format_handler format_impl;
        wformat_handler wformat_impl;
    };

    detail::format_handler & formatString = detail::format_impl;
    detail::wformat_handler & wformatString = detail::wformat_impl;

    FormatOptions null_format_impl(0, false, "");
    FormatOptions& null_format = null_format_impl;
}
