// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class Error : private DenyCopy
    {
    public:
        static std::runtime_error LastError(std::string const & what);
        static std::runtime_error WindowsError(DWORD errorCode, std::string const & what);

    private:
        Error()
        {
        }

        ~Error()
        {
        }
    };
}
