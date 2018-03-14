// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IPHelper
    {
    public:
        static bool TryParse(const std::wstring &ipString, uint &ip);
        static std::wstring Format(uint ip);
        static bool TryParseCIDR(const std::wstring &cidr, uint &baseIp, int &maskNum);
        static std::wstring FormatCIDR(uint ip, int maskNum);
    };
}
