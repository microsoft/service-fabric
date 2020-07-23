// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class MACHelper
    {
    public:
        static bool TryParse(const std::wstring & macString, uint64 & mac);
        static std::wstring Format(uint64 mac);

    private:
        static std::wstring MACHelper::ConvertDecimalToHexString(uint decimalValue);
    };
}
