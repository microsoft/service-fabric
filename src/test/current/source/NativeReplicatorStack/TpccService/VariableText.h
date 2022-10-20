// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    template <int num>
    class VariableText
    {
    public:
        static std::wstring GetValue()
        {
            Common::Random rnd(0);
            return std::wstring(rnd.Next(1, num), L'x');
        }
    };
}
