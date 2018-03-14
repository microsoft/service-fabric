// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FormatOptions
    {
        DENY_COPY(FormatOptions);
    public:
        bool leadingZero;
        std::string formatString;
        int width;

        FormatOptions(int width, bool leadingZero, std::string const & formatString) 
            : leadingZero(leadingZero), width(width), formatString(formatString)
        {}
    };

    extern FormatOptions& null_format;
}
