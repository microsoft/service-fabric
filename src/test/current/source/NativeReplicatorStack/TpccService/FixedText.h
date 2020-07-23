// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    template<int num>
    class FixedText
    {
    private:
        static const std::wstring value;

    public:
        static std::wstring GetValue()
        {
            return FixedText::value;
        }
    };

    template<int num>
    const std::wstring FixedText<num>::value = std::wstring(num, L'x');
}
