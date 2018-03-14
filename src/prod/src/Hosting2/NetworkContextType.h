// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace NetworkContextType
    {
        enum Enum
        {
            None = 0,
            Flat = 1,
            Overlay = 2
        };

        std::wstring ToString(Enum const & val);
        Common::ErrorCode FromString(std::wstring const & str, __out Enum & val);
    }
}
